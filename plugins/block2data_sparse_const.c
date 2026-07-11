#include	<stdio.h>
#include	<string.h>

#include	"plugins.h"
#include	"gc_malloc.h"


#define SPARSE_CONST_IR_ITEM_LIMIT	64
#define SPARSE_CONST_SIMD_MIN_RUN	8

#define sparse_emit_push(t, ...) \
	do { \
		char *_list[] = {__VA_ARGS__}; \
		int_t _i; \
		for (_i=0; _i<(int_t)(sizeof(_list)/sizeof(char*)); _i++) \
			target_push((t), _list[_i]); \
	} while (0)

typedef struct
{
	int_t	k_offset;
	int_t	x_offset;
	int_t	out_offset;
	double	k_value;
} sparse_item_t;

typedef struct
{
	sparse_item_t *p;
	int_t n;
	int_t cap;
} sparse_item_vec_t;

typedef enum
{
	sparse_emit_direct,
	sparse_emit_ir_rewrite,
	sparse_emit_simd_block
} sparse_emit_path_t;


static int is_single_datadesc_set(block ob)
{
	return ob
		&& ob->type == block_type_datadesc_set
		&& ob->p.d
		&& ob->p.d->n == 1
		&& ob->p.d->p[0];
}

static int try_extract_K_X(block par_int, datadesc *out_K, datadesc *out_X)
{
	block left, right;
	datadesc dl, dr;

	if (!par_int || par_int->type != block_type_con_par_int)
		return 0;

	left = par_int->p.b[0];
	right = par_int->p.b[1];
	if (!is_single_datadesc_set(left) || !is_single_datadesc_set(right))
		return 0;

	dl = left->p.d->p[0];
	dr = right->p.d->p[0];

	if (dl->region == region_direct && dr->region != region_direct)
	{
		*out_K = dl;
		*out_X = dr;
		return 1;
	}
	if (dr->region == region_direct && dl->region != region_direct)
	{
		*out_K = dr;
		*out_X = dl;
		return 1;
	}

	return 0;
}

static int_t active_ndim(datadesc d)
{
	int_t i;

	if (!d)
		return 0;
	for (i=0; i<d->n_dim && d->dims[i]; i++);
	return i;
}

static int is_dense_datadesc(datadesc d)
{
	int_t i;
	int_t ndim = active_ndim(d);

	if (ndim == 0)
		return 0;

	for (i=0; i<ndim; i++)
		if (!d->dims[i] || d->dims[i]->n_tuple != 1 || d->dims[i]->n_entry <= 0)
			return 0;

	return 1;
}

static int is_sparse_tuple_datadesc(datadesc d)
{
	return d
		&& active_ndim(d) == 1
		&& d->dims[0]
		&& d->dims[0]->n_tuple > 1
		&& d->dims[0]->n_entry >= 0;
}

static int_t find_dense_dim(datadesc d, int_t dim_name)
{
	int_t i;
	int_t ndim = active_ndim(d);

	for (i=0; i<ndim; i++)
		if (d->dims[i]->n_tuple == 1 && d->dims[i]->dims[0] == dim_name)
			return i;

	return -1;
}

static int_t find_tuple_dim(dimdesc d, int_t dim_name)
{
	int_t i;

	for (i=0; i<d->n_tuple; i++)
		if (d->dims[i] == dim_name)
			return i;

	return -1;
}

static int_t find_entry_by_index(dimdesc d, int_t index)
{
	int_t i;

	for (i=0; i<d->n_entry; i++)
		if (d->indices[i * d->n_tuple] == index)
			return i;

	return -1;
}

static int same_dense_index_set(dimdesc a, dimdesc b)
{
	int_t i;

	if (!a || !b || a->n_tuple != 1 || b->n_tuple != 1)
		return 0;
	if (a->dims[0] != b->dims[0] || a->n_entry != b->n_entry)
		return 0;

	for (i=0; i<a->n_entry; i++)
		if (find_entry_by_index(b, a->indices[i]) < 0)
			return 0;

	return 1;
}

static int dense_layout_matches_by_name(datadesc K, datadesc X)
{
	int_t i;
	int_t k_ndim = active_ndim(K);
	int_t x_ndim = active_ndim(X);

	if (!is_dense_datadesc(K) || !is_dense_datadesc(X) || k_ndim != x_ndim)
		return 0;

	for (i=0; i<x_ndim; i++)
	{
		int_t k_dim = find_dense_dim(K, X->dims[i]->dims[0]);

		if (k_dim < 0 || !same_dense_index_set(K->dims[k_dim], X->dims[i]))
			return 0;
	}

	return 1;
}

static int sparse_tuple_matches_dense(datadesc tuple_data, datadesc dense)
{
	int_t i;
	dimdesc td;

	if (!is_sparse_tuple_datadesc(tuple_data) || !is_dense_datadesc(dense))
		return 0;

	td = tuple_data->dims[0];
	if (td->n_tuple != active_ndim(dense))
		return 0;

	for (i=0; i<td->n_tuple; i++)
		if (find_dense_dim(dense, td->dims[i]) < 0)
			return 0;

	return 1;
}

static int sparse_tuple_matches_sparse_tuple(datadesc K, datadesc X)
{
	int_t i;
	dimdesc kd, xd;

	if (!is_sparse_tuple_datadesc(K) || !is_sparse_tuple_datadesc(X))
		return 0;

	kd = K->dims[0];
	xd = X->dims[0];
	if (kd->n_tuple != xd->n_tuple)
		return 0;

	for (i=0; i<xd->n_tuple; i++)
		if (find_tuple_dim(kd, xd->dims[i]) < 0)
			return 0;

	return 1;
}

static int tuple_entry_to_dense_offset(datadesc tuple_data, int_t entry, datadesc dense, int_t *offset)
{
	int_t i;
	int_t off = 0;
	dimdesc tuple_dim = tuple_data->dims[0];

	for (i=0; i<tuple_dim->n_tuple; i++)
	{
		int_t dense_dim_index = find_dense_dim(dense, tuple_dim->dims[i]);
		int_t tuple_index = tuple_dim->indices[entry * tuple_dim->n_tuple + i];
		int_t dense_entry;

		if (dense_dim_index < 0)
			return 0;

		dense_entry = find_entry_by_index(dense->dims[dense_dim_index], tuple_index);
		if (dense_entry < 0)
			return 0;

		off += dense->dims[dense_dim_index]->offsets[dense_entry];
	}

	*offset = off;
	return 1;
}

static int tuple_entries_match(dimdesc a, int_t ae, dimdesc b, int_t be)
{
	int_t i;

	if (a->n_tuple != b->n_tuple)
		return 0;

	for (i=0; i<a->n_tuple; i++)
	{
		int_t bj = find_tuple_dim(b, a->dims[i]);

		if (bj < 0)
			return 0;
		if (a->indices[ae * a->n_tuple + i] != b->indices[be * b->n_tuple + bj])
			return 0;
	}

	return 1;
}

static int_t find_matching_tuple_entry(datadesc tuple_data, dimdesc coords, int_t coord_entry)
{
	int_t i;
	dimdesc td = tuple_data->dims[0];

	for (i=0; i<td->n_entry; i++)
		if (tuple_entries_match(td, i, coords, coord_entry))
			return i;

	return -1;
}

static int_t data_span_bytes(datadesc d)
{
	int_t i, j;
	int_t max_offset = 0;
	int_t ndim = active_ndim(d);

	if (ndim == 0)
		return d->elem_size;

	if (is_sparse_tuple_datadesc(d))
	{
		for (j=0; j<d->dims[0]->n_entry; j++)
			if (d->dims[0]->offsets[j] > max_offset)
				max_offset = d->dims[0]->offsets[j];
		return (max_offset + 1) * d->elem_size;
	}

	for (i=0; i<ndim; i++)
	{
		int_t dim_max = 0;

		for (j=0; j<d->dims[i]->n_entry; j++)
			if (d->dims[i]->offsets[j] > dim_max)
				dim_max = d->dims[i]->offsets[j];

		max_offset += dim_max;
	}

	return (max_offset + 1) * d->elem_size;
}

static double read_K_value_at_offset(datadesc K, int_t elem_offset)
{
	char *base = (char *)K->ref.p_direct + K->offset0;
	int_t off = elem_offset * K->elem_size;

	if (K->elem_size == 4)
		return *(float *)(base + off);
	else
		return *(double *)(base + off);
}

static char *literal_real(datadesc K, double value)
{
	char *res = malloc(80);

	if (K->elem_size == 4)
		snprintf(res, 80, "%.9ef", (float)value);
	else
		snprintf(res, 80, "%.17e", value);

	return res;
}

static void item_vec_init(sparse_item_vec_t *v)
{
	v->n = 0;
	v->cap = 32;
	v->p = malloc(sizeof(sparse_item_t) * v->cap);
}

static void item_vec_push(sparse_item_vec_t *v, int_t k_offset, int_t x_offset,
                          int_t out_offset, double k_value)
{
	if (v->n == v->cap)
	{
		sparse_item_t *p;

		v->cap *= 2;
		p = malloc(sizeof(sparse_item_t) * v->cap);
		memcpy(p, v->p, sizeof(sparse_item_t) * v->n);
		v->p = p;
	}

	v->p[v->n].k_offset = k_offset;
	v->p[v->n].x_offset = x_offset;
	v->p[v->n].out_offset = out_offset;
	v->p[v->n].k_value = k_value;
	v->n++;
}

static void append_useful_item(sparse_item_vec_t *items, enum_block_type reduce_type,
                               int_t k_offset, int_t x_offset, int_t out_offset,
                               double kv)
{
	if (reduce_type == block_builtin_reduce_prod && kv == 0.0)
		return;
	if (reduce_type == block_builtin_reduce_sum && kv == 0.0)
		return;

	item_vec_push(items, k_offset, x_offset, out_offset, kv);
}

static void sort_items_by_out(sparse_item_vec_t *items)
{
	int_t i, j;

	for (i=1; i<items->n; i++)
	{
		sparse_item_t cur = items->p[i];

		for (j=i; j>0 && items->p[j-1].out_offset > cur.out_offset; j--)
			items->p[j] = items->p[j-1];
		items->p[j] = cur;
	}
}

static int items_have_unique_outputs(sparse_item_vec_t *items)
{
	int_t i;

	for (i=1; i<items->n; i++)
		if (items->p[i-1].out_offset == items->p[i].out_offset)
			return 0;

	return 1;
}

static int_t longest_contiguous_run(sparse_item_vec_t *items)
{
	int_t i;
	int_t cur = 1;
	int_t best = 0;

	if (items->n == 0)
		return 0;

	best = 1;
	for (i=1; i<items->n; i++)
	{
		if (items->p[i].out_offset == items->p[i-1].out_offset + 1
		 && items->p[i].x_offset == items->p[i-1].x_offset + 1)
		{
			cur++;
		}
		else
		{
			if (cur > best)
				best = cur;
			cur = 1;
		}
	}
	if (cur > best)
		best = cur;

	return best;
}

static void increment_positions(int_t ndim, int_t *pos, datadesc d, int *done)
{
	int_t i;

	for (i=ndim; i-->0;)
	{
		pos[i]++;
		if (pos[i] < d->dims[i]->n_entry)
			return;
		pos[i] = 0;
	}

	*done = 1;
}

static int build_dense_dense_items(datadesc K, datadesc X, enum_block_type reduce_type,
                                   sparse_item_vec_t *items)
{
	int done = 0;
	int_t x_ndim = active_ndim(X);
	int_t *pos;
	int_t j;

	if (!dense_layout_matches_by_name(K, X))
		return 0;

	pos = malloc(sizeof(int_t) * x_ndim);
	for (j=0; j<x_ndim; j++)
		pos[j] = 0;

	while (!done)
	{
		int_t k_idx = 0;
		int_t x_idx = 0;

		for (j=0; j<x_ndim; j++)
		{
			int_t x_dim_name = X->dims[j]->dims[0];
			int_t k_dim = find_dense_dim(K, x_dim_name);
			int_t x_index = X->dims[j]->indices[pos[j]];
			int_t k_entry = find_entry_by_index(K->dims[k_dim], x_index);

			if (k_entry < 0)
				return 0;

			k_idx += K->dims[k_dim]->offsets[k_entry];
			x_idx += X->dims[j]->offsets[pos[j]];
		}

		append_useful_item(items, reduce_type, k_idx, x_idx, x_idx,
		                   read_K_value_at_offset(K, k_idx));

		increment_positions(x_ndim, pos, X, &done);
	}

	return 1;
}

static int build_tuple_dense_items(datadesc K, datadesc X, enum_block_type reduce_type,
                                   sparse_item_vec_t *items)
{
	int_t i;
	dimdesc kd;

	if (!sparse_tuple_matches_dense(K, X))
		return 0;

	kd = K->dims[0];
	for (i=0; i<kd->n_entry; i++)
	{
		int_t x_idx;
		int_t k_idx = kd->offsets[i];

		if (!tuple_entry_to_dense_offset(K, i, X, &x_idx))
			return 0;

		append_useful_item(items, reduce_type, k_idx, x_idx, x_idx,
		                   read_K_value_at_offset(K, k_idx));
	}

	return 1;
}

static int build_dense_tuple_items(datadesc K, datadesc X, enum_block_type reduce_type,
                                   sparse_item_vec_t *items)
{
	int_t i;
	dimdesc xd;

	if (!sparse_tuple_matches_dense(X, K))
		return 0;

	xd = X->dims[0];
	for (i=0; i<xd->n_entry; i++)
	{
		int_t k_idx;
		int_t x_idx = xd->offsets[i];

		if (!tuple_entry_to_dense_offset(X, i, K, &k_idx))
			return 0;

		append_useful_item(items, reduce_type, k_idx, x_idx, x_idx,
		                   read_K_value_at_offset(K, k_idx));
	}

	return 1;
}

static int build_tuple_tuple_items(datadesc K, datadesc X, enum_block_type reduce_type,
                                   sparse_item_vec_t *items)
{
	int_t i;
	dimdesc xd;

	if (!sparse_tuple_matches_sparse_tuple(K, X))
		return 0;

	xd = X->dims[0];
	for (i=0; i<xd->n_entry; i++)
	{
		int_t k_entry = find_matching_tuple_entry(K, xd, i);
		int_t x_idx = xd->offsets[i];
		int_t k_idx = 0;
		double kv = 0.0;

		if (k_entry >= 0)
		{
			k_idx = K->dims[0]->offsets[k_entry];
			kv = read_K_value_at_offset(K, k_idx);
		}

		append_useful_item(items, reduce_type, k_idx, x_idx, x_idx, kv);
	}

	return 1;
}

static int build_sparse_items(datadesc K, datadesc X, enum_block_type reduce_type,
                              sparse_item_vec_t *items)
{
	int ok = 0;

	if (is_dense_datadesc(K) && is_dense_datadesc(X))
		ok = build_dense_dense_items(K, X, reduce_type, items);
	else if (is_sparse_tuple_datadesc(K) && is_dense_datadesc(X))
		ok = build_tuple_dense_items(K, X, reduce_type, items);
	else if (is_dense_datadesc(K) && is_sparse_tuple_datadesc(X))
		ok = build_dense_tuple_items(K, X, reduce_type, items);
	else if (is_sparse_tuple_datadesc(K) && is_sparse_tuple_datadesc(X))
		ok = build_tuple_tuple_items(K, X, reduce_type, items);

	if (!ok)
		return 0;

	sort_items_by_out(items);
	return items_have_unique_outputs(items);
}

static datadesc make_output_desc(datadesc X)
{
	datadesc out;
	int_t i;

	out = datadesc_new(X->n_dim);
	out->region = region_main;
	out->ref.p_refname = token_new();
	out->offset0 = 0;
	out->endian = X->endian;
	out->elem_type = X->elem_type;
	out->elem_size = X->elem_size;
	out->n_dim = X->n_dim;
	for (i=0; i<X->n_dim; i++)
		out->dims[i] = X->dims[i] ? dimdesc_copy(X->dims[i]) : NULL;

	return out;
}

static sparse_emit_path_t choose_emit_path(sparse_item_vec_t *items)
{
	if (items->n == 0)
		return sparse_emit_direct;

	if (longest_contiguous_run(items) >= SPARSE_CONST_SIMD_MIN_RUN)
		return sparse_emit_simd_block;

	if (items->n > SPARSE_CONST_IR_ITEM_LIMIT)
		return sparse_emit_ir_rewrite;

	return sparse_emit_direct;
}

static int emit_output_allocation(target t, datadesc K, datadesc X, datadesc out,
                                  enum_block_type reduce_type, int_t nbytes)
{
	char *type_str = target_query_type(t, elem_type_real, K->elem_size);

	if (!type_str)
		return 0;

	sparse_emit_push(t, type_str, "*", out->ref.p_refname,
	                 "=(", type_str, "*)malloc(", mt_itoa(nbytes), ");");
	if (reduce_type == block_builtin_reduce_prod)
	{
		sparse_emit_push(t, "memset(", out->ref.p_refname, ",0,",
		                 mt_itoa(nbytes), ");");
	}
	else
	{
		sparse_emit_push(t, "memcpy(", out->ref.p_refname, ",(char*)(",
		                 X->ref.p_refname, ")+", mt_itoa(X->offset0), ",",
		                 mt_itoa(nbytes), ");");
	}

	return 1;
}

static int emit_scalar_item(target t, datadesc K, enum_block_type reduce_type,
                            char *X_var, char *out_var, sparse_item_t *it)
{
	if (reduce_type == block_builtin_reduce_prod && it->k_value == 1.0)
	{
		sparse_emit_push(t, out_var, "[", mt_itoa(it->out_offset), "]=",
		                 X_var, "[", mt_itoa(it->x_offset), "];");
	}
	else if (reduce_type == block_builtin_reduce_sum)
	{
		sparse_emit_push(t, out_var, "[", mt_itoa(it->out_offset), "]=",
		                 literal_real(K, it->k_value), "+", X_var, "[",
		                 mt_itoa(it->x_offset), "];");
	}
	else
	{
		sparse_emit_push(t, out_var, "[", mt_itoa(it->out_offset), "]=",
		                 literal_real(K, it->k_value), "*", X_var, "[",
		                 mt_itoa(it->x_offset), "];");
	}

	return 1;
}

static int emit_simd_block_item_run(target t, datadesc K, enum_block_type reduce_type,
                                    char *X_var, char *out_var,
                                    sparse_item_vec_t *items, int_t start, int_t len)
{
	int_t i;
	int_t width = (K->elem_size == 4) ? 8 : 4;
	char *type_str = target_query_type(t, elem_type_real, K->elem_size);
	char *k_arr = token_new();
	char *idx = token_new();

	if (!type_str || len <= 0)
		return 0;

	sparse_emit_push(t, "{const ", type_str, " ", k_arr, "[", mt_itoa(len), "]={");
	for (i=0; i<len; i++)
	{
		if (i > 0)
			sparse_emit_push(t, ",");
		sparse_emit_push(t, literal_real(K, items->p[start + i].k_value));
	}
	sparse_emit_push(t, "};int32_t ", idx, "=0;");

	sparse_emit_push(t, "\n#if defined(__AVX2__)\n");
	if (K->elem_size == 4)
	{
		char *vk = token_new();
		char *vx = token_new();
		char *vy = token_new();

		sparse_emit_push(t, "for(;", idx, "+", mt_itoa(width), "<=",
		                 mt_itoa(len), ";", idx, "+=", mt_itoa(width), "){");
		sparse_emit_push(t, "__m256 ", vk, "=_mm256_loadu_ps(", k_arr, "+", idx, ");");
		sparse_emit_push(t, "__m256 ", vx, "=_mm256_loadu_ps(", X_var, "+",
		                 mt_itoa(items->p[start].x_offset), "+", idx, ");");
		if (reduce_type == block_builtin_reduce_sum)
			sparse_emit_push(t, "__m256 ", vy, "=_mm256_add_ps(", vk, ",", vx, ");");
		else
			sparse_emit_push(t, "__m256 ", vy, "=_mm256_mul_ps(", vk, ",", vx, ");");
		sparse_emit_push(t, "_mm256_storeu_ps(", out_var, "+",
		                 mt_itoa(items->p[start].out_offset), "+", idx, ",", vy, ");}");
	}
	else
	{
		char *vk = token_new();
		char *vx = token_new();
		char *vy = token_new();

		sparse_emit_push(t, "for(;", idx, "+", mt_itoa(width), "<=",
		                 mt_itoa(len), ";", idx, "+=", mt_itoa(width), "){");
		sparse_emit_push(t, "__m256d ", vk, "=_mm256_loadu_pd(", k_arr, "+", idx, ");");
		sparse_emit_push(t, "__m256d ", vx, "=_mm256_loadu_pd(", X_var, "+",
		                 mt_itoa(items->p[start].x_offset), "+", idx, ");");
		if (reduce_type == block_builtin_reduce_sum)
			sparse_emit_push(t, "__m256d ", vy, "=_mm256_add_pd(", vk, ",", vx, ");");
		else
			sparse_emit_push(t, "__m256d ", vy, "=_mm256_mul_pd(", vk, ",", vx, ");");
		sparse_emit_push(t, "_mm256_storeu_pd(", out_var, "+",
		                 mt_itoa(items->p[start].out_offset), "+", idx, ",", vy, ");}");
	}
	sparse_emit_push(t, "\n#endif\n");

	sparse_emit_push(t, "for(;", idx, "<", mt_itoa(len), ";", idx, "++){");
	if (reduce_type == block_builtin_reduce_sum)
	{
		sparse_emit_push(t, out_var, "[", mt_itoa(items->p[start].out_offset), "+", idx,
		                 "]=", k_arr, "[", idx, "]+", X_var, "[",
		                 mt_itoa(items->p[start].x_offset), "+", idx, "];");
	}
	else
	{
		sparse_emit_push(t, out_var, "[", mt_itoa(items->p[start].out_offset), "+", idx,
		                 "]=", k_arr, "[", idx, "]*", X_var, "[",
		                 mt_itoa(items->p[start].x_offset), "+", idx, "];");
	}
	sparse_emit_push(t, "}}");

	return 1;
}

static int emit_direct_or_simd_path(target t, datadesc K, datadesc X, datadesc out,
                                    sparse_item_vec_t *items, enum_block_type reduce_type,
                                    sparse_emit_path_t path)
{
	char *type_str = target_query_type(t, elem_type_real, K->elem_size);
	char *X_var;
	char *out_var;
	int_t i;

	if (!type_str)
		return 0;

	sparse_emit_push(t, "{");
	X_var = token_new();
	out_var = token_new();
	sparse_emit_push(t, type_str, "*", X_var, "=(", type_str, "*)((char*)(",
	                 X->ref.p_refname, ")+", mt_itoa(X->offset0), ");");
	sparse_emit_push(t, type_str, "*", out_var, "=(", type_str, "*)((char*)(",
	                 out->ref.p_refname, ")+", mt_itoa(out->offset0), ");");

	for (i=0; i<items->n;)
	{
		int_t j = i + 1;

		while (j < items->n
		    && items->p[j].out_offset == items->p[j-1].out_offset + 1
		    && items->p[j].x_offset == items->p[j-1].x_offset + 1)
			j++;

		if (path == sparse_emit_simd_block && j - i >= SPARSE_CONST_SIMD_MIN_RUN)
		{
			if (!emit_simd_block_item_run(t, K, reduce_type, X_var, out_var, items, i, j - i))
				return 0;
		}
		else
		{
			for (; i<j; i++)
				if (!emit_scalar_item(t, K, reduce_type, X_var, out_var, &items->p[i]))
					return 0;
			continue;
		}
		i = j;
	}

	sparse_emit_push(t, "}");
	return 1;
}

static dimdesc make_sparse_ir_dim(int_t dim_name, int_t n, int_t *offsets)
{
	dimdesc d;
	int_t i;

	d = dimdesc_new(1, n);
	d->reduce_type = reduce_type_none;
	d->dims[0] = dim_name;
	for (i=0; i<n; i++)
	{
		d->indices[i] = i;
		d->offsets[i] = offsets ? offsets[i] : i;
	}

	return d;
}

static datadesc make_sparse_ir_desc(enum_region region, char *ref_name,
                                    enum_endian endian, enum_elem_type elem_type,
                                    int_t elem_size, int_t n, int_t dim_name,
                                    int_t *offsets)
{
	datadesc d;

	d = datadesc_new(1);
	d->region = region;
	d->ref.p_refname = ref_name;
	d->offset0 = 0;
	d->endian = endian;
	d->elem_type = elem_type;
	d->elem_size = elem_size;
	d->n_dim = 1;
	d->dims[0] = make_sparse_ir_dim(dim_name, n, offsets);

	return d;
}

static int emit_ir_rewrite_path(target t, datadesc K, datadesc X, datadesc out,
                                sparse_item_vec_t *items, enum_block_type reduce_type)
{
	char *type_str = target_query_type(t, elem_type_real, K->elem_size);
	char *k_arr;
	int_t *x_offsets;
	int_t i;
	int_t sparse_dim;
	datadesc K_ir, X_ir;
	datadesc_set sK, sX, tmp;
	block bK, bX, par, ir_ob;
	char *tmp_var;
	char *out_var;
	char *out_offsets_arr;
	char *scatter_i;

	if (!type_str)
		return 0;
	if (items->n == 0)
		return 1;

	k_arr = token_new();
	x_offsets = malloc(sizeof(int_t) * items->n);

	sparse_emit_push(t, "{const ", type_str, " ", k_arr, "[", mt_itoa(items->n), "]={");
	for (i=0; i<items->n; i++)
	{
		if (i > 0)
			sparse_emit_push(t, ",");
		sparse_emit_push(t, literal_real(K, items->p[i].k_value));
		x_offsets[i] = items->p[i].x_offset;
	}
	sparse_emit_push(t, "};");

	sparse_dim = dimtmp_new();
	K_ir = make_sparse_ir_desc(region_main, k_arr, X->endian, X->elem_type,
	                           X->elem_size, items->n, sparse_dim, NULL);
	X_ir = make_sparse_ir_desc(region_main, X->ref.p_refname, X->endian, X->elem_type,
	                           X->elem_size, items->n, sparse_dim, x_offsets);
	X_ir->offset0 = X->offset0;

	sK = datadesc_set_new(1);
	sK->p[0] = K_ir;
	sX = datadesc_set_new(1);
	sX->p[0] = X_ir;
	bK = block_new(block_type_datadesc_set, sK);
	bX = block_new(block_type_datadesc_set, sX);
	par = block_new(block_type_con_par_int, bK, bX);
	ir_ob = block_new(block_type_con_ser, block_new(reduce_type), par);

	tmp = block2data_arithmetic(t, ir_ob);
	if (!tmp || tmp->n != 1 || !tmp->p[0])
		return 0;

	tmp_var = token_new();
	out_var = token_new();
	sparse_emit_push(t, type_str, "*", tmp_var, "=(", type_str, "*)((char*)(",
	                 tmp->p[0]->ref.p_refname, ")+", mt_itoa(tmp->p[0]->offset0), ");");
	sparse_emit_push(t, type_str, "*", out_var, "=(", type_str, "*)((char*)(",
	                 out->ref.p_refname, ")+", mt_itoa(out->offset0), ");");

	out_offsets_arr = token_new();
	scatter_i = token_new();
	sparse_emit_push(t, "const int32_t ", out_offsets_arr, "[", mt_itoa(items->n), "]={");
	for (i=0; i<items->n; i++)
	{
		if (i > 0)
			sparse_emit_push(t, ",");
		sparse_emit_push(t, mt_itoa(items->p[i].out_offset));
	}
	sparse_emit_push(t, "};");
	sparse_emit_push(t, "for(int32_t ", scatter_i, "=0;", scatter_i, "<",
	                 mt_itoa(items->n), ";", scatter_i, "++){",
	                 out_var, "[", out_offsets_arr, "[", scatter_i, "]]=",
	                 tmp_var, "[", scatter_i, "];}");

	sparse_emit_push(t, "free(", tmp->p[0]->ref.p_refname, ");}");
	return 1;
}

datadesc_set	block2data_sparse_const(target t, block ob)
{
	datadesc K = NULL;
	datadesc X = NULL;
	datadesc out;
	datadesc_set res;
	sparse_item_vec_t items;
	enum_block_type reduce_type;
	sparse_emit_path_t path;
	int_t nbytes;

	if (!ob
	 || ob->type != block_type_con_ser
	 || !ob->p.b[0]
	 || (ob->p.b[0]->type != block_builtin_reduce_prod
	  && ob->p.b[0]->type != block_builtin_reduce_sum)
	 || !try_extract_K_X(ob->p.b[1], &K, &X))
		return 0;

	emit_start(t);

	fprintf(stderr, "SPARSE PATH HIT\n");

	reduce_type = ob->p.b[0]->type;

	emit_assert(K->elem_type == elem_type_real);
	emit_assert(X->elem_type == elem_type_real);
	emit_assert(K->elem_size == 4 || K->elem_size == 8);
	emit_assert(X->elem_size == K->elem_size);
	emit_assert(X->region == region_main);
	emit_assert(is_dense_datadesc(X) || is_sparse_tuple_datadesc(X));
	emit_assert(is_dense_datadesc(K) || is_sparse_tuple_datadesc(K));

	item_vec_init(&items);
	emit_assert(build_sparse_items(K, X, reduce_type, &items));

	out = make_output_desc(X);
	nbytes = data_span_bytes(out);
	emit_assert(emit_output_allocation(t, K, X, out, reduce_type, nbytes));

	path = choose_emit_path(&items);
	if (path == sparse_emit_ir_rewrite)
	{
		fprintf(stderr, "SPARSE IR REWRITE PATH HIT\n");
		emit_assert(emit_ir_rewrite_path(t, K, X, out, &items, reduce_type));
	}
	else
	{
		if (path == sparse_emit_simd_block)
			fprintf(stderr, "SPARSE SIMD BLOCK PATH HIT\n");
		emit_assert(emit_direct_or_simd_path(t, K, X, out, &items, reduce_type, path));
	}

	emit_finish();

	res = datadesc_set_new(1);
	res->p[0] = out;
	return res;
}
