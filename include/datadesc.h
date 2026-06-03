#ifndef	F_DATADESC_H_
#define	F_DATADESC_H_


/*
 * 关于datadesc的说明：
 *
 * datadesc是用于张量数据结构的元编程的工具，它描述了在目标代码中的数据类型和储存方式，它一般在元编程阶段被建立和处理。
 *
 * datadesc里的成员变量如下：
 * 	region表示这个数据储存的位置在哪里：
 * 		none表示其实并没有这个数据，只是在处理数据类型时作为中间类型辅助用的
 * 		direct是在元编程阶段直接可用的数据，对于目标代码而言是常量
 * 		main是表示在目标程序的运行时的主内存
 * 		sunway_...、tianhe_...等表示不同架构下的不同内存区域，比如神威从核的LDM、GPU显存等
 * 	ref表示对于这个数据的访问名字
 * 		如果使用p_direct，则表示在元编程阶段的实际内存指针
 * 		如果使用p_refname，则表示在目标代码运行时的指针的名字
 * 	offset0表示从ref指向的位置开始，跳过多少偏移量去找真正的数据
 * 	elem_type是数据元素的类型，包括整数、浮点数、simd向量等
 * 	endian是字节序
 * 	elem_size是每个元素的大小，以字节为单位。与elem_type配合，可以知道具体的数据类型。
 * 	n_dim指数据的维度。
 * 		datadesc所表达的数据是张量，则一般包含多个维度，每个维度包含若干指标，抽象上可以看成是各维度张开的笛卡尔积。
 * 	dims[]数组表示每个维度的信息，采用类型dimdesc_t。
 * 		与多维数组不同，datadesc所描述的数据并不一定在内存上连续分布，指标也不一定是从0或1开始的连续下标。因此需要针对每个维度和指标提供灵活的偏移量。
 * 		实际任一数据元素的存储位置在ref.p_+offset0+所有数据指标对应的偏移量
 * 		在dimdesc_t类型里：
 * 			n_tuple表示此处描述几个维度。一般来说是1，对于稀疏张量的情况，它可以是2或更多，表示在这些维度上是稀疏的。
 * 			dims[]数组给出相关维度的名字的索引，具体名字可以在全局数组dimnames_g里查到。
 *
 */


#include	<stdarg.h>


typedef	long		int_t;
typedef	unsigned long	uint_t;		// FIXME: 实际上最好不要使用uint_t，之前的部分代码已都换成int_t


typedef	enum
{
	region_none = 0,
	region_invalid,

	region_direct,

	region_main,
	region_sunway_ldm,
	region_tianhe_share,
	region_tianhe_local,
	region_tianhe_local_scalar,
	// ...

	region_sunway_ldm_cache,
	region_tianhe_share_cache,
	// ...

	region_last
} enum_region;

typedef	enum
{
	elem_type_raw = 0,
	elem_type_integer,
	elem_type_real,
	elem_type_integer_vector,
	elem_type_float_vector,
	// ...
	elem_type_last
} enum_elem_type;

typedef	enum
{
	endian_local = 0,

	endian_little,
	endian_big,
	endian_middle,
} enum_endian;

typedef	enum
{
	reduce_type_none = 0,
	reduce_type_sum,
	reduce_type_product,
	reduce_type_average,
	reduce_type_max,
	reduce_type_min,
	// ...
	reduce_type_last
} enum_reduce_type;


struct dimdesc_t
{
	enum_reduce_type	reduce_type;
	int_t	n_tuple;
	int_t	n_entry;
	int_t	*offsets;	// 这里的偏移量是以elem_size为单位的，与offset0不同。// FIXME: 需要仔细斟酌，这个设定是考虑到产生代码时不必来回换算，但在某些向量处理时是否有对齐问题？
	int_t	*indices;
	// C99 6.7.2.1.18 : 柔性数组，下同
	int_t	dims[];	// 存储dimnames_g的下标
	// 随后的内存以int_t或int_t为单位，共有n_tuple+n_entry+n_tuple*n_entry个，排列顺序为：
	// int_t	dims[n_tuple]
	// int_t	offsets[n_entry]
	// int_t	indices[n_entry][n_tuple]
};
typedef	struct dimdesc_t dimdesc_t;
typedef	dimdesc_t	* dimdesc;

struct datadesc_t
{
	enum_region	region;
	union
	{
		void *p_direct;
		char *p_refname;
	} ref;
	int_t	offset0;	// 单位是字节。可以用这个调整对齐。

	enum_endian	endian;
	enum_elem_type	elem_type;
	int_t	elem_size;

	int_t	n_dim;
	dimdesc	dims[];	// 数组可用长度为n_dim，但不必全占满，有稀疏情况时，后面冗余内存可以设置为NULL
					// NOTE: 所有不为NULL的dim必须置于第一个NULL之前
};
typedef	struct datadesc_t datadesc_t;
typedef	datadesc_t	* datadesc;

struct datadesc_set_t
{
	int_t		n;
	// 设计用于block2data_dense
	int_t		data_start_index;
	datadesc	p[];	// 数组长度为n
};
typedef	struct datadesc_set_t datadesc_set_t;
typedef	datadesc_set_t	* datadesc_set;

struct datadesc_set_set_t {
	int_t n;
	datadesc_set p[];
};
typedef struct datadesc_set_set_t datadesc_set_set_t;
typedef datadesc_set_set_t * datadesc_set_set;

typedef enum
{
	merge_type_1step,
	merge_type_1dim,
} enum_merge_info_type;

// 这里的dims_cnt和dims[]都指tuple
struct merge_info_piece_t {
	enum_merge_info_type merge_info_type;
	int_t data_index1;
	int_t data_index2;
	int_t dims_cnt;
	int_t dims[];
};
typedef struct merge_info_piece_t merge_info_piece_t;
typedef merge_info_piece_t * merge_info_piece;

/*
 * 以下是针对datadesc数据描述的操作函数：
 *
 * dimdesc_new是用于产生一个新的dimdesc_t对象；datadesc_new是用于产生一个新的datadesc_t对象。
 * 	考虑使用自动的垃圾处理方案，所以所有malloc得到的内存都不必free。
 * 	返回的对象的大部分成员还需要调用者去初始化。
 *
 * datadesc_normalize用于对datadesc内的成员进行整理：
 * 	对dims[]里的维度进行排序：从外圈到内圈，或至少使最大偏移量递减
 * 	对dims[]里的每个dimdesc里的offsets[]数组进行排序，按偏移量递增储存。同时，把最小偏移量加到offset0里，并把各偏移量减去这个最小偏移量，使最小偏移量总为0。
 * 	TODO: 对于符合笛卡尔积特点的稀疏维度，把它们整合成非稀疏的维度
 * 		这个操作应先于排序完成
 * 		如果有两个维度在一个tuple里，满足如下条件，则符合笛卡尔积的特点：
 * 			设这两个维度的指标各有m和n个，对于其他维度的指标的任何取值，这两个维度的指标都有m*n个不同取值
 * 			存在确定的函数f()和g()，对于这两个维度的每组指标i和j，使offset的值为C+f(i)+g(j)，其中C是与其他维度的指标有关的常数（即：其他维度的指标相同时，C是常数）
 *
 * datadesc_find_common用于寻找若干datadesc的公共指标集合。
 * 	对应于输入的n个datadesc，返回包含n个新的datadesc的指针数组（数组本身也不需要free）。
 * 	每个输出的datadesc与对应的输入相比，所有维度都保留（不增加也不减少维度），对每个维度而言，输出的指标集合是输入指标集合的子集。
 * 	对于所有输出的datadesc，他们之间如果有相同的维度，则他们在这个维度上的指标集合相同。
 * 		换句话说：这个函数是对每个维度的指标集合寻找最大公共子集。
 *
 * datadesc_find_common_set用于寻找若干datadesc_set的公共指标集合。
 * 	此时的每个datadesc_set被看作一个整体，其中的datadesc可以看作其不同的部分，它们的维度和指标的并集构成了整个datadesc_set的维度和指标
 * 		NOTE: 这意味着在同一个datadesc_set中的各个datadesc的维度和指标应当满足一定的要求，比如可能的指标集合保证无重复等。
 * 	关于维度和指标的common规则，类似datadesc_find_common，只是这个函数以datadesc_set为对象，也要返回n个datadesc_set。
 * 	TODO: 有个问题需要想清楚：结果返回的n个datadesc_set里的datadesc及其dimdesc里的tuple结构，一定是一样的吗？
 *
 * datadesc_pick用于提取数据的一部分。
 * 	输入一个datadesc，输出一个新的datadesc。
 * 	维度dim包含在输入中，不包含在输出中。
 * 	相当于对输入的datadesc做了一个slice操作，提取其指定的维度的指定指标。
 *
 * datadesc_dimmap用于对数据描述中的维度和指标进行变换，其中需要输入类型为dimmap_f的回调函数
 * 	dims_in[]中的所有维度都必须是p中含有的，否则返回NULL。
 * 	dims_out[]中的任何维度要么是在dims_in[]中包含，要么是在p中不存在，否则返回NULL。这是为了避免存在维度冲突
 * 	对于所有在p中有效的indices_in[]组合，f返回的indices_out[]不可以有完全相同的，否则返回NULL。这是为了避免存在指标冲突
 * 	dimmap_f正常应当返回非零值，如果返回0，则扔掉此数据点。
 *
 * datadesc_set_show用可读的格式向stdout打印输入的datadesc_set对象，主要用于调试。
 *
 * datadesc_set_(de)serialize是一对序列化与反序列化函数，用于把dadadesc_set对象和一个连续的内存打包对象相互转换。
 * 	内存打包对象的前8个字节是头，后续跟随数据
 * 		字节0是用于判断endian的标记，类型兼容enum_endian
 * 		字节1-3保留，目前置0
 * 		字节4-7是一个32-bit unsigned int类型，指定整个内存打包对象的长度（包括头）
 * 		可以在实现中定义更多字节的头，保证前8个字节兼容即可
 * 	实现时，注意dimname的处理
 * 	实现时，region_direct的情况可以暂时先不实现，这个有些复杂，留作后续实现
 * 	实现时，endian可以先不管，不兼容x86的暂时留待以后实现
 *
 * datadesc_set_merge把p中可以合并的datadesc合并起来，无法合并的保持原样
 * 	如果两个datadesc所指的对象，其他各成员变量均相同，dims[]里有n_dim-1个dimdesc_t相同，另外一个dimdesc的维度tuple相同但indices不同，则两个datadesc可以合并。
 * 		此处所说的dimdesc的比较，是指其内容本质相同，不需要dimdesc_t对象是同一个。
 *
 */


int_t		dimname_new(const char *p);
const char *	dimname(int_t index);
int_t		dimtmp_new();

dimdesc		dimdesc_new(int_t n_tuple, int_t n_entry);
dimdesc		dimdesc_copy(dimdesc p);
void		dimdesc_set_entry(dimdesc p, int_t entry, int_t offset, ...);
datadesc	datadesc_new(int_t n_dim);
datadesc	datadesc_new_array(enum_region region, void *p_ref, enum_endian endian, enum_elem_type elem_type, int_t elem_size, int_t n_dim, char **p_dimname, int_t *p_dimlength);
datadesc_set	datadesc_set_new(int_t n);

datadesc	datadesc_normalize(datadesc p);

datadesc	datadesc_pick(datadesc p, int_t dim, int_t index);

typedef	int_t	dimmap_f(int_t *indices_out, int_t *indices_in);
datadesc	datadesc_dimmap(datadesc p, dimmap_f f, int_t n_out, int_t *dims_out, int_t n_in, int_t *dims_in);

datadesc_set	datadesc_find_common(datadesc_set p);
datadesc_set *	datadesc_find_common_set(int_t n, datadesc_set p[]);
datadesc	datadesc_find_span(datadesc_set p);

datadesc_set	datadesc_set_merge(datadesc_set p);

void		datadesc_set_show(datadesc_set p);

void *		datadesc_set_serialize(datadesc_set p);
datadesc_set	datadesc_set_deserialize(void *p);
int_t		query_dims_cnt(datadesc p);

datadesc datadesc_merge_dimb2a(datadesc data, int dima , int dimb);
void datadesc_show(datadesc p);

#endif
