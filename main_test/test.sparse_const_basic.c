#include	<stdio.h>
#include	<string.h>

#include	"plugins.h"
#include	"target_c99.h"


enum {
	N = 16,
	ROWS = 3,
	COLS = 4,
	N2 = ROWS * COLS,
	BIG = 96,
	BLOCK = 32,
	XTUP = 4,
};

static float K_float[N] = {
	0.5f, 0.0f, 0.0f, 1.2f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, -0.3f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.7f, 0.0f,
};

static double K_double[N] = {
	0.25, 0.0, 0.0, -2.5,
	0.0, 1.0, 0.0, 0.0,
	3.125, 0.0, 0.0, 0.0,
	0.0, 0.0, -0.75, 0.0,
};

static float K_2d[N2] = {
	0.5f, 0.0f, 0.0f, 1.0f,
	0.0f, -1.5f, 0.0f, 0.0f,
	2.0f, 0.0f, 0.0f, -0.25f,
};

static float K_tuple_values[] = {
	0.5f, 1.0f, -0.75f,
};

static float K_ir[BIG];
static float K_block[BLOCK];

static void fill_extra_constants(void)
{
	int i;

	for (i=0; i<BIG; i++)
	{
		if ((i + 1) % 8 == 0)
			K_ir[i] = 0.0f;
		else
			K_ir[i] = (float)((i % 9) + 1) * 0.125f;
	}

	for (i=0; i<BLOCK; i++)
	{
		if (i < 20)
			K_block[i] = (float)((i % 5) + 1) * 0.2f;
		else
			K_block[i] = 0.0f;
	}
}

static int_t data_bytes(datadesc d)
{
	int_t i, j;
	int_t max_offset = 0;

	for (i=0; i<d->n_dim && d->dims[i]; i++)
	{
		int_t dim_max = 0;

		for (j=0; j<d->dims[i]->n_entry; j++)
			if (d->dims[i]->offsets[j] > dim_max)
				dim_max = d->dims[i]->offsets[j];

		max_offset += dim_max;
	}

	return (max_offset + 1) * d->elem_size;
}

static datadesc make_direct_real_len(void *data, int_t elem_size, int_t n,
                                     const char *dim_name)
{
	datadesc K;
	int_t k;

	K = datadesc_new(1);
	K->region = region_direct;
	K->ref.p_direct = data;
	K->offset0 = 0;
	K->endian = endian_little;
	K->elem_type = elem_type_real;
	K->elem_size = elem_size;
	K->n_dim = 1;
	K->dims[0] = dimdesc_new(1, n);
	K->dims[0]->reduce_type = reduce_type_none;
	K->dims[0]->dims[0] = dimname_new(dim_name);
	for (k=0; k<n; k++)
	{
		K->dims[0]->offsets[k] = k;
		K->dims[0]->indices[k] = k;
	}

	return K;
}

static datadesc make_direct_real(void *data, int_t elem_size)
{
	return make_direct_real_len(data, elem_size, N, "i");
}

static datadesc make_runtime_real_len(char *name, int_t elem_size, int_t n,
                                      char *dim_name)
{
	char *dimx[1];
	int_t lenx[1];

	dimx[0] = dim_name;
	lenx[0] = n;

	return datadesc_new_array(region_main, name,
	                          endian_little, elem_type_real, elem_size,
	                          1, dimx, lenx);
}

static datadesc make_runtime_real(char *name, int_t elem_size)
{
	return make_runtime_real_len(name, elem_size, N, "i");
}

static datadesc make_direct_2d(float *data)
{
	char *dims[] = {"row", "col"};
	int_t lens[] = {ROWS, COLS};

	return datadesc_new_array(region_direct, data,
	                          endian_little, elem_type_real, 4,
	                          2, dims, lens);
}

static datadesc make_runtime_2d(char *name)
{
	char *dims[] = {"row", "col"};
	int_t lens[] = {ROWS, COLS};

	return datadesc_new_array(region_main, name,
	                          endian_little, elem_type_real, 4,
	                          2, dims, lens);
}

static datadesc make_direct_tuple(float *data)
{
	datadesc K;

	K = datadesc_new(1);
	K->region = region_direct;
	K->ref.p_direct = data;
	K->offset0 = 0;
	K->endian = endian_little;
	K->elem_type = elem_type_real;
	K->elem_size = 4;
	K->n_dim = 1;
	K->dims[0] = dimdesc_new(2, 3);
	K->dims[0]->reduce_type = reduce_type_none;
	K->dims[0]->dims[0] = dimname_new("row");
	K->dims[0]->dims[1] = dimname_new("col");
	K->dims[0]->offsets[0] = 0;
	K->dims[0]->indices[0] = 0;
	K->dims[0]->indices[1] = 0;
	K->dims[0]->offsets[1] = 1;
	K->dims[0]->indices[2] = 1;
	K->dims[0]->indices[3] = 2;
	K->dims[0]->offsets[2] = 2;
	K->dims[0]->indices[4] = 2;
	K->dims[0]->indices[5] = 3;

	return K;
}

static datadesc make_runtime_tuple(char *name)
{
	datadesc X;

	X = datadesc_new(1);
	X->region = region_main;
	X->ref.p_refname = name;
	X->offset0 = 0;
	X->endian = endian_little;
	X->elem_type = elem_type_real;
	X->elem_size = 4;
	X->n_dim = 1;
	X->dims[0] = dimdesc_new(2, XTUP);
	X->dims[0]->reduce_type = reduce_type_none;
	X->dims[0]->dims[0] = dimname_new("row");
	X->dims[0]->dims[1] = dimname_new("col");

	X->dims[0]->offsets[0] = 0;
	X->dims[0]->indices[0] = 0;
	X->dims[0]->indices[1] = 0;
	X->dims[0]->offsets[1] = 1;
	X->dims[0]->indices[2] = 0;
	X->dims[0]->indices[3] = 1;
	X->dims[0]->offsets[2] = 2;
	X->dims[0]->indices[4] = 1;
	X->dims[0]->indices[5] = 2;
	X->dims[0]->offsets[3] = 3;
	X->dims[0]->indices[6] = 2;
	X->dims[0]->indices[7] = 3;

	return X;
}

static block make_sparse_block(datadesc K, datadesc X, enum_block_type reduce_type, int reverse)
{
	datadesc_set sK, sX;
	block bK, bX, par;

	sK = datadesc_set_new(1);
	sK->p[0] = K;
	sX = datadesc_set_new(1);
	sX->p[0] = X;

	bK = block_new(block_type_datadesc_set, sK);
	bX = block_new(block_type_datadesc_set, sX);
	if (reverse)
		par = block_new(block_type_con_par_int, bX, bK);
	else
		par = block_new(block_type_con_par_int, bK, bX);

	return block_new(block_type_con_ser, block_new(reduce_type), par);
}

static int dump_desc_case(const char *path, const char *function_name, const char *ctype,
                          enum_block_type reduce_type, int reverse, datadesc K, datadesc X)
{
	target t;
	datadesc_set sc;
	block ob;
	FILE *fp;

	ob = make_sparse_block(K, X, reduce_type, reverse);

	t = target_c99_new();
	sc = block2data(t, ob);
	if (!sc)
	{
		fprintf(stderr, "sparse_const plugin failed for %s\n", function_name);
		return 1;
	}

	fp = fopen(path, "wt");
	if (!fp)
	{
		fprintf(stderr, "failed to open %s\n", path);
		return 1;
	}

	fprintf(fp,
	        "#include <stdint.h>\n"
	        "#include <stdlib.h>\n"
	        "#include <string.h>\n"
	        "#if defined(__AVX2__)\n"
	        "#include <immintrin.h>\n"
	        "#endif\n"
	        "void %s(%s *input_x, %s *out) {\n",
	        function_name, ctype, ctype);
	target_c99_dump(t, fp);
	fprintf(fp, "memcpy(out,%s,%ld);free(%s);\n",
	        sc->p[0]->ref.p_refname, (long)data_bytes(sc->p[0]), sc->p[0]->ref.p_refname);
	fputs("}\n", fp);
	fclose(fp);

	return 0;
}

static int dump_case(const char *path, const char *function_name, const char *ctype,
                     int_t elem_size, enum_block_type reduce_type, int reverse, void *k_data)
{
	return dump_desc_case(path, function_name, ctype, reduce_type, reverse,
	                      make_direct_real(k_data, elem_size),
	                      make_runtime_real("input_x", elem_size));
}

static int dump_case_len(const char *path, const char *function_name, const char *ctype,
                         int_t elem_size, int_t n, char *dim_name,
                         enum_block_type reduce_type, int reverse, void *k_data)
{
	return dump_desc_case(path, function_name, ctype, reduce_type, reverse,
	                      make_direct_real_len(k_data, elem_size, n, dim_name),
	                      make_runtime_real_len("input_x", elem_size, n, dim_name));
}

int main(void)
{
	fill_extra_constants();

	if (dump_case("dump_sparse.c", "compute_sparse", "float",
	              4, block_builtin_reduce_prod, 0, K_float))
		return 1;
	if (dump_case("dump_sparse_reverse.c", "compute_sparse_reverse", "float",
	              4, block_builtin_reduce_prod, 1, K_float))
		return 1;
	if (dump_case("dump_sparse_sum.c", "compute_sparse_sum", "float",
	              4, block_builtin_reduce_sum, 0, K_float))
		return 1;
	if (dump_case("dump_sparse_double.c", "compute_sparse_double", "double",
	              8, block_builtin_reduce_prod, 0, K_double))
		return 1;
	if (dump_desc_case("dump_sparse_2d.c", "compute_sparse_2d", "float",
	                   block_builtin_reduce_prod, 0,
	                   make_direct_2d(K_2d), make_runtime_2d("input_x")))
		return 1;
	if (dump_desc_case("dump_sparse_2d_sum.c", "compute_sparse_2d_sum", "float",
	                   block_builtin_reduce_sum, 0,
	                   make_direct_2d(K_2d), make_runtime_2d("input_x")))
		return 1;
	if (dump_desc_case("dump_sparse_tuple.c", "compute_sparse_tuple", "float",
	                   block_builtin_reduce_prod, 0,
	                   make_direct_tuple(K_tuple_values), make_runtime_2d("input_x")))
		return 1;
	if (dump_desc_case("dump_sparse_tuple_sum.c", "compute_sparse_tuple_sum", "float",
	                   block_builtin_reduce_sum, 0,
	                   make_direct_tuple(K_tuple_values), make_runtime_2d("input_x")))
		return 1;
	if (dump_case_len("dump_sparse_ir.c", "compute_sparse_ir", "float",
	                  4, BIG, "big_i", block_builtin_reduce_prod, 0, K_ir))
		return 1;
	if (dump_case_len("dump_sparse_block.c", "compute_sparse_block", "float",
	                  4, BLOCK, "block_i", block_builtin_reduce_prod, 0, K_block))
		return 1;
	if (dump_desc_case("dump_sparse_x_tuple.c", "compute_sparse_x_tuple", "float",
	                   block_builtin_reduce_prod, 0,
	                   make_direct_2d(K_2d), make_runtime_tuple("input_x")))
		return 1;
	if (dump_desc_case("dump_sparse_dual_tuple.c", "compute_sparse_dual_tuple", "float",
	                   block_builtin_reduce_prod, 0,
	                   make_direct_tuple(K_tuple_values), make_runtime_tuple("input_x")))
		return 1;
	if (dump_desc_case("dump_sparse_dual_tuple_sum.c", "compute_sparse_dual_tuple_sum", "float",
	                   block_builtin_reduce_sum, 0,
	                   make_direct_tuple(K_tuple_values), make_runtime_tuple("input_x")))
		return 1;

	printf("OK; sparse dump files written\n");
	return 0;
}
