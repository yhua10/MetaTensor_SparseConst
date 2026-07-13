#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
void compute_sparse_broadcast_sum(float *input_x, float *out) {
float*var_191_=(float*)malloc(48);memcpy(var_191_,(char*)(input_x)+0,48);float*var_196_=malloc(24);{const float var_197_[6]={2.000000000e+00f,-1.000000000e+00f,2.000000000e+00f,-1.000000000e+00f,2.000000000e+00f,-1.000000000e+00f};float*var_193_=(float*)var_197_;float*var_194_=(float*)((char *)(input_x)+4);float*var_195_=(float*)((char *)(var_196_)+0);int32_t var_200_=0;int32_t var_201_=0;int32_t var_202_=0;const int32_t var_203_[6]={0,1,2,3,4,5};const int32_t var_204_[6]={0,1,4,5,8,9};const int32_t var_205_[6]={0,1,2,3,4,5};for(int32_t var_206_=0;var_206_<6;var_206_++){int32_t var_207_=var_200_+var_203_[var_206_];int32_t var_208_=var_201_+var_204_[var_206_];int32_t var_209_=var_202_+var_205_[var_206_];var_195_[var_209_]=var_193_[var_207_]+var_194_[var_208_];}}{float*var_211_=(float*)((char*)(var_196_)+0);float*var_212_=(float*)((char*)(var_191_)+0);const int32_t var_213_[6]={1,2,5,6,9,10};for(int32_t var_214_=0;var_214_<6;var_214_++){var_212_[var_213_[var_214_]]=var_211_[var_214_];}free(var_196_);}
memcpy(out,var_191_,48);free(var_191_);
}
