#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
void compute_sparse_sum(float *input_x, float *out) {
float*var_55_=(float*)malloc(64);memcpy(var_55_,(char*)(input_x)+0,64);float*var_60_=malloc(20);{const float var_61_[5]={5.000000000e-01f,1.200000048e+00f,1.000000000e+00f,-3.000000119e-01f,6.999999881e-01f};float*var_57_=(float*)var_61_;float*var_58_=(float*)((char *)(input_x)+0);float*var_59_=(float*)((char *)(var_60_)+0);int32_t var_64_=0;int32_t var_65_=0;int32_t var_66_=0;const int32_t var_67_[5]={0,1,2,3,4};const int32_t var_68_[5]={0,3,6,9,14};const int32_t var_69_[5]={0,1,2,3,4};for(int32_t var_70_=0;var_70_<5;var_70_++){int32_t var_71_=var_64_+var_67_[var_70_];int32_t var_72_=var_65_+var_68_[var_70_];int32_t var_73_=var_66_+var_69_[var_70_];var_59_[var_73_]=var_57_[var_71_]+var_58_[var_72_];}}{float*var_75_=(float*)((char*)(var_60_)+0);float*var_76_=(float*)((char*)(var_55_)+0);const int32_t var_77_[5]={0,3,6,9,14};for(int32_t var_78_=0;var_78_<5;var_78_++){var_76_[var_77_[var_78_]]=var_75_[var_78_];}free(var_60_);}
memcpy(out,var_55_,64);free(var_55_);
}
