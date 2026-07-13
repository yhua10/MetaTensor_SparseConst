#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
void compute_sparse_reverse(float *input_x, float *out) {
float*var_29_=(float*)malloc(64);memset(var_29_,0,64);float*var_34_=malloc(20);{const float var_35_[5]={5.000000000e-01f,1.200000048e+00f,1.000000000e+00f,-3.000000119e-01f,6.999999881e-01f};float*var_31_=(float*)var_35_;float*var_32_=(float*)((char *)(input_x)+0);float*var_33_=(float*)((char *)(var_34_)+0);int32_t var_38_=0;int32_t var_39_=0;int32_t var_40_=0;const int32_t var_41_[5]={0,1,2,3,4};const int32_t var_42_[5]={0,3,6,9,14};const int32_t var_43_[5]={0,1,2,3,4};for(int32_t var_44_=0;var_44_<5;var_44_++){int32_t var_45_=var_38_+var_41_[var_44_];int32_t var_46_=var_39_+var_42_[var_44_];int32_t var_47_=var_40_+var_43_[var_44_];var_33_[var_47_]=var_31_[var_45_]*var_32_[var_46_];}}{float*var_49_=(float*)((char*)(var_34_)+0);float*var_50_=(float*)((char*)(var_29_)+0);const int32_t var_51_[5]={0,3,6,9,14};for(int32_t var_52_=0;var_52_<5;var_52_++){var_50_[var_51_[var_52_]]=var_49_[var_52_];}free(var_34_);}
memcpy(out,var_29_,64);free(var_29_);
}
