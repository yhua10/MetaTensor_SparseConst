#include <stdint.h>
#include <stdlib.h>
#include <string.h>
void compute_sparse_tuple(float *input_x, float *out) {
float*var_26_=(float*)malloc(48);memset(var_26_,0,48);{float*var_27_=(float*)((char*)(input_x)+0);float*var_28_=(float*)((char*)(var_26_)+0);var_28_[0]=5.000000000e-01f*var_27_[0];var_28_[6]=var_27_[6];var_28_[11]=-7.500000000e-01f*var_27_[11];}
memcpy(out,var_26_,48);free(var_26_);
}
