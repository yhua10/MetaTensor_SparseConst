#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
void compute_sparse_2d_sum(float *input_x, float *out) {
float*var_139_=(float*)malloc(48);memcpy(var_139_,(char*)(input_x)+0,48);float*var_144_=malloc(20);{const float var_145_[5]={5.000000000e-01f,1.000000000e+00f,-1.500000000e+00f,2.000000000e+00f,-2.500000000e-01f};float*var_141_=(float*)var_145_;float*var_142_=(float*)((char *)(input_x)+0);float*var_143_=(float*)((char *)(var_144_)+0);int32_t var_148_=0;int32_t var_149_=0;int32_t var_150_=0;const int32_t var_151_[5]={0,1,2,3,4};const int32_t var_152_[5]={0,3,5,8,11};const int32_t var_153_[5]={0,1,2,3,4};for(int32_t var_154_=0;var_154_<5;var_154_++){int32_t var_155_=var_148_+var_151_[var_154_];int32_t var_156_=var_149_+var_152_[var_154_];int32_t var_157_=var_150_+var_153_[var_154_];var_143_[var_157_]=var_141_[var_155_]+var_142_[var_156_];}}{float*var_159_=(float*)((char*)(var_144_)+0);float*var_160_=(float*)((char*)(var_139_)+0);const int32_t var_161_[5]={0,3,5,8,11};for(int32_t var_162_=0;var_162_<5;var_162_++){var_160_[var_161_[var_162_]]=var_159_[var_162_];}free(var_144_);}
memcpy(out,var_139_,48);free(var_139_);
}
