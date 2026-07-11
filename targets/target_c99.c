#include	<stdio.h>
#include	<string.h>

#include	"target_c99.h"
#include	"gc_malloc.h"
#include	"plugins.h"


static	enum_endian	query_endian(target_c99 _this)
{
	return _this->endian;
}

static	void *	query_type(target_c99 _this, enum_elem_type type, int_t size)
{
	switch (type)
	{
		case elem_type_integer :
			switch (size)
			{
				case 1 :	return	"int8_t";
				case 2 :	return	"int16_t";
				case 4 :	return	"int32_t";
				case 8 :	return	"int64_t";
				default :	return	NULL;
			};
		case elem_type_real :
			switch (size)	// TODO: 此处需要斟酌C99标准的规定和实现的平台，是否需要long double？
			{
				case 4 :	return	"float";
				case 8 :	return	"double";
				default :	return	NULL;
			};
		default :	return	NULL;
	}
}


static	void *	alloc(target_c99 _this, int_t size)
{
	char	* res = token_new();

	emit_start((target)_this);
	emit_push("void *", res, "=malloc(", mt_itoa(size), ");");
	emit_finish();

	return	res;
}


static	datadesc	create_data(target_c99 _this, void *p)
{
	// TODO
	// p是一个类似"double[5][3][10]"样子的字符串
	// 变量名字自动产生
}

static	int_t	check_point(target_c99 _this)
{
	target_c99_buf_c	* p = malloc(sizeof(target_c99_buf_c));

	p->ckpt = index_new();
	p->buf[0] = 0;
	p->prev = _this->pbuf;
	_this->pbuf = p;

	return	p->ckpt;
}

static	void	push(target_c99 _this, void *p)
{
	if (strlen(_this->pbuf->buf)+strlen(p) < TARGET_C99_BUF_SIZE)
		strcat(_this->pbuf->buf, p);
	else
	{
		int ncopy = TARGET_C99_BUF_SIZE-strlen(_this->pbuf->buf)-1;
		strncat(_this->pbuf->buf, p, ncopy);

		target_c99_buf_c	* pb = malloc(sizeof(target_c99_buf_c));

		pb->ckpt = 0;
		pb->buf[0] = 0;
		pb->prev = _this->pbuf;
		_this->pbuf = pb;

		push(_this, (char *)p+ncopy);
	}
}

static	void	roll_back(target_c99 _this, int_t ckpt)
{
	target_c99_buf_c	* p = _this->pbuf;

	while (p && p->ckpt != ckpt)
		p = p->prev;

	if (p)
		_this->pbuf = p->prev;
	else
		fprintf(stderr, "Error : roll_back to a non-existing check point in file %s at line %d.\n", __FILE__, __LINE__);
}

static	void	commit(target_c99 _this, int_t ckpt)
{
	;	// 好像什么都不用做？
}


target	target_c99_new()
{
	// NOTE: 编译器会在这个函数报告一些警告，是安全的。见C99 6.7.2.1.13。

	target_c99	_this = malloc(sizeof(target_c99_t));
	target		_base = & _this->base;	// NOTE: _this 和 _base 应该是严格相等的

	_base->target_type = target_type_c_macro;

	// 填充虚函数表
	_base->query_endian = query_endian;
	_base->query_type = query_type;
	_base->alloc = alloc;
	_base->create_data = create_data;
	_base->check_point = check_point;
	_base->push = push;
	_base->roll_back = roll_back;
	_base->commit = commit;

	_this->endian = endian_little;	// FIXME: 暂时设置为小端，以后可以考虑根据实际的运行平台动态设置。
	_this->pbuf = NULL;

	// 注册datacopy处理函数，后注册的优先被匹配。
	_base->datacopy_p = NULL;
	target_register_datacopy(_base, datacopy_foreach);
	target_register_datacopy(_base, datacopy_121_reference);
	//target_register_datacopy(_base, datacopy_121_memcpy);
	//target_register_datacopy(_base, datacopy_121_template);

	_base->block2data_p = NULL;
	target_register_block2data(_base, block2data_arithmetic);
	target_register_block2data(_base, block2data_sparse_const);

	_base->optimize_p = NULL;
	target_register_optimize(_base, optimize_sparse_const);

	_base->compile_p = NULL;


	return	_base;
}

void	target_c99_dump(target t, FILE *fp)
{
	target_c99_buf_c *	pbuf;

	while ((pbuf = ((target_c99)t)->pbuf) && pbuf->prev)
	{
		while (pbuf->prev->prev)
			pbuf = pbuf->prev;
		fprintf(fp, "%s", pbuf->prev->buf);
		pbuf->prev = NULL;		// NOTE: 输出完之后顺便清空，是否合适？
	}
	if (pbuf)
	{
		fprintf(fp, "%s", pbuf->buf);
		((target_c99)t)->pbuf = NULL;
	}
	fprintf(fp, "\n");

	return;
}
