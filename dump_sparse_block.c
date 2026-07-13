#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
void compute_sparse_block(float *input_x, float *out) {
float*var_251_=(float*)malloc(128);memset(var_251_,0,128);float*var_256_=malloc(80);{const float var_257_[20]={2.000000030e-01f,4.000000060e-01f,6.000000238e-01f,8.000000119e-01f,1.000000000e+00f,2.000000030e-01f,4.000000060e-01f,6.000000238e-01f,8.000000119e-01f,1.000000000e+00f,2.000000030e-01f,4.000000060e-01f,6.000000238e-01f,8.000000119e-01f,1.000000000e+00f,2.000000030e-01f,4.000000060e-01f,6.000000238e-01f,8.000000119e-01f,1.000000000e+00f};float*var_253_=(float*)var_257_;float*var_254_=(float*)((char *)(input_x)+0);float*var_255_=(float*)((char *)(var_256_)+0);int32_t var_260_=0;int32_t var_261_=0;int32_t var_262_=0;const int32_t var_263_[20]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};const int32_t var_264_[20]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};const int32_t var_265_[20]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};for(int32_t var_266_=0;var_266_<20;var_266_++){int32_t var_267_=var_260_+var_263_[var_266_];int32_t var_268_=var_261_+var_264_[var_266_];int32_t var_269_=var_262_+var_265_[var_266_];var_255_[var_269_]=var_253_[var_267_]*var_254_[var_268_];}}{float*var_271_=(float*)((char*)(var_256_)+0);float*var_272_=(float*)((char*)(var_251_)+0);memcpy(var_272_,var_271_,80);free(var_256_);}
memcpy(out,var_251_,128);free(var_251_);
}
