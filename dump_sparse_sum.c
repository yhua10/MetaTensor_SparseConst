#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
void compute_sparse_sum(float *input_x, float *out) {
float*var_10_=(float*)malloc(64);memcpy(var_10_,(char*)(input_x)+0,64);{float*var_11_=(float*)((char*)(input_x)+0);float*var_12_=(float*)((char*)(var_10_)+0);var_12_[0]=5.000000000e-01f+var_11_[0];var_12_[3]=1.200000048e+00f+var_11_[3];var_12_[6]=1.000000000e+00f+var_11_[6];var_12_[9]=-3.000000119e-01f+var_11_[9];var_12_[14]=6.999999881e-01f+var_11_[14];}
memcpy(out,var_10_,64);free(var_10_);
}
