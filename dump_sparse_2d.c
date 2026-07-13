#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
void compute_sparse_2d(float *input_x, float *out) {
float*var_113_=(float*)malloc(48);memset(var_113_,0,48);float*var_118_=malloc(20);{const float var_119_[5]={5.000000000e-01f,1.000000000e+00f,-1.500000000e+00f,2.000000000e+00f,-2.500000000e-01f};float*var_115_=(float*)var_119_;float*var_116_=(float*)((char *)(input_x)+0);float*var_117_=(float*)((char *)(var_118_)+0);int32_t var_122_=0;int32_t var_123_=0;int32_t var_124_=0;const int32_t var_125_[5]={0,1,2,3,4};const int32_t var_126_[5]={0,3,5,8,11};const int32_t var_127_[5]={0,1,2,3,4};for(int32_t var_128_=0;var_128_<5;var_128_++){int32_t var_129_=var_122_+var_125_[var_128_];int32_t var_130_=var_123_+var_126_[var_128_];int32_t var_131_=var_124_+var_127_[var_128_];var_117_[var_131_]=var_115_[var_129_]*var_116_[var_130_];}}{float*var_133_=(float*)((char*)(var_118_)+0);float*var_134_=(float*)((char*)(var_113_)+0);const int32_t var_135_[5]={0,3,5,8,11};for(int32_t var_136_=0;var_136_<5;var_136_++){var_134_[var_135_[var_136_]]=var_133_[var_136_];}free(var_118_);}
memcpy(out,var_113_,48);free(var_113_);
}
