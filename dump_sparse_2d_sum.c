#include <stdint.h>
#include <stdlib.h>
#include <string.h>
void compute_sparse_2d_sum(float *input_x, float *out) {
float*var_22_=(float*)malloc(48);{float*var_23_=(float*)((char*)(input_x)+0);float*var_24_=(float*)((char*)(var_22_)+0);var_24_[0]=5.000000000e-01f+var_23_[0];var_24_[1]=var_23_[1];var_24_[2]=var_23_[2];var_24_[3]=1.000000000e+00f+var_23_[3];var_24_[4]=var_23_[4];var_24_[5]=-1.500000000e+00f+var_23_[5];var_24_[6]=var_23_[6];var_24_[7]=var_23_[7];var_24_[8]=2.000000000e+00f+var_23_[8];var_24_[9]=var_23_[9];var_24_[10]=var_23_[10];var_24_[11]=-2.500000000e-01f+var_23_[11];}
memcpy(out,var_22_,48);free(var_22_);
}
