#ifndef	F_TARGET_BASE_H_
#define F_TARGET_BASE_H_

#include	"datadesc.h"
#include	"block.h"

#include	<stdio.h>	// for emit_log


typedef	struct	function_c	function_c;
struct	function_c
{
	function_c	* next;
	void (*f)();	// NOTE: 根据C99标准6.2.5，数据指针和函数指针大小可能不同；根据6.3.2.3，函数指针可以通用。
};

typedef	struct	target_t	target_t;
typedef	target_t	* target;

struct target_t
{
	enum
	{
		target_type_c_macro,
		// ...
		target_type_last
	} target_type;

	// 模板匹配链表
	function_c	* datacopy_p;
	function_c	* block2data_p;
	function_c	* optimize_p;
	function_c	* compile_p;

	// 虚函数表

	enum_endian	(*query_endian)(target _this);
	void *	(*query_type)(target _this, enum_elem_type type, int_t size);	// 无对应类型则返回0

	void *		(*alloc)(target _this, int_t size);	// 返回一个运行时名字或其他标记
	datadesc	(*create_data)(target _this, void *p);	// 定义运行时数据，并装入当前代码

	int_t	(*check_point)(target _this);
	void	(*push)(target _this, void *p);
	void	(*roll_back)(target _this, int_t ckpt);
	void	(*commit)(target _this, int_t ckpt);

};

#define	target_query_endian(t)		(((target)t)->query_endian(t))
#define	target_query_type(t, type, size)(((target)t)->query_type(t, type, size))
#define	target_create_data(t, p)	(((target)t)->create_data((t), (p)))
#define	target_check_point(t)		(((target)t)->check_point(t))
#define	target_push(t, p)		(((target)t)->push((t), (p)))
#define	target_roll_back(t, ckpt)	(((target)t)->roll_back((t), (ckpt)))
#define	target_commit(t, ckpt)		(((target)t)->commit((t), (ckpt)))

// 这几个宏是用来在datacopy等重载函数的具体实现中使用的，用于判断匹配和生成代码的控制。
// NOTE: 注意这里有两个露在外面的临时变量_emit_ckpt和_emit_target，这意味着在同一个函数里最好只出现一次emit_start()，且在其他相关宏之前。
#define	emit_log()		do { fprintf(stderr, "EMIT LOG : Assert Failed in file %s at line #%d.\n", __FILE__, __LINE__);fflush(stderr);} while (0)
//#define	emit_log()	// 也可以关闭掉log
#define	emit_start(t)		int_t _emit_ckpt = target_check_point(t); target _emit_target = (t);
#define	emit_assert(b)		do { if (!(b)) { target_roll_back((_emit_target), _emit_ckpt); emit_log(); return 0; }} while (0)
#define	emit_push(...)		do { char *_list[]={__VA_ARGS__}; int_t _i; for (_i=0; _i<sizeof(_list)/sizeof(char*); _i++) target_push(_emit_target, _list[_i]); } while (0)
#define	emit_finish()		do { target_commit(_emit_target, _emit_ckpt); } while (0)


typedef	int_t	datacopy_f(target t, datadesc_set dst, datadesc_set src);	// 返回0表示不支持功能，并不做任何操作；返回非零值表示已成功发射代码。
datacopy_f	datacopy;
void	target_register_datacopy(target t, datacopy_f *f);


typedef	datadesc_set	block2data_f(target t, block ob);
block2data_f	block2data;
void	target_register_block2data(target t, block2data_f *f);


typedef	block	optimize_f(target t, block ob);
optimize_f	optimize;
void	target_register_optimize(target t, optimize_f *f);


typedef	int_t	compile_f(target t, block ob);
compile_f	compile;
void	target_register_compile(target t, compile_f *f);



/*
 * 一些辅助功能
 */

int_t	index_new();	// start from 1

char *	token_new();

char *	itoa(int_t i);




#endif
