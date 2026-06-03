
#include	"plugins.h"


// 目前这个函数的功能是实现如下的公式：
// 	[U] - ( a & b ) => ( [U] - a ) & ( [U] - b )
// 	[U] - ( a - b ) => ( [U] - a ) - b
//		FIXME: a = [U] || a = [R]时，不处理；或者说，当且仅当b = [G]时需要处理？
//
// NOTE: 这个函数只需要处理ob->p.b[0]位置的这一个一元运算即可，它可以配合optimize_recursion实现全树的搜索处理

block	map_unary(target t, block ob)
{
	if ( ob->type == block_type_con_ser && block_builtin_unary_first < ob->p.b[0]->type && ob->p.b[0]->type < block_builtin_unary_last )
	{
		if ( ob->p.b[1]->type == block_type_con_par_int )
			return	block_new(block_type_con_par_int,
					block_new(block_type_con_ser, ob->p.b[0], ob->p.b[1]->p.b[0]),
					block_new(block_type_con_ser, ob->p.b[0], ob->p.b[1]->p.b[1]));
		else if ( ob->p.b[1]->type == block_type_con_ser && 
			!(block_builtin_unary_first < ob->p.b[1]->p.b[0]->type && ob->p.b[1]->p.b[0]->type < block_builtin_unary_last) && 
			!(block_builtin_reduce_first < ob->p.b[1]->p.b[0]->type && ob->p.b[1]->p.b[0]->type < block_builtin_reduce_last) )
			return	block_new(block_type_con_ser,
					block_new(block_type_con_ser, ob->p.b[0], ob->p.b[1]->p.b[0]),
					ob->p.b[1]->p.b[1]);
		else
			return	ob;
	}
	else
		return	ob;
}

block	optimize_map_unary(target t, block ob)
{
	block	p0, p1;
	
	ob = map_unary(t, ob);
	
	
	switch (ob->type)
	{
		case block_type_box :
			p0 = optimize_map_unary(t, ob->p.u);
			if (p0 != ob->p.u)
				return	optimize_map_unary(t, block_new(ob->type, p0));		// NOTE: 此处故意保持原block只读，为了不给未来并行优化惹麻烦
			break;
		case block_type_con_ser :
		case block_type_con_par_int :
		case block_type_con_par_uni :
			p0 = optimize_map_unary(t, ob->p.b[0]);
			p1 = optimize_map_unary(t, ob->p.b[1]);
			if (p0 != ob->p.b[0] || p1 != ob->p.b[1])
				return	optimize_map_unary(t, block_new(ob->type, p0, p1));
			break;
	}

	return ob;
}
