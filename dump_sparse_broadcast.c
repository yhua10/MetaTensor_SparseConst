#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
void compute_sparse_broadcast(float *input_x, float *out) {
float*var_165_=(float*)malloc(48);memset(var_165_,0,48);float*var_170_=malloc(24);{const float var_171_[6]={2.000000000e+00f,-1.000000000e+00f,2.000000000e+00f,-1.000000000e+00f,2.000000000e+00f,-1.000000000e+00f};float*var_167_=(float*)var_171_;float*var_168_=(float*)((char *)(input_x)+4);float*var_169_=(float*)((char *)(var_170_)+0);int32_t var_174_=0;int32_t var_175_=0;int32_t var_176_=0;const int32_t var_177_[6]={0,1,2,3,4,5};const int32_t var_178_[6]={0,1,4,5,8,9};const int32_t var_179_[6]={0,1,2,3,4,5};for(int32_t var_180_=0;var_180_<6;var_180_++){int32_t var_181_=var_174_+var_177_[var_180_];int32_t var_182_=var_175_+var_178_[var_180_];int32_t var_183_=var_176_+var_179_[var_180_];var_169_[var_183_]=var_167_[var_181_]*var_168_[var_182_];}}{float*var_185_=(float*)((char*)(var_170_)+0);float*var_186_=(float*)((char*)(var_165_)+0);const int32_t var_187_[6]={1,2,5,6,9,10};for(int32_t var_188_=0;var_188_<6;var_188_++){var_186_[var_187_[var_188_]]=var_185_[var_188_];}free(var_170_);}
memcpy(out,var_165_,48);free(var_165_);
}
