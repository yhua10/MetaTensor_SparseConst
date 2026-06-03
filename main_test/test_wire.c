
#include	<stdio.h>
#include	<string.h>

#include	"plugins.h"
#include	"target_c99.h"


void	main()
{
	target		t = target_c99_new();
	 // m, i, j, k = 0, 1, 2, 3
	char *		dima[] = {"i", "j"};
	char *		dimb[] = {"j", "k", "l"};
	char *		dimc[] = {"k", "i"};
	char *		dimr[] = {"k", "l"};
	uint_t		length[] = {4, 4, 4};

	datadesc_set	a, b, c, r, d;
	block		bs, bs1, bs2, bp, wire ;

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
			block_new(block_type_datadesc_set,b),
			block_new(block_type_wire,3,2)
	);

	printf("bs\n");
	print_data_block(bs,0);
	
	int wire_left = bs->p.b[1]->p.w[0];
	printf("%d \n\n\n\n",wire_left);
	int wire_right = bs->p.b[1]->p.w[1];
	printf("%d \n\n\n\n",wire_right);
	
	// c :k i  i -> k
	
	/*
	printf("%d \n\n\n\n", bs->p.b[0]->p.d->p[0]->n_dim);
	for (int_t i = 0; i < bs->p.b[0]->p.d->n; i++) {
				for (int_t j = 0; j < bs->p.b[0]->p.d->p[i]->n_dim; j++) {
					for (int_t k = 0; k < bs->p.b[0]->p.d->p[i]->dims[j]->n_tuple; k++) {
						printf("n_tuple : %d \n\n\n\n", bs->p.b[0]->p.d->p[i]->dims[j]->n_tuple);
						int_t dim_index = bs->p.b[0]->p.d->p[i]->dims[j]->dims[k];
						printf("dim_index : %d \n\n\n\n", dim_index);
						if (dim_index > 0) {
							//for (int_t m = 0; m < wire_cnt; m++) {
								if (wire_left == dim_index) {
									
									bs->p.b[0]->p.d->p[i]->dims[j]->dims[k] = wire_right;
									//   
									
									// 第二次rename的时候需要把dim k -> dim i dim j -> dim i , 需要把
									// 需要把indeices求交集，offset求和，并删除这个维度
									// if(wire_left == dim_index){
									// 		if(wire_right == bs->p.b[0]->p.d->p[i]->dims[j]->dims[k]) 如果match到
									//			bs->p.b[0]->p.d->p[i]->dims[j]->n_tuple++
									//			indeices求交集? 
									//			offset 求和 ? 
									//}
									//printf("%ld", dim_p->indices[k * dim_p->n_tuple + l]);
									//
									//printf(")\t - \t%ld\n", dim_p->offsets[k]);
									// 1.bs->p.b[0]->p.d->p[i]->dims[j]->n_tuple++
									// 2.bs->p.b[0]->p.d->p[i]->dims[j]->n_tuple
									break;
								}
							//}
						}
					}
				}
			}
	*/


    print_data_block(bs,0);


	
}

//									//printf("%ld", dim_p->indices[k * dim_p->n_tuple + l]);
									// indices[k] 里存的是 下标数字， 若稀疏则按n_tuple顺序储存，如 indices[0] indices[1]是一组; 2, 3一组  …… 
									//printf(")\t - \t%ld\n", dim_p->offsets[k]);
void indices_intersection(dimdesc a , dimdesc b,int_t* rst){
	int_t tmp_indices[9999];
	for (int i = 0; i < 9999; i++){
		tmp_indices[i] = -1000;
	} 
	int cnt = 0, rst_cnt = 0;
	printf("\t\t\tindices - offsets : \n");
			// 1. 开足够大的空间
			for (k = 0; k < a->n_entry; k++)
			{
				printf("\t\t\t\t(");
				for (l = 0; l < a->n_tuple; l++)
				{
					if (l>0)
						printf(", ");
					printf("%ld", a->indices[k * a->n_tuple + l]); 
					tmp_indices[k * a->n_tuple + l] =  a->indices[k * a->n_tuple + l] //rst记录所有a的indices;
					cnt ++;
				}
				printf(")\t - \t%ld\n", a->offsets[k]);
			}

			for (k = 0; k < b->n_entry; k++)
			{
				printf("\t\t\t\t(");
				for (l = 0; l < b->n_tuple; l++)
				{
					if (l>0)
						printf(", ");
					printf("%ld", b->indices[k * b->n_tuple + l]); 
					for(int m = 0 ; m < cnt; m++){
						if(b->indices[k * b->n_tuple + l] == tmp_indices[m]){
							rst[rst_cnt] = tmp[m]; //判断tmp中是否有indices，若有则记录在rst中
							rst_cnt++;
						} 
					}
				}
				printf(")\t - \t%ld\n", b->offsets[k])
			}
			//根据rst 的 indices计算offset和 if(rst[i] == b->indices[k * b->n_tuple + l]) off_rst[i] += b->offsets[k]; 需要对a,b两个dimdesc都遍历查找一遍
			//暂时没考虑稀疏情况再整合；；；；
			//应最后直接改为返回一个dimdesc。


}

			 