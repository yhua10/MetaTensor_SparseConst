
#include "plugins.h"

/*
 * 这个版本是对目前的reference版本的重写
 * 	由于采用了两遍扫描，效率变低了，这个是因为没仔细处理reduce的情况，可以后续优化回来
 * 	这个版本的主要目的是展示emit_for的用法
 * 	稀疏情况和其他一些优化，大部分可以挪到emit_for的实现中
 */

int_t	datacopy_121_template(target t, datadesc_set dst, datadesc_set src)
{
	datadesc_set	d;
	char **		idx;

	emit_start(t);
	emit_assert(t->target_type == target_type_c_macro);
	emit_assert(dst->n==1 && src->n==1);
	emit_assert(dst->p[0]->region==region_main);	// FIXME: 这里其实可以不必限制得如此保守，其实只要可以直接访问的内存区域，代码应该是通用的。
	emit_assert(src->p[0]->region==region_main);
	emit_assert(dst->p[0]->endian==src->p[0]->endian && src->p[0]->endian==target_query_endian(t));		// TODO: 这里需要增补实现

	emit_push("{");

	d = datadesc_set_new(2);
	d->p[0] = dst->p[0];
	d->p[1] = src->p[0];

	char *	dst_type = target_query_type(t, d->p[0]->elem_type, d->p[0]->elem_size);
	char *	src_type = target_query_type(t, d->p[1]->elem_type, d->p[1]->elem_size);
	emit_assert(dst_type && src_type);

	char *	t_dst_p = token_new();	// 目标位置的首地址
	char *	t_src_p = token_new();	// 源位置的首地址

	emit_push(dst_type, "*", t_dst_p, "=(", dst_type, "*)((char *)(", d->p[0]->ref.p_refname, ")+", mt_itoa(d->p[0]->offset0), ");");
	emit_push(src_type, "*", t_src_p, "=(", src_type, "*)((char *)(", d->p[1]->ref.p_refname, ")+", mt_itoa(d->p[1]->offset0), ");");

	emit_assert(emit_for_begin(t, d, &idx));
	emit_push(t_dst_p, "[", idx[0], "]=0;");	// FIXME: 这里显然有优化空间，不知道编译器是否能自动发现，最好还是应该仔细处理一下。
	emit_assert(emit_for_end(t, d));

	emit_assert(emit_for_begin(t, d, &idx));
	emit_push(t_dst_p, "[", idx[0], "]+=", t_src_p, "[", idx[1], "];");	// TODO: 目前只实现了sum，其他也可以类似实现，需要增补。
	emit_assert(emit_for_end(t, d));

	emit_push("}");

	emit_finish();
	return	1;
}


