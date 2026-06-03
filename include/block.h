#ifndef	F_BLOCK_H_
#define	F_BLOCK_H_

#include	<stdarg.h>

#include	"datadesc.h"


typedef	enum
{
	block_type_first = 0,

	block_type_box,			// ->p.u

	block_type_con_ser,		// ->p.b
	block_type_con_par_int,
	block_type_con_par_uni,

	block_type_datadesc_set,	// ->p.d

	block_type_wire,		// ->p.w

	block_type_int_t,		// ->p.n
	block_type_double,		// ->p.f
	// ...

	//block_type_wire_typed,
	//block_type_spl_in,
	//block_type_spl_out,
	//block_type_iter,

	// ...

	block_type_last,

	block_builtin_basic_first = 1024,

	block_builtin_con_ser,			// [-]	右侧的输出连到左侧的输入
	block_builtin_con_par_int,		// [&]
	block_builtin_con_par_uni,		// [+]

	block_builtin_wire,			// [name_out-name_in]
	block_builtin_wire_filter,		// [<3.5]
	block_builtin_wire_revert,		// [~]

	block_builtin_generator_all,		// [G]

	// ...

	block_builtin_basic_last,

	block_builtin_unary_first = 2048,
	block_builtin_unary_minus,		// [minus]	// FIXME: 这两个其实是相反数和倒数，也许需要换个名字，opposite/reciprocal？
	block_builtin_unary_div,		// [div]
	block_builtin_unary_exp,		// [exp], 下同
	block_builtin_unary_log,
	block_builtin_unary_sin,
	block_builtin_unary_cos,
	block_builtin_unary_tan,
	block_builtin_unary_atan,
	block_builtin_unary_last,
	
	block_builtin_reduce_first = 3072,
	block_builtin_reduce_sum,		// [sum]	// FIXME: 为了避免命名冲突，用文字而非字符表示？
	block_builtin_reduce_prod,		// [prod]
	block_builtin_reduce_max,		// [max], 下同
	block_builtin_reduce_min,
	block_builtin_reduce_mean,
	block_builtin_reduce_median,
	block_builtin_reduce_last,

	block_builtin_compare_first = 4096,
	block_builtin_compare_equal,
	block_builtin_compare_less,
	block_builtin_compare_more,
	block_builtin_compare_last,

	block_builtin_emit_first = 5120,
	block_builtin_emit_refget,		// [Rg] : 有一个空名输出，有名字分别为"name"和"index"的两个输入，代表输出代码中的数组读访问操作
	block_builtin_emit_refset,		// [Rs] : 无输出，有名字分别为"name"、"index"、"value"的三个输入，代表输出代码中的数组写访问操作
	block_builtin_emit_type_int,		// [Ti]
	block_builtin_emit_type_double,		// [Tr]
	block_builtin_emit_last,

	// ...

	block_builtin_last
} enum_block_type;

typedef	struct	block_t	block_t;
typedef	block_t	* block;
struct	block_t
{
	enum_block_type	type;
	union {
		block			u;	// 一元组合：装箱
		block			b[2];	// 二元组合：（求同/存异）并联、串联、条件导线
		datadesc_set 		d;	// 数据块
		int_t			w[2];	// 导线，0为输入名字，1为输出名字
		int_t			n;	// 整型常数块
		double			f;	// 实型常数块
	} p;
};	


block	block_new(enum_block_type t, ...);

void *	block_serialize(void *buf, int_t bufsize, block ob);
block	block_deserialize(void *buf);


void print_block(block b, int indent);
void print_data_block(block b, int indent);
const char* get_block_type_name(enum_block_type type);
#endif

