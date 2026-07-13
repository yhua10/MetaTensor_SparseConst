#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
void compute_sparse_double(double *input_x, double *out) {
double*var_87_=(double*)malloc(128);memset(var_87_,0,128);double*var_92_=malloc(40);{const double var_93_[5]={2.50000000000000000e-01,-2.50000000000000000e+00,1.00000000000000000e+00,3.12500000000000000e+00,-7.50000000000000000e-01};double*var_89_=(double*)var_93_;double*var_90_=(double*)((char *)(input_x)+0);double*var_91_=(double*)((char *)(var_92_)+0);int32_t var_96_=0;int32_t var_97_=0;int32_t var_98_=0;const int32_t var_99_[5]={0,1,2,3,4};const int32_t var_100_[5]={0,3,5,8,14};const int32_t var_101_[5]={0,1,2,3,4};for(int32_t var_102_=0;var_102_<5;var_102_++){int32_t var_103_=var_96_+var_99_[var_102_];int32_t var_104_=var_97_+var_100_[var_102_];int32_t var_105_=var_98_+var_101_[var_102_];var_91_[var_105_]=var_89_[var_103_]*var_90_[var_104_];}}{double*var_107_=(double*)((char*)(var_92_)+0);double*var_108_=(double*)((char*)(var_87_)+0);const int32_t var_109_[5]={0,3,5,8,14};for(int32_t var_110_=0;var_110_<5;var_110_++){var_108_[var_109_[var_110_]]=var_107_[var_110_];}free(var_92_);}
memcpy(out,var_87_,128);free(var_87_);
}
