
#include	<stdio.h>
#include	<string.h>

#include	"plugins.h"
#include	"target_c99.h"

void main() {
    target		t = target_c99_new();

    // m, i, j, k = 1, 2, 3, 4
    char *      dima[] = {"i", "m"};
    char *      dimb[] = {"k", "j"};
    char *      dimc[] = {"m"};
    char *      dimd[] = {"j"};
    char *      dime[] = {"i"};
    char *      dimf[] = {"j", "k"};
    char *      dimtmp[]= {"tmp"};
    uint_t      length[] = {4, 4};

    datadesc_set    a, b, c, d, e, f,tmp;
    block           bs, bs1, bs2, bs3, bs4;

    tmp = datadesc_set_new(1);
    tmp->p[0] = datadesc_new_array(region_main, "tmp", endian_little,  elem_type_real, 8, 1, dimtmp, length);
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
    
    bs = block_new(block_type_con_ser, 
            block_new(block_builtin_unary_minus), 
            block_new(block_type_con_ser,
                block_new(block_builtin_reduce_sum), 
                block_new(block_type_con_ser, 
                    block_new(block_builtin_unary_cos),
                    block_new(block_type_con_ser, 
                        block_new(block_builtin_unary_sin), 
                        block_new(block_type_con_ser, 
                            block_new(block_type_con_par_int, 
                                block_new(block_type_datadesc_set, a), 
                                block_new(block_type_datadesc_set, b)),
                            block_new(block_type_con_ser, 
                                block_new(block_type_wire, 3L, 0L),
                                block_new(block_builtin_generator_all)))))));

    bs1 = block_new(block_type_con_ser,
            block_new(block_builtin_reduce_sum),
            block_new(block_type_con_ser, 
                block_new(block_type_con_ser, 
                    block_new(block_type_con_par_int,
                        block_new(block_type_con_ser, 
                            block_new(block_type_con_par_int, 
                                block_new(block_type_datadesc_set, c), 
                                block_new(block_type_con_ser, 
                                    block_new(block_type_datadesc_set, d), 
                                    block_new(block_type_wire, 3L, 2L))), 
                            block_new(block_type_wire, 2L, 4L)),
                        block_new(block_type_con_ser, 
                            block_new(block_type_con_par_int, 
                                block_new(block_type_datadesc_set, e), 
                                block_new(block_type_datadesc_set, f)), 
                            block_new(block_type_wire, 2L, 3L))),  
                    block_new(block_type_wire, 1L, 3L)), 
                block_new(block_type_con_ser, 
                    block_new(block_type_wire, 3L, 0L), 
                    block_new(block_builtin_generator_all))));

    // bs2 = optimize_block(t, bs);

    // print_block(bs2, 0);
    // print_data_block(bs2, 0);
    
    bs3 = optimize_block(t, bs1);

    print_data_block(bs3, 0);
    // print_data_block(bs3, 0);
    
}