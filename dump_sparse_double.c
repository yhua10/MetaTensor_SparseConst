#include <stdint.h>
#include <stdlib.h>
#include <string.h>
void compute_sparse_double(double *input_x, double *out) {
double*var_14_=(double*)malloc(128);memset(var_14_,0,128);{double*var_15_=(double*)((char*)(input_x)+0);double*var_16_=(double*)((char*)(var_14_)+0);var_16_[0]=2.50000000000000000e-01*var_15_[0];var_16_[3]=-2.50000000000000000e+00*var_15_[3];var_16_[5]=var_15_[5];var_16_[8]=3.12500000000000000e+00*var_15_[8];var_16_[14]=-7.50000000000000000e-01*var_15_[14];}
memcpy(out,var_14_,128);free(var_14_);
}
