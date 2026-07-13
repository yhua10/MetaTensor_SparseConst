#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
void compute_sparse_tuple(float *input_x, float *out) {
float*var_216_=(float*)malloc(48);memset(var_216_,0,48);{float*var_217_=(float*)((char*)(input_x)+0);float*var_218_=(float*)((char*)(var_216_)+0);var_218_[0]=5.000000000e-01f*var_217_[0];var_218_[6]=var_217_[6];var_218_[11]=-7.500000000e-01f*var_217_[11];}
memcpy(out,var_216_,48);free(var_216_);
}
