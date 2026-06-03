#include	<stdio.h>
#include	<string.h>

#include	"plugins.h"
#include	"target_c99.h"

void	main()
{
	target		t = target_c99_new();

	// i, j, k, l = 2, 1, 4, 3
	char *		dima[] = {"i", "j"};
	char *		dimb[] = {"j", "k", "l"};
	char *		dimc[] = {"k", "i"};
	char *		dimr[] = {"k", "l"};
	char *		dimTmp[] = {"tmp"};
	uint_t		length[] = {4, 4, 4};

	datadesc_set	a, b, c, r, d, tmp;
	block		bs, bp, br, wire ;

	FILE	*fp;
	char	*shead="#include <stdint.h>\n#include <stdlib.h>\nvoid calc(void *res, void *a , void *a1 , void *b, void *c){";
	char	*stail="}";

	tmp = datadesc_set_new(1);
	tmp->p[0] = datadesc_new_array(region_main, "tmp", endian_little, elem_type_real, 8, 1, dimTmp, length);
	a = datadesc_set_new(2);
	a->p[0] = datadesc_new_array(region_main, "a", endian_little, elem_type_real, 8, 2, dima, length);
	a->p[0]->dims[0]->n_entry = 2;
	a->p[1] = datadesc_new_array(region_main, "a1", endian_little, elem_type_real, 8, 2, dima, length);
	a->p[1]->dims[0]->indices[0] = a->p[1]->dims[0]->indices[2];
	a->p[1]->dims[0]->indices[1] = a->p[1]->dims[0]->indices[3];
	a->p[1]->dims[0]->n_entry = 2;
	b = datadesc_set_new(1);
	b->p[0] = datadesc_new_array(region_main, "b", endian_little, elem_type_real, 8, 3, dimb, length);
	c = datadesc_set_new(1);
	c->p[0] = datadesc_new_array(region_main, "c", endian_little, elem_type_real, 8, 2, dimc, length);
	r = datadesc_set_new(1);
	r->p[0] = datadesc_new_array(region_main, "res", endian_little, elem_type_real, 8, 2, dimr, length);

	bs = block_new(block_type_con_ser,
			block_new(block_builtin_reduce_sum),
			block_new(block_type_con_par_int,
                block_new(block_type_datadesc_set, b),
                block_new(block_type_datadesc_set, c)));

	bp = block_new(block_type_con_ser,
			block_new(block_builtin_reduce_prod),
			block_new(block_type_con_par_int,
                    block_new(block_type_datadesc_set, a),
                    bs));
	
	br = block_new(block_type_con_ser, 
				block_new(block_builtin_reduce_sum), 
				block_new(block_type_con_ser,
					block_new(block_type_con_ser, 
						block_new(block_type_con_ser, 
							bp, 
							block_new(block_type_con_ser, 
								block_new(block_type_wire, 2L, 0),
								block_new(block_builtin_generator_all))), 
						block_new(block_type_wire, 1L, 2L)), 
					block_new(block_type_con_ser, 
						block_new(block_type_wire, 2L, 0), 
						block_new(block_builtin_generator_all))));

	br = optimize_block(t, br);
	print_data_block(br, 0);

	d = block2data_dense(t, br);

    fp = fopen("dump.c", "wt");
	fwrite(shead, strlen(shead), 1, fp);
	target_c99_dump(t, fp);
	fwrite(stail, strlen(stail), 1, fp);
	fclose(fp);

}