#include <stdint.h>
#include <stdlib.h>
#include <string.h>
void compute_sparse(float *input_x, float *out) {
float*var_2_=(float*)malloc(64);memset(var_2_,0,64);{float*var_3_=(float*)((char*)(input_x)+0);float*var_4_=(float*)((char*)(var_2_)+0);var_4_[0]=5.000000000e-01f*var_3_[0];var_4_[3]=1.200000048e+00f*var_3_[3];var_4_[6]=var_3_[6];var_4_[9]=-3.000000119e-01f*var_3_[9];var_4_[14]=6.999999881e-01f*var_3_[14];}
memcpy(out,var_2_,64);free(var_2_);
}
