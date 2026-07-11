
#include	"target_base.h"
#include	"gc_malloc.h"


int_t	datacopy(target t, datadesc_set dst, datadesc_set src)
{
	function_c	* p;

	// TODO: 合法性检查：
	// 	0、集合为空等平凡问题。是否应放到具体实现中检查？
	// 	1、每个目标位置对应的非reduce的源位置不可多于一个。是否应当定义为UB而不做检查？
	// 	2、reduce类型不匹配。是否应放到具体实现中检查？
	// 	3、……

	p = t->datacopy_p;
	while (p && !((datacopy_f *)p->f)(t, dst, src))
		p = p->next;

	return	!!p;
}

datadesc_set	block2data(target t, block ob)
{
	function_c	* p;
	datadesc_set	res;

	p = t->block2data_p;
	while (p && !(res = ((block2data_f *)p->f)(t, ob)))
		p = p->next;

	return	res;
}

block	optimize(target t, block ob)
{
	function_c	* p;

	p = t->optimize_p;
	while (p)
	{
		ob = ((optimize_f *)p->f)(t, ob);
		p = p->next;
	}

	return	ob;
}

int_t	compile(target t, block ob)
{
	function_c	* p;

	p = t->compile_p;
	while (p && !((compile_f *)p->f)(t, ob))
		p = p->next;

	return	!!p;
}

void	target_register_datacopy(target t, datacopy_f *f)
{
	function_c	* p;

	p = malloc(sizeof(function_c));
	p->f = (void (*)())f;
	p->next = t->datacopy_p;
	t->datacopy_p = p;
}

void	target_register_block2data(target t, block2data_f *f)
{
	function_c	* p;

	p = malloc(sizeof(function_c));
	p->f = (void (*)())f;
	p->next = t->block2data_p;
	t->block2data_p = p;
}

void	target_register_optimize(target t, optimize_f *f)
{
	function_c	* p;

	p = malloc(sizeof(function_c));
	p->f = (void (*)())f;
	p->next = t->optimize_p;
	t->optimize_p = p;
}

void	target_register_compile(target t, compile_f *f)
{
	function_c	* p;

	p = malloc(sizeof(function_c));
	p->f = (void (*)())f;
	p->next = t->compile_p;
	t->compile_p = p;
}


int_t	index_new()
{
	static	int_t	index = 0;

	index ++;	// FIXME: 应该检查溢出

	return index;
}

char *	token_new()
{
	int_t	i = index_new();
	size_t	s = snprintf(NULL, 0, "var_%lu_", i);	// TODO: “%ld”与int_t的定义需要配合

	if (s<0)	//FIXME: 什么情况下会出这个问题？另外，snprintf()似乎返回的长度不包括'\0'，需要仔细搞清楚标准。
		return	NULL;

	char	* p = malloc(s+1);

	snprintf(p, s+1, "var_%lu_", i);

	return	p;
}

char *	mt_itoa(int_t i)
{
	size_t	s = snprintf(NULL, 0, "%lu", i);

	if (s<0)
		return	NULL;

	char	* p = malloc(s+1);

	snprintf(p, s+1, "%lu", i);

	return	p;
}





