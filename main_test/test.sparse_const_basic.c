#include	<stdio.h>
#include	<string.h>

#include	"plugins.h"
#include	"target_c99.h"


int main(void)
{
	enum { N = 16 };
	static float K_data[N] = {
		0.5f, 0.0f, 0.0f, 1.2f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, -0.3f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
	};
	target t;
	datadesc K, X;
	datadesc_set sK, sX, sc;
	block bK, bX, ca, cb;
	FILE *fp;
	char *dimx[] = {"i"};
	int_t lenx[] = {N};
	int_t k;

	K = datadesc_new(1);
	K->region = region_direct;
	K->ref.p_direct = K_data;
	K->offset0 = 0;
	K->endian = endian_little;
	K->elem_type = elem_type_real;
	K->elem_size = 4;
	K->n_dim = 1;
	K->dims[0] = dimdesc_new(1, N);
	K->dims[0]->reduce_type = reduce_type_none;
	K->dims[0]->dims[0] = dimname_new("i");
	for (k=0; k<N; k++)
	{
		K->dims[0]->offsets[k] = k;
		K->dims[0]->indices[k] = k;
	}

	X = datadesc_new_array(region_main, "input_x",
	                       endian_little, elem_type_real, 4,
	                       1, dimx, lenx);

	sK = datadesc_set_new(1);
	sK->p[0] = K;
	sX = datadesc_set_new(1);
	sX->p[0] = X;

	bK = block_new(block_type_datadesc_set, sK);
	bX = block_new(block_type_datadesc_set, sX);
	ca = block_new(block_type_con_par_int, bK, bX);
	cb = block_new(block_type_con_ser, block_new(block_builtin_reduce_prod), ca);

	t = target_c99_new();
	sc = block2data(t, cb);
	if (!sc)
	{
		fprintf(stderr, "sparse_const plugin failed\n");
		return 1;
	}

	fp = fopen("dump_sparse.c", "wt");
	if (!fp)
	{
		fprintf(stderr, "failed to open dump_sparse.c\n");
		return 1;
	}

	fputs("#include <stdint.h>\n"
	      "#include <stdlib.h>\n"
	      "#include <string.h>\n"
	      "void compute_sparse(float *input_x, float *out) {\n", fp);
	target_c99_dump(t, fp);
	fprintf(fp, "memcpy(out,%s,%d);free(%s);\n",
	        sc->p[0]->ref.p_refname, N * 4, sc->p[0]->ref.p_refname);
	fputs("}\n", fp);
	fclose(fp);

	printf("OK; dump written to dump_sparse.c\n");
	return 0;
}
