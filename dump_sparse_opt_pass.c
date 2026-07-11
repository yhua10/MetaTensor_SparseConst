#include <stdint.h>
#include <stdlib.h>
#include <string.h>
void compute_sparse_opt_pass(float *input_x, float *out) {
float*var_82_=malloc(20);{const float var_83_[5]={5.000000000e-01f,1.200000048e+00f,1.000000000e+00f,-3.000000119e-01f,6.999999881e-01f};float*var_79_=(float*)var_83_;float*var_80_=(float*)((char *)(input_x)+0);float*var_81_=(float*)((char *)(var_82_)+0);int32_t var_86_=0;int32_t var_87_=0;int32_t var_88_=0;const int32_t var_89_[5]={0,1,2,3,4};const int32_t var_90_[5]={0,3,6,9,14};const int32_t var_91_[5]={0,1,2,3,4};for(int32_t var_92_=0;var_92_<5;var_92_++){int32_t var_93_=var_86_+var_89_[var_92_];int32_t var_94_=var_87_+var_90_[var_92_];int32_t var_95_=var_88_+var_91_[var_92_];var_81_[var_95_]=1*var_79_[var_93_]*var_80_[var_94_];}}
memcpy(out,var_82_,20);free(var_82_);
}
