#include	<stdio.h>
#include	<string.h>

#include	"plugins.h"
#include	"gc_malloc.h"


typedef struct
{
	int_t k_offset;
	int_t x_offset;
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

static int dense_layout_matches_by_name(datadesc K, datadesc X)
{
	int_t i;

	if (!is_dense_datadesc(K) || !is_dense_datadesc(X))
		return 0;
	if (active_ndim(K) != active_ndim(X))
		return 0;

	for (i=0; i<active_ndim(X); i++)
	{
		int_t j;
		int_t k_dim = find_dense_dim(K, X->dims[i]->dims[0]);

		if (k_dim < 0 || K->dims[k_dim]->n_entry != X->dims[i]->n_entry)
			return 0;

		for (j=0; j<X->dims[i]->n_entry; j++)
			if (find_entry_by_index(K->dims[k_dim], X->dims[i]->indices[j]) < 0)
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
                          double k_value)
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

static int collect_dense_prod_items(datadesc K, datadesc X, opt_sparse_item_vec_t *items)
{
	int done = 0;
	int_t x_ndim = active_ndim(X);
	int_t *pos;
	int_t i;

	if (!dense_layout_matches_by_name(K, X))
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
			int_t k_dim = find_dense_dim(K, dim_name);
			int_t x_index = X->dims[i]->indices[pos[i]];
			int_t k_entry = find_entry_by_index(K->dims[k_dim], x_index);

			if (k_entry < 0)
				return 0;

			k_offset += K->dims[k_dim]->offsets[k_entry];
			x_offset += X->dims[i]->offsets[pos[i]];
		}

		kv = read_direct_real(K, k_offset);
		if (kv != 0.0)
			item_vec_push(items, k_offset, x_offset, kv);

		increment_positions(x_ndim, pos, X, &done);
	}

	return 1;
}

static void *make_packed_direct_values(datadesc K, opt_sparse_item_vec_t *items)
{
	int_t i;

	if (K->elem_size == 4)
	{
		float *p = malloc(sizeof(float) * items->n);

		for (i=0; i<items->n; i++)
			p[i] = (float)items->p[i].k_value;
		return p;
	}
	else
	{
		double *p = malloc(sizeof(double) * items->n);

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
	int_t min_offset;
	int_t *offsets;

	min_offset = items->p[0].x_offset;
	for (i=1; i<items->n; i++)
		if (items->p[i].x_offset < min_offset)
			min_offset = items->p[i].x_offset;

	offsets = malloc(sizeof(int_t) * items->n);
	for (i=0; i<items->n; i++)
		offsets[i] = items->p[i].x_offset - min_offset;

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

block optimize_sparse_const(target t, block ob)
{
	datadesc K = NULL;
	datadesc X = NULL;
	datadesc K_view, X_view;
	datadesc_set sK, sX;
	opt_sparse_item_vec_t items;
	int_t dim_name;
	block bK, bX, par;

	(void)t;

	if (!ob
	 || ob->type != block_type_con_ser
	 || !ob->p.b[0]
	 || ob->p.b[0]->type != block_builtin_reduce_prod
	 || !try_extract_K_X(ob->p.b[1], &K, &X))
		return ob;

	if (K->elem_type != elem_type_real
	 || X->elem_type != elem_type_real
	 || (K->elem_size != 4 && K->elem_size != 8)
	 || K->elem_size != X->elem_size
	 || !is_dense_datadesc(K)
	 || !is_dense_datadesc(X))
		return ob;

	item_vec_init(&items);
	if (!collect_dense_prod_items(K, X, &items) || items.n == 0)
		return ob;

	dim_name = dimtmp_new();
	K_view = make_K_view(K, &items, dim_name);
	X_view = make_X_view(X, &items, dim_name);

	sK = datadesc_set_new(1);
	sX = datadesc_set_new(1);
	sK->p[0] = K_view;
	sX->p[0] = X_view;

	bK = block_new(block_type_datadesc_set, sK);
	bX = block_new(block_type_datadesc_set, sX);
	par = block_new(block_type_con_par_int, bK, bX);

	fprintf(stderr, "SPARSE OPTIMIZE PASS HIT\n");
	return block_new(block_type_con_ser, block_new(block_builtin_reduce_prod), par);
}
