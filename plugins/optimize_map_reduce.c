
#include	"plugins.h"


// 目前这个函数的功能是实现如下的公式：
// 	[R] - ( a & b ) => [R] - ( ( [R] - a ) & ( [R] - b ) )
//	FIXME: ( [R] - ( ( a & b ) - ( [* - -1] - [G] ) ) )，也需要处理
// NOTE: 这个函数只需要处理ob->p.b[0]位置的这一个一元运算即可，它可以配合optimize_recursion实现全树的搜索处理
//		但是，和optimize_map_unary有本质区别：reduce是复制，unary是剪切，因此reduce不能产生临时的[R]
// FIXME: 如果上式的a或b正好是([R]-c)的样子，就不必重复添加[R]了

int_t	need_allocate_reduce(block ob) {
	if (ob->type == block_type_datadesc_set) {
		// FIX：目前的构想是统一规范到[R] - D，或许利于代码生成
		return 1;
	}
	if (ob->type == block_type_con_par_int) {
		return 1;
	}
	if (ob->type == block_type_con_ser) {
		// FIXME: 递归判断，避免 [R] - ( [R] - c )，是否有必要
		if (ob->p.b[0]->type > block_builtin_reduce_first && ob->p.b[0]->type < block_builtin_reduce_last) {
			return 0;
		} 
		else if (ob->p.b[0]->type > block_builtin_unary_first && ob->p.b[0]->type < block_builtin_unary_last) 
		{
			return need_allocate_reduce(ob->p.b[1]);
		}
		else if (ob->p.b[1]->type == block_type_con_ser && ob->p.b[1]->p.b[0]->type == block_type_wire) 
		{
			return need_allocate_reduce(ob->p.b[0]);
		}
	}
	return 1;
}

block	optimize_map_reduce_allocate(enum_block_type type, block ob)
{
	if ( ob->type == block_type_con_par_int )
	{
		block blk0, blk1;
		if (need_allocate_reduce(ob->p.b[0])) {
			blk0 = block_new(block_type_con_ser, block_new(type), ob->p.b[0]);
		} else {
			blk0 = ob->p.b[0];
		}
		if (need_allocate_reduce(ob->p.b[1])) {
			blk1 = block_new(block_type_con_ser, block_new(type), ob->p.b[1]);
		} else {
			blk1 = ob->p.b[1];
		}
		return	block_new(block_type_con_par_int, 
					blk0, 
					blk1);
	}
	else
		return	ob;
}

block	map_reduce(target t, block ob)
{
	if ( ob->type == block_type_con_ser && block_builtin_reduce_first < ob->p.b[0]->type && ob->p.b[0]->type < block_builtin_reduce_last)
	{
		if (ob->p.b[1]->type == block_type_con_par_int) {
			ob->p.b[1] = optimize_map_reduce_allocate(ob->p.b[0]->type, ob->p.b[1]);
			return ob;
		} 
		// ( [R] - ( ( ( a & b ) - ( [* - -1] - [G] ) ) - ( [* - -1] - [G] ) ) )
		// FIXME: 经过optimize_map_unary.c，[U]不会出现在 ( a & b ) 外部?
		if (ob->p.b[1]->type == block_type_con_ser &&
			 (ob->p.b[1]->p.b[1]->type == block_type_con_ser && ob->p.b[1]->p.b[1]->p.b[0]->type == block_type_wire && ob->p.b[1]->p.b[1]->p.b[1]->type == block_builtin_generator_all)) 
		{
			enum_block_type type = ob->p.b[0]->type;
			block tmp_ob = ob->p.b[1];
			while (tmp_ob->p.b[0]->type == block_type_con_ser && tmp_ob->p.b[0]->p.b[1]->type == block_type_con_ser && 
				(tmp_ob->p.b[0]->p.b[1]->p.b[0]->type == block_type_wire && tmp_ob->p.b[0]->p.b[1]->p.b[1]->type == block_builtin_generator_all)) 
			{
				tmp_ob = tmp_ob->p.b[0];
			}
			
			if (tmp_ob->p.b[0]->type == block_type_con_par_int) 
			{
				tmp_ob->p.b[0] = optimize_map_reduce_allocate(type, tmp_ob->p.b[0]);
			}
			
		}
		
	}

	return	ob;
}

block	optimize_map_reduce(target t, block ob)
{
	block	p0, p1;

	ob = map_reduce(t, ob);

	switch (ob->type)
	{
		case block_type_box :
			p0 = optimize_map_reduce(t, ob->p.u);
			if (p0 != ob->p.u)
				return	optimize_map_reduce(t, block_new(ob->type, p0));		// NOTE: 此处故意保持原block只读，为了不给未来并行优化惹麻烦
			break;
		case block_type_con_ser :
		case block_type_con_par_int :
		case block_type_con_par_uni :
			p0 = optimize_map_reduce(t, ob->p.b[0]);
			p1 = optimize_map_reduce(t, ob->p.b[1]);
			if (p0 != ob->p.b[0] || p1 != ob->p.b[1])
				return	optimize_map_reduce(t, block_new(ob->type, p0, p1));
			break;
	}

	return ob;
}


