#ifndef	F_TARGET_C99_H_
#define F_TARGET_C99_H_

#include	"target_base.h"

#include	<stdio.h>


#define	TARGET_C99_BUF_SIZE	1000
typedef	struct	target_c99_buf_c	target_c99_buf_c;
struct	target_c99_buf_c
{
	target_c99_buf_c	* prev;
	int_t			ckpt;
	char			buf[TARGET_C99_BUF_SIZE];
};


typedef	struct	target_c99_t	target_c99_t;
struct target_c99_t
{
	target_t	base;	// C99 6.7.2.1.13 : base的地址和整个结构体的地址相同 ，因此可以强制转换指针类型，但会导致编译期警告

	enum_endian	endian;

	target_c99_buf_c	* pbuf;
};
typedef	target_c99_t	* target_c99;


target	target_c99_new();
void	target_c99_dump(target t, FILE *fp);


#endif
