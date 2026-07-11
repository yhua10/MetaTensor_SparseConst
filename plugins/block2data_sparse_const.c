#include	<stdio.h>
#include	<string.h>

#include	"plugins.h"
#include	"gc_malloc.h"


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

	for (i=0; i<d->n_dim && d->dims[i]; i++);
	return i;
}

static int is_dense_datadesc(datadesc d)
{
	int_t i;
	int_t ndim;

	if (!d)
		return 0;

	ndim = active_ndim(d);
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

static int same_dim_entries(dimdesc a, dimdesc b)
{
	int_t i;

	if (!a || !b || a->n_tuple != 1 || b->n_tuple != 1)
		return 0;
	if (a->dims[0] != b->dims[0])
		return 0;
	if (a->n_entry != b->n_entry)
		return 0;

	for (i=0; i<a->n_entry; i++)
		if (a->indices[i] != b->indices[i])
			return 0;

	return 1;
}

static int same_dense_layout(datadesc K, datadesc X)
{
	int_t i;
	int_t ndim = active_ndim(K);

	if (ndim != active_ndim(X))
		return 0;
	if (!is_dense_datadesc(K) || !is_dense_datadesc(X))
		return 0;

	for (i=0; i<ndim; i++)
		if (!same_dim_entries(K->dims[i], X->dims[i]))
			return 0;

	return 1;
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

static int_t find_entry_by_index(dimdesc d, int_t index)
{
	int_t i;

	for (i=0; i<d->n_entry; i++)
		if (d->indices[i] == index)
			return i;

	return -1;
}

static int sparse_tuple_matches_dense(datadesc K, datadesc X)
{
	int_t i;
	dimdesc kd;

	if (!is_sparse_tuple_datadesc(K) || !is_dense_datadesc(X))
		return 0;

	kd = K->dims[0];
	if (kd->n_tuple != active_ndim(X))
		return 0;

	for (i=0; i<kd->n_tuple; i++)
		if (find_dense_dim(X, kd->dims[i]) < 0)
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
	char *res = malloc(64);

	if (K->elem_size == 4)
		snprintf(res, 64, "%.9ef", (float)value);
	else
		snprintf(res, 64, "%.17e", value);

	return res;
}

static int_t dense_bytes(datadesc d)
{
	int_t i, j;
	int_t max_offset = 0;
	int_t ndim = active_ndim(d);

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

datadesc_set	block2data_sparse_const(target t, block ob)
{
	datadesc K = NULL;
	datadesc X = NULL;
	datadesc out;
	datadesc_set res;
	char *type_str;
	char *X_var;
	char *out_var;
	int_t i;
	int_t nbytes;
	int_t x_ndim;
	int K_dense;
	int K_sparse_tuple;
	enum_block_type reduce_type;

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
	emit_assert(is_dense_datadesc(X));

	K_dense = is_dense_datadesc(K);
	K_sparse_tuple = is_sparse_tuple_datadesc(K);
	emit_assert(K_dense || K_sparse_tuple);
	emit_assert((K_dense && same_dense_layout(K, X))
	         || (K_sparse_tuple && sparse_tuple_matches_dense(K, X)));

	nbytes = dense_bytes(X);
	x_ndim = active_ndim(X);
	type_str = target_query_type(t, elem_type_real, K->elem_size);
	emit_assert(type_str);

	out = datadesc_new(x_ndim);
	out->region = region_main;
	out->ref.p_refname = token_new();
	out->offset0 = 0;
	out->endian = X->endian;
	out->elem_type = elem_type_real;
	out->elem_size = K->elem_size;
	out->n_dim = x_ndim;
	for (i=0; i<x_ndim; i++)
		out->dims[i] = dimdesc_copy(X->dims[i]);

	emit_push(type_str, "*", out->ref.p_refname,
	          "=(", type_str, "*)malloc(", mt_itoa(nbytes), ");");
	if (reduce_type == block_builtin_reduce_prod)
		emit_push("memset(", out->ref.p_refname, ",0,", mt_itoa(nbytes), ");");
	emit_push("{");

	X_var = token_new();
	out_var = token_new();
	emit_push(type_str, "*", X_var, "=(", type_str, "*)((char*)(",
	          X->ref.p_refname, ")+", mt_itoa(X->offset0), ");");
	emit_push(type_str, "*", out_var, "=(", type_str, "*)((char*)(",
	          out->ref.p_refname, ")+", mt_itoa(out->offset0), ");");

	if (K_sparse_tuple && reduce_type == block_builtin_reduce_sum)
	{
		int done = 0;
		int_t *pos = malloc(sizeof(int_t) * x_ndim);
		int_t j;

		for (j=0; j<x_ndim; j++)
			pos[j] = 0;

		while (!done)
		{
			int_t x_idx = 0;

			for (j=0; j<x_ndim; j++)
				x_idx += X->dims[j]->offsets[pos[j]];

			emit_push(out_var, "[", mt_itoa(x_idx), "]=",
			          X_var, "[", mt_itoa(x_idx), "];");
			increment_positions(x_ndim, pos, X, &done);
		}
	}

	if (K_dense)
	{
		int done = 0;
		int_t *pos = malloc(sizeof(int_t) * x_ndim);
		int_t j;

		for (j=0; j<x_ndim; j++)
			pos[j] = 0;

		while (!done)
		{
			double kv;
			int_t k_idx = 0;
			int_t x_idx = 0;

			for (j=0; j<x_ndim; j++)
			{
				k_idx += K->dims[j]->offsets[pos[j]];
				x_idx += X->dims[j]->offsets[pos[j]];
			}

			kv = read_K_value_at_offset(K, k_idx);

			if (!(reduce_type == block_builtin_reduce_prod && kv == 0.0))
			{
				if ((reduce_type == block_builtin_reduce_prod && kv == 1.0)
				 || (reduce_type == block_builtin_reduce_sum && kv == 0.0))
				{
					emit_push(out_var, "[", mt_itoa(x_idx), "]=",
					          X_var, "[", mt_itoa(x_idx), "];");
				}
				else if (reduce_type == block_builtin_reduce_sum)
				{
					emit_push(out_var, "[", mt_itoa(x_idx), "]=",
					          literal_real(K, kv), "+", X_var, "[",
					          mt_itoa(x_idx), "];");
				}
				else
				{
					emit_push(out_var, "[", mt_itoa(x_idx), "]=",
					          literal_real(K, kv), "*", X_var, "[",
					          mt_itoa(x_idx), "];");
				}
			}

			increment_positions(x_ndim, pos, X, &done);
		}
	}
	else
	{
		dimdesc kd = K->dims[0];

		for (i=0; i<kd->n_entry; i++)
		{
			double kv = read_K_value_at_offset(K, kd->offsets[i]);
			int_t x_idx;

			if (reduce_type == block_builtin_reduce_prod && kv == 0.0)
				continue;
			if (reduce_type == block_builtin_reduce_sum && kv == 0.0)
				continue;

			emit_assert(tuple_entry_to_dense_offset(K, i, X, &x_idx));

			if (reduce_type == block_builtin_reduce_prod && kv == 1.0)
			{
				emit_push(out_var, "[", mt_itoa(x_idx), "]=",
				          X_var, "[", mt_itoa(x_idx), "];");
			}
			else if (reduce_type == block_builtin_reduce_sum)
			{
				emit_push(out_var, "[", mt_itoa(x_idx), "]=",
				          literal_real(K, kv), "+", X_var, "[",
				          mt_itoa(x_idx), "];");
			}
			else
			{
				emit_push(out_var, "[", mt_itoa(x_idx), "]=",
				          literal_real(K, kv), "*", X_var, "[",
				          mt_itoa(x_idx), "];");
			}
		}
	}

	emit_push("}");
	emit_finish();

	res = datadesc_set_new(1);
	res->p[0] = out;
	return res;
}
