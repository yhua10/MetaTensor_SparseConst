#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
void compute_sparse_dual_tuple(float *input_x, float *out) {
float*var_278_=(float*)malloc(16);memset(var_278_,0,16);{float*var_279_=(float*)((char*)(input_x)+0);float*var_280_=(float*)((char*)(var_278_)+0);var_280_[0]=5.000000000e-01f*var_279_[0];var_280_[2]=var_279_[2];var_280_[3]=-7.500000000e-01f*var_279_[3];}
memcpy(out,var_278_,16);free(var_278_);
}
