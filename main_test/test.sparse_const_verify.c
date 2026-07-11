#include	<math.h>
#include	<stdio.h>

#define N 16
#define ROWS 3
#define COLS 4
#define N2 (ROWS * COLS)

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

void compute_sparse(float *x, float *out);
void compute_sparse_reverse(float *x, float *out);
void compute_sparse_sum(float *x, float *out);
void compute_sparse_double(double *x, double *out);
void compute_sparse_2d(float *x, float *out);
void compute_sparse_2d_sum(float *x, float *out);
void compute_sparse_tuple(float *x, float *out);
void compute_sparse_tuple_sum(float *x, float *out);

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

static int check_float_case(const char *name, void (*generated)(float *, float *),
                            void (*naive)(float *, float *), float *x, int n)
{
	float out_naive[N2 > N ? N2 : N];
	float out_sparse[N2 > N ? N2 : N];
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

	printf("OK\n");
	return 0;
}
