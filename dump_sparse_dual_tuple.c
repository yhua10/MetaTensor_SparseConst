#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
void compute_sparse_dual_tuple(float *input_x, float *out) {
float*var_71_=(float*)malloc(16);memset(var_71_,0,16);{float*var_72_=(float*)((char*)(input_x)+0);float*var_73_=(float*)((char*)(var_71_)+0);var_73_[0]=5.000000000e-01f*var_72_[0];var_73_[2]=var_72_[2];var_73_[3]=-7.500000000e-01f*var_72_[3];}
memcpy(out,var_71_,16);free(var_71_);
}
