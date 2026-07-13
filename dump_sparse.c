#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
void compute_sparse(float *input_x, float *out) {
float*var_3_=(float*)malloc(64);memset(var_3_,0,64);float*var_8_=malloc(20);{const float var_9_[5]={5.000000000e-01f,1.200000048e+00f,1.000000000e+00f,-3.000000119e-01f,6.999999881e-01f};float*var_5_=(float*)var_9_;float*var_6_=(float*)((char *)(input_x)+0);float*var_7_=(float*)((char *)(var_8_)+0);int32_t var_12_=0;int32_t var_13_=0;int32_t var_14_=0;const int32_t var_15_[5]={0,1,2,3,4};const int32_t var_16_[5]={0,3,6,9,14};const int32_t var_17_[5]={0,1,2,3,4};for(int32_t var_18_=0;var_18_<5;var_18_++){int32_t var_19_=var_12_+var_15_[var_18_];int32_t var_20_=var_13_+var_16_[var_18_];int32_t var_21_=var_14_+var_17_[var_18_];var_7_[var_21_]=var_5_[var_19_]*var_6_[var_20_];}}{float*var_23_=(float*)((char*)(var_8_)+0);float*var_24_=(float*)((char*)(var_3_)+0);const int32_t var_25_[5]={0,3,6,9,14};for(int32_t var_26_=0;var_26_<5;var_26_++){var_24_[var_25_[var_26_]]=var_23_[var_26_];}free(var_8_);}
memcpy(out,var_3_,64);free(var_3_);
}
