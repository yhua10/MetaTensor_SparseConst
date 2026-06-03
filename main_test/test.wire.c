
#include	<stdio.h>
#include	<string.h>

#include	"plugins.h"
#include	"target_c99.h"

datadesc datadesc_merge_dimb2a(datadesc data, int dima , int dimb);
void datadesc_show(datadesc p);
block seprate_wire(target t, block ob);
void main() {
    target		t = target_c99_new();

    // m, i, j, k = 0, 1, 2, 3
    char *      dima[] = {"i", "m"};
    char *      dimb[] = {"k", "j"};
    char *      dimc[] = {"m"};
    char *      dimd[] = {"j"};
    char *      dime[] = {"i"};
    char *      dimf[] = {"j", "k"};
    uint_t      length[] = {4, 4};

    datadesc_set    a, b, c, d, e, f;
    block           bs, bs1, bs2, bs3, bs4;

    a = datadesc_set_new(1);
    a->p[0] = datadesc_new_array(region_main, "a", endian_little,  elem_type_real, 8, 2, dima, length);
    b = datadesc_set_new(1);
    b->p[0] = datadesc_new_array(region_main, "b", endian_little,  elem_type_real, 8, 2, dimb, length);
    c = datadesc_set_new(1);
    c->p[0] = datadesc_new_array(region_main, "c", endian_little,  elem_type_real, 8, 1, dimc, length);
    d = datadesc_set_new(1);
    d->p[0] = datadesc_new_array(region_main, "d", endian_little,  elem_type_real, 8, 1, dimd, length);
    e = datadesc_set_new(1);
    e->p[0] = datadesc_new_array(region_main, "e", endian_little,  elem_type_real, 8, 1, dime, length);
    f = datadesc_set_new(1);
    f->p[0] = datadesc_new_array(region_main, "f", endian_little,  elem_type_real, 8, 2, dimf, length);
    
    bs1 = block_new(block_type_wire, 2, -1);

    bs2 = block_new(block_type_wire, 2, 1),

    printf("#############bs1 begin#################\n");
    print_block(bs1, 0);
    printf("w[0] = %ld, w[1] = %ld\n\n",bs1->p.w[0],bs1->p.w[1]);
    //这里很奇怪，如果w[1]是使用%d打印就是-1（会有个warning，但也能正常打印》
    printf("如果使用int型打印： w[1] = %d\n\n",bs1->p.w[1]);
    printf("#############separate#################\n");
    //if (ob->p.w[0] >= 0 && ob->p.w[1] >= 0) 才会separate，实际上不该separate
    //但if判断为true，也就是p.w[1]确实是一个大的正整数
    bs1 =  seprate_wire(t, bs1);

    print_block(bs1, 0);
    // print_data_block(bs2, 0);
    printf("############bs1 end#################\n\n\n");

    printf("#############bs2 begin#################\n");
    print_block(bs2, 0);
    bs2 =  seprate_wire(t, bs2);

    printf("#############separate#################\n");
    //这个一切正常
    print_block(bs2, 0);
    printf("############end#################\n");

}