#include	<math.h>
#include	<stdio.h>

#define N 16

static float K_const[N] = {
	0.5f, 0.0f, 0.0f, 1.2f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, -0.3f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f,
};

void compute_sparse(float *x, float *out);

static void compute_naive(float *x, float *out)
{
	int i;
	for (i=0; i<N; i++)
		out[i] = K_const[i] * x[i];
}

static int check_case(float *x)
{
	float out_naive[N];
	float out_sparse[N];
	int i;

	compute_naive(x, out_naive);
	compute_sparse(x, out_sparse);

	for (i=0; i<N; i++)
	{
		if (fabsf(out_naive[i] - out_sparse[i]) > 1e-6f)
		{
			printf("MISMATCH at %d: %g vs %g\n",
			       i, out_naive[i], out_sparse[i]);
			return 1;
		}
	}

	return 0;
}

int main(void)
{
	float ones[N];
	float ramp[N];
	int i;

	for (i=0; i<N; i++)
	{
		ones[i] = 1.0f;
		ramp[i] = (float)(i + 1) * 0.1f;
	}

	if (check_case(ones))
		return 1;
	if (check_case(ramp))
		return 1;

	printf("OK\n");
	return 0;
}
