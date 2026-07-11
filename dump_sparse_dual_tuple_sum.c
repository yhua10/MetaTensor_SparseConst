#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
void compute_sparse_dual_tuple_sum(float *input_x, float *out) {
float*var_75_=(float*)malloc(16);memcpy(var_75_,(char*)(input_x)+0,16);{float*var_76_=(float*)((char*)(input_x)+0);float*var_77_=(float*)((char*)(var_75_)+0);var_77_[0]=5.000000000e-01f+var_76_[0];var_77_[2]=1.000000000e+00f+var_76_[2];var_77_[3]=-7.500000000e-01f+var_76_[3];}
memcpy(out,var_75_,16);free(var_75_);
}
