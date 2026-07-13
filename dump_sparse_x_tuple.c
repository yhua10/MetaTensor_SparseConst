#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
void compute_sparse_x_tuple(float *input_x, float *out) {
float*var_274_=(float*)malloc(16);memset(var_274_,0,16);{float*var_275_=(float*)((char*)(input_x)+0);float*var_276_=(float*)((char*)(var_274_)+0);var_276_[0]=5.000000000e-01f*var_275_[0];var_276_[3]=-2.500000000e-01f*var_275_[3];}
memcpy(out,var_274_,16);free(var_274_);
}
