#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
void compute_sparse_dual_tuple_sum(float *input_x, float *out) {
float*var_282_=(float*)malloc(16);memcpy(var_282_,(char*)(input_x)+0,16);{float*var_283_=(float*)((char*)(input_x)+0);float*var_284_=(float*)((char*)(var_282_)+0);var_284_[0]=5.000000000e-01f+var_283_[0];var_284_[2]=1.000000000e+00f+var_283_[2];var_284_[3]=-7.500000000e-01f+var_283_[3];}
memcpy(out,var_282_,16);free(var_282_);
}
