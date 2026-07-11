#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
void compute_sparse_reverse(float *input_x, float *out) {
float*var_6_=(float*)malloc(64);memset(var_6_,0,64);{float*var_7_=(float*)((char*)(input_x)+0);float*var_8_=(float*)((char*)(var_6_)+0);var_8_[0]=5.000000000e-01f*var_7_[0];var_8_[3]=1.200000048e+00f*var_7_[3];var_8_[6]=var_7_[6];var_8_[9]=-3.000000119e-01f*var_7_[9];var_8_[14]=6.999999881e-01f*var_7_[14];}
memcpy(out,var_6_,64);free(var_6_);
}
