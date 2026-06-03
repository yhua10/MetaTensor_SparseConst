
#include	"plugins.h"
#include	"block.h"

// 目前这个函数的功能是实现如下的乘法分配律公式：
// 	[+] - ( ( [*] - ( a & b ) ) - ( [n-] - [G] ) ) => [*] - ( a & ( [+] - ( b - ( [n-] - [G] ) ) ) )
// exchange:
//	[*] - ( a & ( [+] - ( b - ( [n-] - [G] ) ) ) )
// 	此公式仅在发现a的输入端口中不包含n时成立
//
// NOTE: 这个函数只需要处理ob位置的情况即可，它可以配合optimize_recursion实现全树的搜索处理

block	common_factor(target t, block ob)
{
	// TODO
	//return block_new(block_builtin_reduce_sum);
	printf("optimize comon factor");
	if(ob->type==block_type_con_ser
	&& ob->p.b[0]->type == block_builtin_reduce_sum 
	&& ob->p.b[1]->type == block_type_con_ser
	&& ob->p.b[1]->p.b[0]->type == block_type_con_ser 
	&& ob->p.b[1]->p.b[0]->p.b[0]->type == block_builtin_reduce_prod
	&& ob->p.b[1]->p.b[0]->p.b[1]->type == block_type_con_par_int
	&& ob->p.b[1]->p.b[0]->p.b[1]->p.b[0]->type == block_type_datadesc_set 
	&& ob->p.b[1]->p.b[0]->p.b[1]->p.b[1]->type == block_type_datadesc_set 
	&& ob->p.b[1]->p.b[1]->type == block_type_con_ser
	&& ob->p.b[1]->p.b[1]->p.b[0]->type == block_type_wire
	&& ob->p.b[1]->p.b[1]->p.b[1]->type == block_builtin_generator_all 
	)
	{
	block wire_temp = ob->p.b[1]->p.b[1]->p.b[0];
	block a_temp = ob->p.b[1]->p.b[0]->p.b[0];
	block b_temp = ob->p.b[1]->p.b[0]->p.b[1];


	ob->p.b[0] = block_new(block_builtin_reduce_prod);
	ob->p.b[1] = block_new(block_type_con_par_int,
								block_new(block_type_datadesc_set,a_temp),
								block_new(block_type_con_ser,
									block_new(block_builtin_reduce_sum),
									block_new(block_type_con_ser,
										block_new(block_type_datadesc_set,b_temp),
										block_new(block_type_con_ser,
											wire_temp,
											block_new(block_builtin_generator_all))
										)
									)
							);


	return ob;							
	}

	
}


block	optimize_common_factor(target t, block ob)
{
	block	p0, p1;

	ob = common_factor(t,ob);
	switch (ob->type)
	{
		case block_type_box :
			p0 = optimize_common_factor(t, ob->p.u);
			if (p0 != ob->p.u)
				return	optimize_common_factor(t, block_new(ob->type, p0));		// NOTE: 此处故意保持原block只读，为了不给未来并行优化惹麻烦
			break;
		case block_type_con_ser :
		case block_type_con_par_int :
		case block_type_con_par_uni :
			p0 = optimize_common_factor(t, ob->p.b[0]);
			p1 = optimize_common_factor(t, ob->p.b[1]);
			if (p0 != ob->p.b[0] || p1 != ob->p.b[1])
				return	optimize_common_factor(t, block_new(ob->type, p0, p1));
			break;
	}

	return ob;
}
