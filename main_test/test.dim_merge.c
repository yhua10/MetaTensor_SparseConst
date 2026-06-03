
#include	<stdio.h>
#include	<string.h>

#include	"plugins.h"
#include	"target_c99.h"

datadesc datadesc_merge_dimb2a(datadesc data, int dima , int dimb);
void datadesc_show(datadesc p);

void	main()
{
	target		t = target_c99_new();
	 // m, i, j, k = 0, 1, 2, 3
	char *		dima[] = {"i", "j"};
	char *		dimb[] = {"m" ,"i","j", "k", "l"};
	char *		dimc[] = {"k", "i"};
	char *		dimr[] = {"k", "l"};
	uint_t		length[] = {5, 8, 3, 2, 4};

	datadesc_set	a, b, c, r, d;
	block		bs, bs1, bs2, bp, wire ;
	dimdesc dim_p;
	FILE	*fp;
	char	*shead="#include <stdint.h>\n#include <stdlib.h>\nvoid calc(void *res, void *a , void *b, void *c){";
	char	*stail="}";

	a = datadesc_set_new(1);
	a->p[0] = datadesc_new_array(region_main, "a", endian_little, elem_type_real, 8, 3, dimb, length);
	b = datadesc_set_new(1);
	b->p[0] = datadesc_new_array(region_main, "b", endian_little, elem_type_real, 8, 3, dimb, length);
	c = datadesc_set_new(1);
	c->p[0] = datadesc_new_array(region_main, "c", endian_little, elem_type_real, 8, 2, dimc, length);
	r = datadesc_set_new(1);
	r->p[0] = datadesc_new_array(region_main, "res", endian_little, elem_type_real, 8, 2, dimr, length);

	bs = block_new(block_type_con_ser,
			block_new(block_type_datadesc_set,a),
			block_new(block_type_wire,3,2)
	);

	//printf("bs\n");
	//print_data_block(bs,0);
	
	int wire_left = bs->p.b[1]->p.w[0];
	//printf("%d \n\n\n\n",wire_left);
	int wire_right = bs->p.b[1]->p.w[1];
	//printf("%d \n\n\n\n",wire_right);
	
	// c :k i  i -> k
	printf("original datadesc :\n\n");
	datadesc_set_show(a);

	//datadesc_merge_dimb2a(a->p[0],a->p[0]->dims[0],a->p[0]->dims[1]);
	//a->n = 2;
	datadesc temp = datadesc_merge_dimb2a(a->p[0],1,2);
	//a->p[1] = temp;
	printf("datadesc after mergeb2a:\n\n");
		datadesc_show(a->p[0]);


	printf("%p,%p\n", a->p[0], temp);
	printf("\n\n\n\n\nfinish\n\n");

	
	
}

			 
