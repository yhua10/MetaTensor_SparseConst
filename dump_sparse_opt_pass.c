#include <stdint.h>
#include <stdlib.h>
#include <string.h>
void compute_sparse_opt_pass(float *input_x, float *out) {
float*var_287_=(float*)malloc(64);memset(var_287_,0,64);float*var_292_=malloc(20);{const float var_293_[5]={5.000000000e-01f,1.200000048e+00f,1.000000000e+00f,-3.000000119e-01f,6.999999881e-01f};float*var_289_=(float*)var_293_;float*var_290_=(float*)((char *)(input_x)+0);float*var_291_=(float*)((char *)(var_292_)+0);int32_t var_296_=0;int32_t var_297_=0;int32_t var_298_=0;const int32_t var_299_[5]={0,1,2,3,4};const int32_t var_300_[5]={0,3,6,9,14};const int32_t var_301_[5]={0,1,2,3,4};for(int32_t var_302_=0;var_302_<5;var_302_++){int32_t var_303_=var_296_+var_299_[var_302_];int32_t var_304_=var_297_+var_300_[var_302_];int32_t var_305_=var_298_+var_301_[var_302_];var_291_[var_305_]=var_289_[var_303_]*var_290_[var_304_];}}{float*var_307_=(float*)((char*)(var_292_)+0);float*var_308_=(float*)((char*)(var_287_)+0);const int32_t var_309_[5]={0,3,6,9,14};for(int32_t var_310_=0;var_310_<5;var_310_++){var_308_[var_309_[var_310_]]=var_307_[var_310_];}free(var_292_);}
memcpy(out,var_287_,64);free(var_287_);
}
