
#include "plugins.h"

/*
 * 目前版本实现了在主内存上的：
 * 	拷贝、转置、类型转换、求和归约、广播，以及这些操作的组合
 * 	输出代码格式为C语言的语句块
 * 目前版本尚未实现的功能包括：
 * 	跨主内区域的操作：不在此实现
 * 	异步支持：不在此实现，但可以fork到另外一个实现
 *	endian转换：可以增补
 *	稀疏情况：可以增补——优先实现
 * 目前版本的优化空间：
 * 	非一对一的拷贝：不在此实现
 * 	大块内存合并拷贝：不在此实现
 * 	并行等与硬件相关的深度优化：不在此实现
 * 	偏移量数组转换成简单运算：可以增补
 * 	除求和外的其他归约：可以增补
 */

int_t	datacopy_121_reference(target t, datadesc_set dst, datadesc_set src)
{
	datadesc_set	d;
	int_t		n_reduce = 0;
	int_t		i_dst, i_src;
	int_t		i, j;

	emit_start(t);
	emit_assert(t->target_type == target_type_c_macro);
	emit_assert(dst->n==1 && src->n==1);
	emit_assert(dst->p[0]->region==region_main);	// FIXME: 这里其实可以不必限制得如此保守，其实只要可以直接访问的内存区域，代码应该是通用的。
	emit_assert(src->p[0]->region==region_main);
	emit_assert(dst->p[0]->endian==src->p[0]->endian && src->p[0]->endian==target_query_endian(t));		// TODO: 这里需要增补实现

	d = datadesc_set_new(2);
	d->p[0] = dst->p[0];
	d->p[1] = src->p[0];
	d = datadesc_find_common(d);
	d->p[0] = datadesc_normalize(d->p[0]);
	d->p[1] = datadesc_normalize(d->p[1]);
	const int_t dst_ndim = query_dims_cnt(d->p[0]);
	const int_t src_ndim = query_dims_cnt(d->p[1]);

	char *	int_type = target_query_type(t, elem_type_integer, 4);	// NOTE: 循环变量和偏移量的类型，目前默认是 32bit signed integer
	char *	dst_type = target_query_type(t, d->p[0]->elem_type, d->p[0]->elem_size);
	char *	src_type = target_query_type(t, d->p[1]->elem_type, d->p[1]->elem_size);
	emit_assert(int_type && dst_type && src_type);

	emit_push("{");

	char *	t_dst_p = token_new();	// 目标位置的首地址
	char *	t_src_p = token_new();	// 源位置的首地址
	char *	t_dst_i = token_new();	// 循环中的目标位置下标，运行时使用若干变量名
	char *	t_src_i = token_new();	// 循环中的源位置下标，运行时使用若干变量名

	emit_push(dst_type, "*", t_dst_p, "=(", dst_type, "*)((char *)(", d->p[0]->ref.p_refname, ")+", mt_itoa(d->p[0]->offset0), ");");
	emit_push(src_type, "*", t_src_p, "=(", src_type, "*)((char *)(", d->p[1]->ref.p_refname, ")+", mt_itoa(d->p[1]->offset0), ");");
	emit_push(int_type, " ", t_dst_i, "=0;");
	emit_push(int_type, " ", t_src_i, "=0;");

	for (i_dst=0; i_dst<dst_ndim; i_dst++)	// NOTE: 对于每个目标维度，如果有对应源维度，则是正常拷贝，否则是广播。
	{
		dimdesc	d_dst = d->p[0]->dims[i_dst];

		emit_assert(d_dst->n_tuple==1);	// TODO: 这里先实现稠密的情况，稀疏的需要增补
		emit_assert(d_dst->n_entry>0);	// FIXME: 如果无数据可传输，是否应当回退并返回1？此处设计需要斟酌

		char *	t_dst_offset_p = token_new();	// 每个维度的各指标的偏移量列表
		char *	t_src_offset_p = token_new();

		// TODO: 这里的偏移量列表有可优化的潜力，比如等差数列等可以直接计算而不查表。
		emit_push(int_type, " ", t_dst_offset_p, "[", mt_itoa(d_dst->n_entry), "]={", mt_itoa(d_dst->offsets[0]));
		for (i=1; i<d_dst->n_entry; i++)
			emit_push(",", mt_itoa(d_dst->offsets[i]));
		emit_push("};");

		// FIXME: 这里没有完整考虑稀疏的情况，需要额外处理
		for (i_src=0; i_src<src_ndim && d_dst->dims[0]!=d->p[1]->dims[i_src]->dims[0]; i_src++);
		if (i_src<src_ndim)
		{
			dimdesc	d_src = d->p[1]->dims[i_src];

			emit_assert(d_src->n_tuple==1);
			emit_assert(d_dst->n_entry==d_src->n_entry);
			emit_push(int_type, " ", t_src_offset_p, "[", mt_itoa(d_src->n_entry), "]={");
			for (i=0; i<d_dst->n_entry; i++)
			{
				for (j=0; j<d_src->n_entry && d_dst->indices[i*d_dst->n_tuple]!=d_src->indices[j*d_src->n_tuple]; j++);
				emit_assert(j<d_src->n_entry);
				if (i>0)
					emit_push(",");
				emit_push(mt_itoa(d_src->offsets[j]));
			}
			emit_push("};");
		}

		char *	t_offset_i = token_new();	// 每个维度对指标的循环变量

		emit_push(int_type, " ", t_offset_i, ";");
		emit_push("for(", t_offset_i, "=0;", t_offset_i, "<", mt_itoa(d_dst->n_entry), ";", t_offset_i, "++){");

		char *	t_tmp = token_new();
		emit_push(int_type, " ", t_tmp, "=", t_dst_i, "+", t_dst_offset_p, "[", t_offset_i, "];");
		t_dst_i = t_tmp;
		if (i_src<src_ndim)
		{
			t_tmp = token_new();
			emit_push(int_type, " ", t_tmp, "=", t_src_i, "+", t_src_offset_p, "[", t_offset_i, "];");
			t_src_i = t_tmp;
		}
	}

	char *	t_acc = token_new();
	emit_push(dst_type, " ", t_acc, "=0;");

	for (i_src=0; i_src<src_ndim; i_src++)	// NOTE: 如果在源中有在目标中不存在的维度，则要归约。
	{
		for (i_dst=0; i_dst<dst_ndim && d->p[0]->dims[i_dst]->dims[0]!=d->p[1]->dims[i_src]->dims[0]; i_dst++);
		if (i_dst==dst_ndim)
		{
			dimdesc	d_src = d->p[1]->dims[i_src];

			emit_assert(d_src->n_tuple==1);
			emit_assert(d_src->n_entry>0);		// FIXME: 如果源数据有空维度应该怎么处理？
			emit_assert(d_src->reduce_type==reduce_type_sum);	// TODO: 目前只实现了sum，其他也可以类似实现，需要增补。

			char *	t_src_offset_p = token_new();

			emit_push(int_type, " ", t_src_offset_p, "[", mt_itoa(d_src->n_entry), "]={", mt_itoa(d_src->offsets[0]));
			for (i=1; i<d_src->n_entry; i++)
				emit_push(",", mt_itoa(d_src->offsets[i]));
			emit_push("};");

			char *	t_offset_i = token_new();	// 每个维度对指标的循环变量

			emit_push(int_type, " ", t_offset_i, ";");
			emit_push("for(", t_offset_i, "=0;", t_offset_i, "<", mt_itoa(d_src->n_entry), ";", t_offset_i, "++){");

			char *	t_tmp = token_new();
			emit_push(int_type, " ", t_tmp, "=", t_src_i, "+", t_src_offset_p, "[", t_offset_i, "];");
			t_src_i = t_tmp;

			n_reduce ++;
		}
	}

	emit_push(t_acc, "+=", t_src_p, "[", t_src_i, "];");

	for (i=0; i<n_reduce; i++)
		emit_push("}");

	emit_push(t_dst_p, "[", t_dst_i, "]=", t_acc, ";");

	for (i_dst=0; i_dst<dst_ndim; i_dst++)
		emit_push("}");

	emit_push("}");

	emit_finish();
	return	1;
}


