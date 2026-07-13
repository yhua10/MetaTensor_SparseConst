#include	<stdio.h>
#include	<string.h>

#include	"plugins.h"
#include	"gc_malloc.h"


typedef struct
{
	int_t k_offset;
	int_t x_offset;
	int_t out_offset;
	double k_value;
} opt_sparse_item_t;

typedef struct
{
	opt_sparse_item_t *p;
	int_t n;
	int_t cap;
} opt_sparse_item_vec_t;


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
	datadesc dl, dr;

	if (!par_int || par_int->type != block_type_con_par_int)
		return 0;
	if (!is_single_datadesc_set(par_int->p.b[0])
	 || !is_single_datadesc_set(par_int->p.b[1]))
		return 0;

	dl = par_int->p.b[0]->p.d->p[0];
	dr = par_int->p.b[1]->p.d->p[0];

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

static int_t find_dense_dim(datadesc d, int_t dim_name)
{
	int_t i;

	for (i=0; i<active_ndim(d); i++)
		if (d->dims[i]->n_tuple == 1 && d->dims[i]->dims[0] == dim_name)
			return i;

	return -1;
}

static int_t find_entry_by_index(dimdesc d, int_t index)
{
	int_t i;

	for (i=0; i<d->n_entry; i++)
		if (d->indices[i] == index)
			return i;

	return -1;
}

static int dense_broadcast_compatible(datadesc K, datadesc X)
{
	int_t i;

	if (!is_dense_datadesc(K) || !is_dense_datadesc(X))
		return 0;

	for (i=0; i<active_ndim(K); i++)
	{
		dimdesc kd = K->dims[i];
		int_t x_dim = find_dense_dim(X, kd->dims[0]);
		int_t j;

		if (x_dim < 0)
			return 0;

		if (kd->n_entry == 1)
			continue;

		if (kd->n_entry != X->dims[x_dim]->n_entry)
			return 0;

		for (j=0; j<kd->n_entry; j++)
			if (find_entry_by_index(X->dims[x_dim], kd->indices[j]) < 0)
				return 0;
	}

	return 1;
}

static double read_direct_real(datadesc d, int_t elem_offset)
{
	char *base = (char *)d->ref.p_direct + d->offset0;

	if (d->elem_size == 4)
		return *(float *)(base + elem_offset * d->elem_size);
	return *(double *)(base + elem_offset * d->elem_size);
}

static void item_vec_init(opt_sparse_item_vec_t *v)
{
	v->n = 0;
	v->cap = 32;
	v->p = malloc(sizeof(opt_sparse_item_t) * v->cap);
}

static void item_vec_push(opt_sparse_item_vec_t *v, int_t k_offset, int_t x_offset,
                          int_t out_offset, double k_value)
{
	if (v->n == v->cap)
	{
		opt_sparse_item_t *p;

		v->cap *= 2;
		p = malloc(sizeof(opt_sparse_item_t) * v->cap);
		memcpy(p, v->p, sizeof(opt_sparse_item_t) * v->n);
		v->p = p;
	}

	v->p[v->n].k_offset = k_offset;
	v->p[v->n].x_offset = x_offset;
	v->p[v->n].out_offset = out_offset;
	v->p[v->n].k_value = k_value;
	v->n++;
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

static int collect_dense_items(datadesc K, datadesc X, enum_block_type reduce_type,
                               opt_sparse_item_vec_t *items)
{
	int done = 0;
	int_t x_ndim = active_ndim(X);
	int_t *pos;
	int_t i;

	if (!dense_broadcast_compatible(K, X))
		return 0;

	pos = malloc(sizeof(int_t) * x_ndim);
	for (i=0; i<x_ndim; i++)
		pos[i] = 0;

	while (!done)
	{
		double kv;
		int_t k_offset = 0;
		int_t x_offset = 0;

		for (i=0; i<x_ndim; i++)
		{
			int_t dim_name = X->dims[i]->dims[0];
			int_t x_index = X->dims[i]->indices[pos[i]];
			int_t k_dim = find_dense_dim(K, dim_name);

			x_offset += X->dims[i]->offsets[pos[i]];

			if (k_dim >= 0)
			{
				int_t k_entry;

				if (K->dims[k_dim]->n_entry == 1)
					k_entry = 0;
				else
					k_entry = find_entry_by_index(K->dims[k_dim], x_index);

				if (k_entry < 0)
					return 0;

				k_offset += K->dims[k_dim]->offsets[k_entry];
			}
		}

		kv = read_direct_real(K, k_offset);
		if (kv != 0.0)
			item_vec_push(items, k_offset, x_offset, x_offset, kv);

		(void)reduce_type;
		increment_positions(x_ndim, pos, X, &done);
	}

	return 1;
}

static void *make_packed_direct_values(datadesc K, opt_sparse_item_vec_t *items)
{
	int_t i;
	int_t n = items->n > 0 ? items->n : 1;

	if (K->elem_size == 4)
	{
		float *p = malloc(sizeof(float) * n);

		for (i=0; i<items->n; i++)
			p[i] = (float)items->p[i].k_value;
		return p;
	}
	else
	{
		double *p = malloc(sizeof(double) * n);

		for (i=0; i<items->n; i++)
			p[i] = items->p[i].k_value;
		return p;
	}
}

static dimdesc make_1d_dim(int_t dim_name, int_t n, int_t *offsets)
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

static datadesc make_K_view(datadesc K, opt_sparse_item_vec_t *items, int_t dim_name)
{
	datadesc d;

	d = datadesc_new(1);
	d->region = region_direct;
	d->ref.p_direct = make_packed_direct_values(K, items);
	d->offset0 = 0;
	d->endian = K->endian;
	d->elem_type = K->elem_type;
	d->elem_size = K->elem_size;
	d->n_dim = 1;
	d->dims[0] = make_1d_dim(dim_name, items->n, NULL);

	return d;
}

static datadesc make_X_view(datadesc X, opt_sparse_item_vec_t *items, int_t dim_name)
{
	datadesc d;
	int_t i;
	int_t min_offset = 0;
	int_t *offsets = NULL;

	if (items->n > 0)
	{
		min_offset = items->p[0].x_offset;
		for (i=1; i<items->n; i++)
			if (items->p[i].x_offset < min_offset)
				min_offset = items->p[i].x_offset;

		offsets = malloc(sizeof(int_t) * items->n);
		for (i=0; i<items->n; i++)
			offsets[i] = items->p[i].x_offset - min_offset;
	}

	d = datadesc_new(1);
	d->region = X->region;
	d->ref = X->ref;
	d->offset0 = X->offset0 + min_offset * X->elem_size;
	d->endian = X->endian;
	d->elem_type = X->elem_type;
	d->elem_size = X->elem_size;
	d->n_dim = 1;
	d->dims[0] = make_1d_dim(dim_name, items->n, offsets);

	return d;
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

static datadesc make_offsets_desc(opt_sparse_item_vec_t *items, int_t dim_name)
{
	datadesc d;
	int_t i;
	int_t n = items->n > 0 ? items->n : 1;
	int_t *offsets = malloc(sizeof(int_t) * n);

	for (i=0; i<items->n; i++)
		offsets[i] = items->p[i].out_offset;

	d = datadesc_new(1);
	d->region = region_direct;
	d->ref.p_direct = offsets;
	d->offset0 = 0;
	d->endian = endian_little;
	d->elem_type = elem_type_integer;
	d->elem_size = sizeof(int_t);
	d->n_dim = 1;
	d->dims[0] = make_1d_dim(dim_name, items->n, NULL);

	return d;
}

static block make_sparse_optimized_block(enum_block_type reduce_type,
                                         datadesc K_view, datadesc X_view,
                                         datadesc out_template,
                                         datadesc offsets_desc,
                                         datadesc X_original)
{
	datadesc_set sK, sX, meta;
	block bK, bX, packed_par, meta_b, tail, inner;

	sK = datadesc_set_new(1);
	sX = datadesc_set_new(1);
	sK->p[0] = K_view;
	sX->p[0] = X_view;

	meta = datadesc_set_new(3);
	meta->p[0] = out_template;
	meta->p[1] = offsets_desc;
	meta->p[2] = X_original;

	bK = block_new(block_type_datadesc_set, sK);
	bX = block_new(block_type_datadesc_set, sX);
	packed_par = block_new(block_type_con_par_int, bK, bX);
	meta_b = block_new(block_type_datadesc_set, meta);
	tail = block_new(block_type_con_par_int, packed_par, meta_b);
	inner = block_new(block_type_con_ser, block_new(reduce_type), tail);

	return block_new(block_type_box, inner);
}

static int decode_sparse_optimized_block(block ob, enum_block_type *reduce_type,
                                         block *packed_par,
                                         datadesc *K_view, datadesc *X_view,
                                         datadesc *out_template,
                                         datadesc *offsets_desc,
                                         datadesc *X_original)
{
	block inner, tail, meta_b;

	if (!ob || ob->type != block_type_box)
		return 0;

	inner = ob->p.u;
	if (!inner
	 || inner->type != block_type_con_ser
	 || !inner->p.b[0]
	 || (inner->p.b[0]->type != block_builtin_reduce_prod
	  && inner->p.b[0]->type != block_builtin_reduce_sum)
	 || !inner->p.b[1]
	 || inner->p.b[1]->type != block_type_con_par_int)
		return 0;

	tail = inner->p.b[1];
	if (!tail->p.b[0]
	 || tail->p.b[0]->type != block_type_con_par_int
	 || !is_single_datadesc_set(tail->p.b[0]->p.b[0])
	 || !is_single_datadesc_set(tail->p.b[0]->p.b[1]))
		return 0;

	meta_b = tail->p.b[1];
	if (!meta_b
	 || meta_b->type != block_type_datadesc_set
	 || !meta_b->p.d
	 || meta_b->p.d->n != 3
	 || !meta_b->p.d->p[0]
	 || !meta_b->p.d->p[1]
	 || !meta_b->p.d->p[2])
		return 0;

	*reduce_type = inner->p.b[0]->type;
	*packed_par = tail->p.b[0];
	*K_view = tail->p.b[0]->p.b[0]->p.d->p[0];
	*X_view = tail->p.b[0]->p.b[1]->p.d->p[0];
	*out_template = meta_b->p.d->p[0];
	*offsets_desc = meta_b->p.d->p[1];
	*X_original = meta_b->p.d->p[2];

	return 1;
}

static int_t data_span_bytes(datadesc d)
{
	int_t i, j;
	int_t max_offset = 0;
	int_t ndim = active_ndim(d);

	if (ndim == 0)
		return d->elem_size;

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

static int_t offset_count(datadesc offsets_desc)
{
	if (!offsets_desc
	 || active_ndim(offsets_desc) != 1
	 || !offsets_desc->dims[0])
		return 0;

	return offsets_desc->dims[0]->n_entry;
}

static int offsets_are_identity(datadesc offsets_desc)
{
	int_t i;
	int_t n = offset_count(offsets_desc);
	int_t *offsets = (int_t *)offsets_desc->ref.p_direct;

	for (i=0; i<n; i++)
		if (offsets[i] != i)
			return 0;

	return 1;
}

block optimize_sparse_const(target t, block ob)
{
	datadesc K = NULL;
	datadesc X = NULL;
	datadesc K_view, X_view, out_template, offsets_desc;
	opt_sparse_item_vec_t items;
	int_t dim_name;
	enum_block_type reduce_type;

	(void)t;

	if (ob && ob->type == block_type_box)
		return ob;

	if (!ob
	 || ob->type != block_type_con_ser
	 || !ob->p.b[0]
	 || (ob->p.b[0]->type != block_builtin_reduce_prod
	  && ob->p.b[0]->type != block_builtin_reduce_sum)
	 || !try_extract_K_X(ob->p.b[1], &K, &X))
		return ob;

	reduce_type = ob->p.b[0]->type;

	if (K->elem_type != elem_type_real
	 || X->elem_type != elem_type_real
	 || (K->elem_size != 4 && K->elem_size != 8)
	 || K->elem_size != X->elem_size
	 || X->region != region_main
	 || !is_dense_datadesc(K)
	 || !is_dense_datadesc(X))
		return ob;

	item_vec_init(&items);
	if (!collect_dense_items(K, X, reduce_type, &items))
		return ob;

	dim_name = dimtmp_new();
	K_view = make_K_view(K, &items, dim_name);
	X_view = make_X_view(X, &items, dim_name);
	out_template = make_output_desc(X);
	offsets_desc = make_offsets_desc(&items, dim_name);

	fprintf(stderr, "SPARSE OPTIMIZE PASS HIT\n");
	return make_sparse_optimized_block(reduce_type, K_view, X_view,
	                                   out_template, offsets_desc, X);
}

datadesc_set block2data_sparse_const_optimized(target t, block ob)
{
	enum_block_type reduce_type;
	block packed_par;
	datadesc K_view, X_view, out_template, offsets_desc, X_original;
	datadesc out;
	datadesc_set tmp, res;
	block packed_ir;
	char *type_str;
	char *tmp_var;
	char *out_var;
	int_t n;
	int_t nbytes;
	int_t i;
	int_t *offsets;

	if (!decode_sparse_optimized_block(ob, &reduce_type, &packed_par,
	                                   &K_view, &X_view, &out_template,
	                                   &offsets_desc, &X_original))
		return 0;

	emit_start(t);

	emit_assert(K_view->elem_type == elem_type_real);
	emit_assert(X_view->elem_type == elem_type_real);
	emit_assert(K_view->elem_size == X_view->elem_size);
	emit_assert(X_original->region == region_main);

	type_str = target_query_type(t, out_template->elem_type, out_template->elem_size);
	emit_assert(type_str);

	out = make_output_desc(out_template);
	nbytes = data_span_bytes(out);
	emit_push(type_str, "*", out->ref.p_refname, "=(", type_str, "*)malloc(",
	          mt_itoa(nbytes), ");");

	if (reduce_type == block_builtin_reduce_prod)
	{
		emit_push("memset(", out->ref.p_refname, ",0,", mt_itoa(nbytes), ");");
	}
	else
	{
		emit_push("memcpy(", out->ref.p_refname, ",(char*)(",
		          X_original->ref.p_refname, ")+", mt_itoa(X_original->offset0),
		          ",", mt_itoa(nbytes), ");");
	}

	n = offset_count(offsets_desc);
	if (n > 0)
	{
		packed_ir = block_new(block_type_con_ser, block_new(reduce_type), packed_par);
		tmp = block2data_arithmetic(t, packed_ir);
		emit_assert(tmp && tmp->n == 1 && tmp->p[0]);

		tmp_var = token_new();
		out_var = token_new();
		emit_push("{", type_str, "*", tmp_var, "=(", type_str, "*)((char*)(",
		          tmp->p[0]->ref.p_refname, ")+", mt_itoa(tmp->p[0]->offset0), ");");
		emit_push(type_str, "*", out_var, "=(", type_str, "*)((char*)(",
		          out->ref.p_refname, ")+", mt_itoa(out->offset0), ");");

		if (offsets_are_identity(offsets_desc))
		{
			emit_push("memcpy(", out_var, ",", tmp_var, ",",
			          mt_itoa(n * out->elem_size), ");");
		}
		else
		{
			char *offset_arr = token_new();
			char *idx = token_new();

			offsets = (int_t *)offsets_desc->ref.p_direct;
			emit_push("const int32_t ", offset_arr, "[", mt_itoa(n), "]={");
			for (i=0; i<n; i++)
			{
				if (i > 0)
					emit_push(",");
				emit_push(mt_itoa(offsets[i]));
			}
			emit_push("};");
			emit_push("for(int32_t ", idx, "=0;", idx, "<", mt_itoa(n), ";",
			          idx, "++){", out_var, "[", offset_arr, "[", idx, "]]=",
			          tmp_var, "[", idx, "];}");
		}

		emit_push("free(", tmp->p[0]->ref.p_refname, ");}");
	}

	emit_finish();

	res = datadesc_set_new(1);
	res->p[0] = out;
	return res;
}
