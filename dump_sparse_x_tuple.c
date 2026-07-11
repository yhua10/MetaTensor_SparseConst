#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
void compute_sparse_x_tuple(float *input_x, float *out) {
float*var_67_=(float*)malloc(16);memset(var_67_,0,16);{float*var_68_=(float*)((char*)(input_x)+0);float*var_69_=(float*)((char*)(var_67_)+0);var_69_[0]=5.000000000e-01f*var_68_[0];var_69_[3]=-2.500000000e-01f*var_68_[3];}
memcpy(out,var_67_,16);free(var_67_);
}
