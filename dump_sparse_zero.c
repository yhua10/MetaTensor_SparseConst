#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
void compute_sparse_zero(float *input_x, float *out) {
float*var_81_=(float*)malloc(64);memset(var_81_,0,64);
memcpy(out,var_81_,64);free(var_81_);
}
