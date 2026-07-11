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

static int same_1d_indices(datadesc K, datadesc X)
{
	int_t i;
	dimdesc dk = K->dims[0];
	dimdesc dx = X->dims[0];

	if (dk->dims[0] != dx->dims[0])
		return 0;
	if (dk->n_entry != dx->n_entry)
		return 0;

	for (i=0; i<dk->n_entry; i++)
		if (dk->indices[i] != dx->indices[i])
			return 0;

	return 1;
}

static float read_K_value(datadesc K, int_t k)
{
	char *base = (char *)K->ref.p_direct + K->offset0;
	int_t off = K->dims[0]->offsets[k] * K->elem_size;
	return *(float *)(base + off);
}

static int_t dense_bytes(datadesc d)
{
	int_t i;
	int_t max_offset = 0;

	for (i=0; i<d->dims[0]->n_entry; i++)
		if (d->dims[0]->offsets[i] > max_offset)
			max_offset = d->dims[0]->offsets[i];

	return (max_offset + 1) * d->elem_size;
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
	int_t N;
	int_t nbytes;

	if (!ob
	 || ob->type != block_type_con_ser
	 || !ob->p.b[0]
	 || ob->p.b[0]->type != block_builtin_reduce_prod
	 || !try_extract_K_X(ob->p.b[1], &K, &X))
		return 0;

	emit_start(t);

	fprintf(stderr, "SPARSE PATH HIT\n");

	emit_assert(K->elem_type == elem_type_real && K->elem_size == 4);
	emit_assert(X->elem_type == elem_type_real && X->elem_size == 4);
	emit_assert(K->n_dim == 1 && X->n_dim == 1);
	emit_assert(K->dims[0] && X->dims[0]);
	emit_assert(K->dims[0]->n_tuple == 1 && X->dims[0]->n_tuple == 1);
	emit_assert(same_1d_indices(K, X));

	N = K->dims[0]->n_entry;
	nbytes = dense_bytes(X);
	type_str = target_query_type(t, elem_type_real, 4);
	emit_assert(type_str);

	out = datadesc_new(1);
	out->region = region_main;
	out->ref.p_refname = token_new();
	out->offset0 = 0;
	out->endian = X->endian;
	out->elem_type = elem_type_real;
	out->elem_size = 4;
	out->n_dim = 1;
	out->dims[0] = dimdesc_copy(X->dims[0]);

	emit_push(type_str, "*", out->ref.p_refname,
	          "=(", type_str, "*)malloc(", mt_itoa(nbytes), ");");
	emit_push("memset(", out->ref.p_refname, ",0,", mt_itoa(nbytes), ");");
	emit_push("{");

	X_var = token_new();
	out_var = token_new();
	emit_push(type_str, "*", X_var, "=(", type_str, "*)((char*)(",
	          X->ref.p_refname, ")+", mt_itoa(X->offset0), ");");
	emit_push(type_str, "*", out_var, "=(", type_str, "*)((char*)(",
	          out->ref.p_refname, ")+", mt_itoa(out->offset0), ");");

	for (i=0; i<N; i++)
	{
		float kv = read_K_value(K, i);
		int_t x_idx;
		int_t out_idx;

		if (kv == 0.0f)
			continue;

		x_idx = X->dims[0]->offsets[i];
		out_idx = out->dims[0]->offsets[i];

		if (kv == 1.0f)
		{
			emit_push(out_var, "[", mt_itoa(out_idx), "]=",
			          X_var, "[", mt_itoa(x_idx), "];");
		}
		else
		{
			char *kv_str = malloc(32);
			snprintf(kv_str, 32, "%.9gf", kv);
			emit_push(out_var, "[", mt_itoa(out_idx), "]=",
			          kv_str, "*", X_var, "[", mt_itoa(x_idx), "];");
		}
	}

	emit_push("}");
	emit_finish();

	res = datadesc_set_new(1);
	res->p[0] = out;
	return res;
}
