#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
void compute_sparse_tuple_sum(float *input_x, float *out) {
float*var_30_=(float*)malloc(48);memcpy(var_30_,(char*)(input_x)+0,48);{float*var_31_=(float*)((char*)(input_x)+0);float*var_32_=(float*)((char*)(var_30_)+0);var_32_[0]=5.000000000e-01f+var_31_[0];var_32_[6]=1.000000000e+00f+var_31_[6];var_32_[11]=-7.500000000e-01f+var_31_[11];}
memcpy(out,var_30_,48);free(var_30_);
}
