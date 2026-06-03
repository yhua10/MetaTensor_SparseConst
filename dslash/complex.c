
#include	<stddef.h>	// for NULL

#include	"block.h"


// NOTE: 复数用"complex"维度表示，其中指标0表示实部、1表示虚部
block	block_complex_mult()
{
	static	datadesc_set	res = NULL;
	static	int	const_complex_mult[] = {1, -1, 1, 1};

	if (!res)
	{
		res = datadesc_set_new(1);
		res->p[0] = datadesc_new(1);
		res->p[0]->region = region_direct;
		res->p[0]->ref.p_direct = const_complex_mult;
		res->p[0]->offset0 = 0;
		res->p[0]->endian = endian_local;
		res->p[0]->elem_type = elem_type_integer;
		res->p[0]->elem_size = sizeof(int);
		res->p[0]->dims[0] = dimdesc_new(3, 4);
		res->p[0]->dims[0]->reduce_type = reduce_type_sum;
		res->p[0]->dims[0]->dims[0] = dimname_new("complex");
		res->p[0]->dims[0]->dims[1] = dimname_new("complex1");
		res->p[0]->dims[0]->dims[2] = dimname_new("complex2");
		res->p[0]->dims[0]->offsets[0] = 0;
		res->p[0]->dims[0]->offsets[1] = 1;
		res->p[0]->dims[0]->offsets[2] = 2;
		res->p[0]->dims[0]->offsets[3] = 3;
		res->p[0]->dims[0]->indices[0*3+0] = 0;
		res->p[0]->dims[0]->indices[0*3+1] = 0;
		res->p[0]->dims[0]->indices[0*3+2] = 0;
		res->p[0]->dims[0]->indices[1*3+0] = 0;
		res->p[0]->dims[0]->indices[1*3+1] = 1;
		res->p[0]->dims[0]->indices[1*3+2] = 1;
		res->p[0]->dims[0]->indices[2*3+0] = 1;
		res->p[0]->dims[0]->indices[2*3+1] = 0;
		res->p[0]->dims[0]->indices[2*3+2] = 1;
		res->p[0]->dims[0]->indices[3*3+0] = 1;
		res->p[0]->dims[0]->indices[3*3+1] = 1;
		res->p[0]->dims[0]->indices[3*3+2] = 0;
	}

	return	block_new(block_type_datadesc_set, res);
}

// 返回的block是做复数乘法的块，这个块有：
// 	一个名字为"a"的输入端口，接受一个块
// 	一个名字为"b"的输入端口，接受一个块
//	一个名字为空的输出端口，它输出一个块，是由两个输入的块构造的复数乘法结果
block	complex_mult()
{
	static	block	res = NULL;

	if (!res)
	{
		res = block_new(block_type_con_ser,						// [ + - ((* - (block_complex_mult & ((a - complex=complex1) & (b - complex=complex2)))) - ((complex1="" - G) & (complex2="" - G))) ] 
				block_new(block_builtin_con_ser),
				block_new(block_type_con_par_int,
					block_new(block_type_box,
						block_new(block_builtin_reduce_sum)),
					block_new(block_type_con_ser,					// [ (* - (block_complex_mult & ((a - complex=complex1) & (b - complex=complex2)))) - ((complex1="" - G) & (complex2="" - G)) ] 
						block_new(block_builtin_con_ser),
						block_new(block_type_con_par_int,
							block_new(block_type_con_ser,				// [ * - (block_complex_mult & ((a - complex=complex1) & (b - complex=complex2))) ]
								block_new(block_builtin_con_ser),
								block_new(block_type_con_par_int,
									block_new(block_type_box,
										block_new(block_builtin_reduce_prod)),
									block_new(block_type_con_ser,			// [ block_complex_mult & ((a - complex=complex1) & (b - complex=complex2)) ]
										block_new(block_builtin_con_par_int),
										block_new(block_type_con_par_int,
											block_new(block_type_box,
												block_complex_mult()),
											block_new(block_type_con_ser,		// [ (a - complex=complex1) & (b - complex=complex2) ]
												block_new(block_builtin_con_par_int),
												block_new(block_type_con_par_int,
													block_new(block_type_con_ser,	// [ a - complex=complex1 ]
														block_new(block_builtin_con_ser),
														block_new(block_type_con_par_int,
															block_new(block_type_wire, dimname_new(""), dimname_new("a")),		// =a
															block_new(block_type_box,						// [complex=complex1]
																block_new(block_type_wire, dimname_new("complex"), dimname_new("complex1"))))),
													block_new(block_type_con_ser,	// [ b - complex=complex2 ]
														block_new(block_builtin_con_ser),
														block_new(block_type_con_par_int,
															block_new(block_type_wire, dimname_new(""), dimname_new("b")),
															block_new(block_type_box,
																block_new(block_type_wire, dimname_new("complex"), dimname_new("complex2"))))))))))),
							block_new(block_type_con_ser,				// [ (complex1="" - G) & (complex2="" - G) ]
									block_new(block_builtin_con_par_int),
									block_new(block_type_con_par_int,
										block_new(block_type_con_ser,		// [ complex1="" - G ]
											block_new(block_builtin_con_ser),
											block_new(block_type_con_par_int,
												block_new(block_type_box,	// [ complex1="" ]
													block_new(block_type_wire, dimname_new("complex1"), dimname_new(""))),
												block_new(block_type_box,	// [ G ]
													block_new(block_builtin_generator_all)))),
										block_new(block_type_con_ser,		// [ complex2="" - G ]
											block_new(block_builtin_con_ser),
											block_new(block_type_con_par_int,
												block_new(block_type_box,
													block_new(block_type_wire, dimname_new("complex2"), dimname_new(""))),
												block_new(block_type_box,
													block_new(block_builtin_generator_all))))))))));
	}

	return	res;
}

block	do_complex_mult(block a, block b)
{
	return	block_new(block_type_con_ser,
			block_new(block_builtin_reduce_sum),
			block_new(block_type_con_ser,
				block_new(block_type_con_ser,
					block_new(block_builtin_reduce_prod),
					block_new(block_type_con_par_int,
						block_complex_mult(),
						block_new(block_type_con_par_int,
							block_new(block_type_con_ser,
								a,
								block_new(block_type_wire, dimname_new("complex"), dimname_new("complex1"))),
							block_new(block_type_con_ser,
								b,
								block_new(block_type_wire, dimname_new("complex"), dimname_new("complex2")))))),
				block_new(block_type_con_par_int,
					block_new(block_type_con_ser,
						block_new(block_type_wire, dimname_new("complex1"), dimname_new("")),
						block_new(block_builtin_generator_all)),
					block_new(block_type_con_ser,
						block_new(block_type_wire, dimname_new("complex2"), dimname_new("")),
						block_new(block_builtin_generator_all)))));
}

block	block_complex_conj()
{
	static	datadesc_set	res = NULL;
	static	int	const_complex_conj[] = {1, -1};

	if (!res)
	{
		res = datadesc_set_new(1);
		res->p[0] = datadesc_new(1);
		res->p[0]->region = region_direct;
		res->p[0]->ref.p_direct = const_complex_conj;
		res->p[0]->offset0 = 0;
		res->p[0]->endian = endian_local;
		res->p[0]->elem_type = elem_type_integer;
		res->p[0]->elem_size = sizeof(int);
		res->p[0]->dims[0] = dimdesc_new(1, 2);
		res->p[0]->dims[0]->reduce_type = reduce_type_sum;
		res->p[0]->dims[0]->dims[0] = dimname_new("complex");
		res->p[0]->dims[0]->offsets[0] = 0;
		res->p[0]->dims[0]->offsets[1] = 1;
		res->p[0]->dims[0]->indices[0] = 0;
		res->p[0]->dims[0]->indices[1] = 1;
	}

	return	block_new(block_type_datadesc_set, res);
}

block	do_complex_conj(block a)
{
	return	block_new(block_type_con_ser,
			block_new(block_builtin_reduce_prod),
			block_new(block_type_con_par_int,
				a,
				block_complex_conj()));
}



