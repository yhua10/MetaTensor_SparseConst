
#include	<stdio.h>
#include	<string.h>

#include	"plugins.h"
#include	"target_c99.h"

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

// [+] - ( ( [*] - ( a & b ) ) - ( [n-] - [G] ) ) => [*] - ( a & ( [+] - ( b - ( [n-] - [G] ) ) ) )
// exchange:  
    bs = block_new(block_type_con_ser, 
            block_new(block_builtin_reduce_sum), 
            block_new(block_type_con_ser,
                block_new(block_type_con_ser,
                    block_new(block_builtin_reduce_prod), 
                    block_new(block_type_con_par_int, 
                        block_new(block_type_datadesc_set,a),
                        block_new(block_type_datadesc_set,b)
                    )
                ),
                block_new(block_type_con_ser,
                    block_new(block_type_wire, 2, -1),
                    block_new(block_builtin_generator_all)
                )
            )
    );
                    

   // bs2 = optimize_redef_dim(t, bs);
    print_block(bs, 0);
    // print_data_block(bs2, 0);
    //printf("aaa\n\n\n");
    bs1 = optimize_common_factor(t,bs);
    print_block(bs1, 0);
    //printf("%d",tem) ;
    //bs3 = optimize_redef_dim(t, bs1);

    //print_block(bs3, 0);
    // print_data_block(bs3, 0);
    
}

