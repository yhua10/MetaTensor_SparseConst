#include	<math.h>
#include	<stdio.h>

#define N 16
#define ROWS 3
#define COLS 4
#define N2 (ROWS * COLS)
#define BIG 96
#define BLOCK 32
#define XTUP 4
#define MAX_FLOAT_N BIG
#define OPT_PASS_NNZ 5

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

static float K_tuple_dense[N2] = {
	0.5f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, -0.75f,
};

static float K_ir[BIG];
static float K_block[BLOCK];
static float K_x_tuple_dense[XTUP] = {
	0.5f, 0.0f, 0.0f, -0.25f,
};
static float K_dual_tuple_dense[XTUP] = {
	0.5f, 0.0f, 1.0f, -0.75f,
};

void compute_sparse(float *x, float *out);
void compute_sparse_reverse(float *x, float *out);
void compute_sparse_sum(float *x, float *out);
void compute_sparse_double(double *x, double *out);
void compute_sparse_2d(float *x, float *out);
void compute_sparse_2d_sum(float *x, float *out);
void compute_sparse_tuple(float *x, float *out);
void compute_sparse_tuple_sum(float *x, float *out);
void compute_sparse_ir(float *x, float *out);
void compute_sparse_block(float *x, float *out);
void compute_sparse_x_tuple(float *x, float *out);
void compute_sparse_dual_tuple(float *x, float *out);
void compute_sparse_dual_tuple_sum(float *x, float *out);
void compute_sparse_opt_pass(float *x, float *out);

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

static void compute_prod_float_naive(float *x, float *out)
{
	int i;
	for (i=0; i<N; i++)
		out[i] = K_float[i] * x[i];
}

static void compute_sum_float_naive(float *x, float *out)
{
	int i;
	for (i=0; i<N; i++)
		out[i] = K_float[i] + x[i];
}

static void compute_prod_double_naive(double *x, double *out)
{
	int i;
	for (i=0; i<N; i++)
		out[i] = K_double[i] * x[i];
}

static void compute_prod_2d_naive(float *x, float *out)
{
	int i;
	for (i=0; i<N2; i++)
		out[i] = K_2d[i] * x[i];
}

static void compute_prod_tuple_naive(float *x, float *out)
{
	int i;
	for (i=0; i<N2; i++)
		out[i] = K_tuple_dense[i] * x[i];
}

static void compute_sum_2d_naive(float *x, float *out)
{
	int i;
	for (i=0; i<N2; i++)
		out[i] = K_2d[i] + x[i];
}

static void compute_sum_tuple_naive(float *x, float *out)
{
	int i;
	for (i=0; i<N2; i++)
		out[i] = K_tuple_dense[i] + x[i];
}

static void compute_prod_ir_naive(float *x, float *out)
{
	int i;
	for (i=0; i<BIG; i++)
		out[i] = K_ir[i] * x[i];
}

static void compute_prod_block_naive(float *x, float *out)
{
	int i;
	for (i=0; i<BLOCK; i++)
		out[i] = K_block[i] * x[i];
}

static void compute_prod_x_tuple_naive(float *x, float *out)
{
	int i;
	for (i=0; i<XTUP; i++)
		out[i] = K_x_tuple_dense[i] * x[i];
}

static void compute_prod_dual_tuple_naive(float *x, float *out)
{
	int i;
	for (i=0; i<XTUP; i++)
		out[i] = K_dual_tuple_dense[i] * x[i];
}

static void compute_sum_dual_tuple_naive(float *x, float *out)
{
	int i;
	for (i=0; i<XTUP; i++)
		out[i] = K_dual_tuple_dense[i] + x[i];
}

static void compute_prod_opt_pass_naive(float *x, float *out)
{
	int i;
	int j = 0;

	for (i=0; i<N; i++)
		if (K_float[i] != 0.0f)
			out[j++] = K_float[i] * x[i];
}

static int check_float_case(const char *name, void (*generated)(float *, float *),
                            void (*naive)(float *, float *), float *x, int n)
{
	float out_naive[MAX_FLOAT_N];
	float out_sparse[MAX_FLOAT_N];
	int i;

	naive(x, out_naive);
	generated(x, out_sparse);

	for (i=0; i<n; i++)
	{
		if (fabsf(out_naive[i] - out_sparse[i]) > 1e-6f)
		{
			printf("%s MISMATCH at %d: %g vs %g\n",
			       name, i, out_naive[i], out_sparse[i]);
			return 1;
		}
	}

	return 0;
}

static int check_double_case(const char *name, double *x)
{
	double out_naive[N];
	double out_sparse[N];
	int i;

	compute_prod_double_naive(x, out_naive);
	compute_sparse_double(x, out_sparse);

	for (i=0; i<N; i++)
	{
		if (fabs(out_naive[i] - out_sparse[i]) > 1e-12)
		{
			printf("%s MISMATCH at %d: %.17g vs %.17g\n",
			       name, i, out_naive[i], out_sparse[i]);
			return 1;
		}
	}

	return 0;
}

int main(void)
{
	float ones[N];
	float ramp[N];
	double dones[N];
	double dramp[N];
	int i;

	fill_extra_constants();

	for (i=0; i<N; i++)
	{
		ones[i] = 1.0f;
		ramp[i] = (float)(i + 1) * 0.1f;
		dones[i] = 1.0;
		dramp[i] = (double)(i + 1) * 0.125;
	}

	float grid[N2];

	for (i=0; i<N2; i++)
		grid[i] = (float)(i + 1) * 0.2f;

	float big[BIG];
	float block[BLOCK];
	float xtup[XTUP];

	for (i=0; i<BIG; i++)
		big[i] = (float)(i + 1) * 0.03125f;
	for (i=0; i<BLOCK; i++)
		block[i] = (float)(i + 1) * 0.0625f;
	for (i=0; i<XTUP; i++)
		xtup[i] = (float)(i + 1) * 0.25f;

	if (check_float_case("prod", compute_sparse, compute_prod_float_naive, ones, N))
		return 1;
	if (check_float_case("prod", compute_sparse, compute_prod_float_naive, ramp, N))
		return 1;
	if (check_float_case("reverse", compute_sparse_reverse, compute_prod_float_naive, ramp, N))
		return 1;
	if (check_float_case("sum", compute_sparse_sum, compute_sum_float_naive, ones, N))
		return 1;
	if (check_float_case("sum", compute_sparse_sum, compute_sum_float_naive, ramp, N))
		return 1;
	if (check_double_case("double", dones))
		return 1;
	if (check_double_case("double", dramp))
		return 1;
	if (check_float_case("2d", compute_sparse_2d, compute_prod_2d_naive, grid, N2))
		return 1;
	if (check_float_case("2d_sum", compute_sparse_2d_sum, compute_sum_2d_naive, grid, N2))
		return 1;
	if (check_float_case("tuple", compute_sparse_tuple, compute_prod_tuple_naive, grid, N2))
		return 1;
	if (check_float_case("tuple_sum", compute_sparse_tuple_sum, compute_sum_tuple_naive, grid, N2))
		return 1;
	if (check_float_case("ir", compute_sparse_ir, compute_prod_ir_naive, big, BIG))
		return 1;
	if (check_float_case("block", compute_sparse_block, compute_prod_block_naive, block, BLOCK))
		return 1;
	if (check_float_case("x_tuple", compute_sparse_x_tuple, compute_prod_x_tuple_naive, xtup, XTUP))
		return 1;
	if (check_float_case("dual_tuple", compute_sparse_dual_tuple, compute_prod_dual_tuple_naive, xtup, XTUP))
		return 1;
	if (check_float_case("dual_tuple_sum", compute_sparse_dual_tuple_sum, compute_sum_dual_tuple_naive, xtup, XTUP))
		return 1;
	if (check_float_case("opt_pass", compute_sparse_opt_pass, compute_prod_opt_pass_naive, ramp, OPT_PASS_NNZ))
		return 1;

	printf("OK\n");
	return 0;
}
