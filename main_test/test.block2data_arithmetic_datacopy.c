
#include	<stdio.h>
#include	<string.h>

#include	"plugins.h"
#include	"target_c99.h"


// 计算： result_{k,l} = \Sum_{i,j} A_{i,j} * ( B_{j,k,l) + C_{k, i} )

void	main()
{
	target		t = target_c99_new();

	char *		dima[] = {"i", "j"};
	char *		dimb[] = {"j", "k", "l"};
	char *		dimc[] = {"k", "i"};
	char *		dimr[] = {"k", "l"};
	uint_t		length[] = {4, 4, 4};

	datadesc_set	a, b, c, r, d;
	block		bs, bs1, bp, wire ;

	FILE	*fp;
	char	*shead="#include <stdint.h>\n#include <stdlib.h>\nvoid calc(void *res, void *a , void *b, void *c){";
	char	*stail="}";

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
				block_new(block_type_datadesc_set, b),
				block_new(block_type_datadesc_set, c)));
	bs1 = block_new(block_type_datadesc_set, block2data_arithmetic(t, bs));

	bp = block_new(block_type_con_ser,
			block_new(block_builtin_reduce_prod),
			block_new(block_type_con_par_int,
				block_new(block_type_datadesc_set, a),
				bs1));
	d = block2data_arithmetic(t, bp);

	wire = seprate_wire(t,block_new(block_type_wire,1,2));
	datacopy_121_reference(t, r, d);

	printf("bs\n");
	print_block(bs,0);
	printf("\n\n\nbs1\n");
	print_block(bs1,0);

	print_block(bp,0);
	printf("bp\n");
	print_block(d,0);
	printf("d \n");

	print_block(wire,0);

	fp = fopen("dump.c", "wt");
	fwrite(shead, strlen(shead), 1, fp);
	target_c99_dump(t, fp);
	fwrite(stail, strlen(stail), 1, fp);
	fclose(fp);
}

