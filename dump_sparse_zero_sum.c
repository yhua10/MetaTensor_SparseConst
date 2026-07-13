#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
void compute_sparse_zero_sum(float *input_x, float *out) {
float*var_84_=(float*)malloc(64);memcpy(var_84_,(char*)(input_x)+0,64);
memcpy(out,var_84_,64);free(var_84_);
}
