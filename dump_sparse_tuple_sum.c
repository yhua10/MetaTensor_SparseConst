#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
void compute_sparse_tuple_sum(float *input_x, float *out) {
float*var_220_=(float*)malloc(48);memcpy(var_220_,(char*)(input_x)+0,48);{float*var_221_=(float*)((char*)(input_x)+0);float*var_222_=(float*)((char*)(var_220_)+0);var_222_[0]=5.000000000e-01f+var_221_[0];var_222_[6]=1.000000000e+00f+var_221_[6];var_222_[11]=-7.500000000e-01f+var_221_[11];}
memcpy(out,var_220_,48);free(var_220_);
}
