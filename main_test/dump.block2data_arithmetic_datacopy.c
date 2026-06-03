#include <stdint.h>
#include <stdlib.h>
void calc(void *res, void *a , void *b, void *c){
	double*var_5_=malloc(2048);
	{
		double*var_2_=(double*)((char *)(b)+0);
		double*var_3_=(double*)((char *)(c)+0);
		double*var_4_=(double*)((char *)(var_5_)+0);
		int32_t var_7_=0;
		int32_t var_8_=0;
		int32_t var_9_=0;
		const int32_t var_10_[4]={ 0,16,32,48};
		const int32_t var_11_[4]={ 0,64,128,192};
		for(int32_t var_12_=0; var_12_<4; var_12_++){
			int32_t var_13_=var_7_+var_10_[var_12_];
			int32_t var_14_=var_9_+var_11_[var_12_];
			const int32_t var_15_[4]={ 0,4,8,12};
			const int32_t var_16_[4]={ 0,4,8,12};
			const int32_t var_17_[4]={ 0,16,32,48};
			for(int32_t var_18_=0; var_18_<4; var_18_++){
				int32_t var_19_=var_13_+var_15_[var_18_];
				int32_t var_20_=var_8_+var_16_[var_18_];
				int32_t var_21_=var_14_+var_17_[var_18_];
				const int32_t var_22_[4]={ 0,1,2,3};
				const int32_t var_23_[4]={ 0,4,8,12};
				for(int32_t var_24_=0; var_24_<4; var_24_++){
					int32_t var_25_=var_19_+var_22_[var_24_];
					int32_t var_26_=var_21_+var_23_[var_24_];
					const int32_t var_27_[4]={ 0,1,2,3};
					const int32_t var_28_[4]={ 0,1,2,3};
					for(int32_t var_29_=0; var_29_<4; var_29_++){
						int32_t var_30_=var_20_+var_27_[var_29_];
						int32_t var_31_=var_26_+var_28_[var_29_];
						var_4_[var_31_]=0+var_2_[var_25_]+var_3_[var_30_];
					}}}}}
	double*var_37_=malloc(2048);
	{
		double*var_34_=(double*)((char *)(a)+0);
		double*var_35_=(double*)((char *)(var_5_)+0);
		double*var_36_=(double*)((char *)(var_37_)+0);
		int32_t var_39_=0;
		int32_t var_40_=0;
		int32_t var_41_=0;
		const int32_t var_42_[4]={ 0,4,8,12};
		const int32_t var_43_[4]={ 0,1,2,3};
		const int32_t var_44_[4]={ 0,64,128,192};
		for(int32_t var_45_=0; var_45_<4; var_45_++){
			int32_t var_46_=var_39_+var_42_[var_45_];
			int32_t var_47_=var_40_+var_43_[var_45_];
			int32_t var_48_=var_41_+var_44_[var_45_];
			const int32_t var_49_[4]={ 0,1,2,3};
			const int32_t var_50_[4]={ 0,64,128,192};
			const int32_t var_51_[4]={ 0,16,32,48};
			for(int32_t var_52_=0; var_52_<4; var_52_++){
				int32_t var_53_=var_46_+var_49_[var_52_];
				int32_t var_54_=var_47_+var_50_[var_52_];
				int32_t var_55_=var_48_+var_51_[var_52_];
				const int32_t var_56_[4]={ 0,16,32,48};
				const int32_t var_57_[4]={ 0,4,8,12};
				for(int32_t var_58_=0; var_58_<4; var_58_++){
					int32_t var_59_=var_54_+var_56_[var_58_];
					int32_t var_60_=var_55_+var_57_[var_58_];
					const int32_t var_61_[4]={ 0,4,8,12};
					const int32_t var_62_[4]={ 0,1,2,3};
					for(int32_t var_63_=0; var_63_<4; var_63_++){
						int32_t var_64_=var_59_+var_61_[var_63_];
						int32_t var_65_=var_60_+var_62_[var_63_];
						var_36_[var_65_]=1*var_34_[var_53_]*var_35_[var_64_];
					}}}}}
	{
		double*var_68_=(double*)((char *)(res)+0);
		double*var_69_=(double*)((char *)(var_37_)+0);
		int32_t var_70_=0;
		int32_t var_71_=0;
		int32_t var_72_[4]={ 0,4,8,12};
		int32_t var_73_[4]={ 0,4,8,12};
		int32_t var_74_;
		for(var_74_=0; var_74_<4; var_74_++){
			int32_t var_75_=var_70_+var_72_[var_74_];
			int32_t var_76_=var_71_+var_73_[var_74_];
			int32_t var_77_[4]={ 0,1,2,3};
			int32_t var_78_[4]={ 0,1,2,3};
			int32_t var_79_;
			for(var_79_=0; var_79_<4; var_79_++){
				int32_t var_80_=var_75_+var_77_[var_79_];
				int32_t var_81_=var_76_+var_78_[var_79_];
				double var_82_=0;
				int32_t var_83_[4]={ 0,64,128,192};
				int32_t var_84_;
				for(var_84_=0; var_84_<4; var_84_++){
					int32_t var_85_=var_81_+var_83_[var_84_];
					int32_t var_86_[4]={ 0,16,32,48};
					int32_t var_87_;
					for(var_87_=0; var_87_<4; var_87_++){
						int32_t var_88_=var_85_+var_86_[var_87_];
						var_82_+=var_69_[var_88_];
					}}
				var_68_[var_80_]=var_82_;
			}}}
}
