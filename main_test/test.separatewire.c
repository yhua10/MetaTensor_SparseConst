
#include	<stdio.h>
#include	<string.h>

#include	"plugins.h"
#include	"target_c99.h"


void	main()
{
	target		t = target_c99_new();

	char *		dima[] = {"i", "j"};
	char *		dimb[] = {"j", "k", "l"};
	char *		dimc[] = {"k", "i"};
	char *		dimr[] = {"k", "l"};
	uint_t		length[] = {4, 4, 4};

	datadesc_set	a, b, c, r, d;
	block		bs, bs1, bs2, bp, bp1 ;


	a = datadesc_set_new(1);
	a->p[0] = datadesc_new_array(region_main, "a", endian_little, elem_type_real, 8, 2, dima, length);
	b = datadesc_set_new(1);
	b->p[0] = datadesc_new_array(region_main, "b", endian_little, elem_type_real, 8, 3, dimb, length);
	c = datadesc_set_new(1);
	c->p[0] = datadesc_new_array(region_main, "c", endian_little, elem_type_real, 8, 2, dimc, length);
	r = datadesc_set_new(1);
	r->p[0] = datadesc_new_array(region_main, "res", endian_little, elem_type_real, 8, 2, dimr, length);




	bs = block_new(block_type_con_ser,
			block_new(block_builtin_reduce_sum),
			block_new(block_type_con_par_int,
				block_new(block_type_con_ser, 
                    block_new(block_type_datadesc_set, b),
                    block_new(block_type_wire,2,3)),
				block_new(block_type_con_ser, 
                    block_new(block_type_datadesc_set, c),
                    block_new(block_type_wire,3,2)))
                );
    
    //test1,bs的separate
    printf("bs\n");
    print_block(bs,0);
    printf("bs1\n");
    bs2=seprate_wire(t,bs);
    //print_block(bs2,0);

    //test2 bp的separate
    bp= block_new(block_type_con_ser,
            block_new(block_builtin_reduce_prod),
            block_new(block_type_con_par_int,
                block_new(block_type_con_ser,//串联
                    block_new(block_type_datadesc_set,a),
                    block_new(block_type_con_ser,
                        block_new(block_type_wire,1,3),
                        block_new(block_type_wire,0,3)
                    )
                ),
                block_new(block_builtin_reduce_prod)
            )
    );

    printf("\n\nbp\n");
    //bp=seprate_wire(t,bp);
    print_data_block(bp,0);
    
    ////test3 对照bp的separate hhh toy没有什么用
    
    //
    //
    //应该怎么连datedesc里的维度？ char *		dimb[] = {"j", "k", "l"}; 并不是123.，，，
    bp1= block_new(block_type_con_ser,
            block_new(block_builtin_reduce_prod),
            block_new(block_type_con_par_int,
                block_new(block_type_con_ser,
                    block_new(block_type_datadesc_set,a),
                    block_new(block_type_con_ser,
                        block_new(block_type_wire,0,2),
                        block_new(block_type_wire,1,2)
                    )
                ),
                bs2
            )
    );

    printf("\n\n\n");
    bp1=optimize_redef_dim(t,bp);
    print_data_block(bp1,0);



	
}

