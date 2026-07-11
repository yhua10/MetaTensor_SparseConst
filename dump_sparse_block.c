#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
void compute_sparse_block(float *input_x, float *out) {
float*var_58_=(float*)malloc(128);memset(var_58_,0,128);{float*var_59_=(float*)((char*)(input_x)+0);float*var_60_=(float*)((char*)(var_58_)+0);{const float var_61_[20]={2.000000030e-01f,4.000000060e-01f,6.000000238e-01f,8.000000119e-01f,1.000000000e+00f,2.000000030e-01f,4.000000060e-01f,6.000000238e-01f,8.000000119e-01f,1.000000000e+00f,2.000000030e-01f,4.000000060e-01f,6.000000238e-01f,8.000000119e-01f,1.000000000e+00f,2.000000030e-01f,4.000000060e-01f,6.000000238e-01f,8.000000119e-01f,1.000000000e+00f};int32_t var_62_=0;
#if defined(__AVX2__)
for(;var_62_+8<=20;var_62_+=8){__m256 var_63_=_mm256_loadu_ps(var_61_+var_62_);__m256 var_64_=_mm256_loadu_ps(var_59_+0+var_62_);__m256 var_65_=_mm256_mul_ps(var_63_,var_64_);_mm256_storeu_ps(var_60_+0+var_62_,var_65_);}
#endif
for(;var_62_<20;var_62_++){var_60_[0+var_62_]=var_61_[var_62_]*var_59_[0+var_62_];}}}
memcpy(out,var_58_,128);free(var_58_);
}
