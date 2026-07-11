#include <stdint.h>
#include <stdlib.h>
#include <string.h>
void compute_sparse_2d(float *input_x, float *out) {
float*var_18_=(float*)malloc(48);memset(var_18_,0,48);{float*var_19_=(float*)((char*)(input_x)+0);float*var_20_=(float*)((char*)(var_18_)+0);var_20_[0]=5.000000000e-01f*var_19_[0];var_20_[3]=var_19_[3];var_20_[5]=-1.500000000e+00f*var_19_[5];var_20_[8]=2.000000000e+00f*var_19_[8];var_20_[11]=-2.500000000e-01f*var_19_[11];}
memcpy(out,var_18_,48);free(var_18_);
}
