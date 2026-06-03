/*  测试样例：
    datadesc_find_common: 元素 > 2：剩余dim，无关dim
    compare_and_find_common：形式化验证走两轮expand_dim即可，无需考虑笛卡尔积反复传递，只需要考虑原生稀疏
    expand_dim：datadesc1首先将多个稀疏&非稀疏做笛卡尔积，然后是datadesc2；观察验证pos不会飞掉
    dim_find_common：成功匹配，但dim_p1->entry[i]匹配dim_p2中多个entry，或dim_p1中多个entry匹配dim_p2->entry[i]
    
    其他测试：test.dimmap回归，覆盖率检测
 */

#include    <stdio.h>
#include	<string.h>

#include    <datadesc.h>
#include	"plugins.h"
#include	"target_c99.h"

void main(){
    datadesc_set data_set1 = datadesc_set_new(4);
    datadesc data1 = datadesc_new(4);
    datadesc data2 = datadesc_new(3);
    datadesc data3 = datadesc_new(2);
    datadesc data4 = datadesc_new(1);
    data_set1->p[0] = data1;
    data_set1->p[1] = data2;
    data_set1->p[2] = data3;
    data_set1->p[3] = data4;
    /*
    data1
    dims: (0, 1), (2, 4), (6, 3, 5), 7 
    offsets: [2, 4], [1, -1], [7, 9], [5, 0, 2]
    indices: [(0, 1), (1, 0)] × [(2, 4), (2, 2)] × [(6, 3, 6), (6, 3, 5)] × [6, 7, 8]
     */
    /* 
        offset0: 0
        dim information: rank 1
        dims[8] ---name index ---->
        (0, 1, 6, 3, 5, 7, 2, 4)
        offsets[1]----->
        12   
        indices[1][8]---->
        0   1   6   3   5   7   2   4   
    */
    dimdesc data1_dim1 = dimdesc_new(2, 2);
    dimdesc data1_dim2 = dimdesc_new(2, 2);
    dimdesc data1_dim3 = dimdesc_new(3, 2);
    dimdesc data1_dim4 = dimdesc_new(1, 3);
    data1->dims[0] = data1_dim1;
    data1->dims[1] = data1_dim2;
    data1->dims[2] = data1_dim3;
    data1->dims[3] = data1_dim4;
    data1_dim1->dims[0] = 0;
    data1_dim1->dims[1] = 1;
    data1_dim2->dims[0] = 2;
    data1_dim2->dims[1] = 4;
    data1_dim3->dims[0] = 6;
    data1_dim3->dims[1] = 3;
    data1_dim3->dims[2] = 5;
    data1_dim4->dims[0] = 7;
    data1_dim1->offsets[0] = 2;
    data1_dim1->offsets[1] = 4;
    data1_dim2->offsets[0] = 1;
    data1_dim2->offsets[1] = -1;
    data1_dim3->offsets[0] = 7;
    data1_dim3->offsets[1] = 9;
    data1_dim4->offsets[0] = 5;
    data1_dim4->offsets[1] = 0;
    data1_dim4->offsets[2] = 2;
    data1_dim1->indices[0] = 0;
    data1_dim1->indices[1] = 1;
    data1_dim1->indices[2] = 1;
    data1_dim1->indices[3] = 0;
    data1_dim2->indices[0] = 2;
    data1_dim2->indices[1] = 4;
    data1_dim2->indices[2] = 2;
    data1_dim2->indices[3] = 2;
    data1_dim3->indices[0] = 6;
    data1_dim3->indices[1] = 3;
    data1_dim3->indices[2] = 6;
    data1_dim3->indices[3] = 6;
    data1_dim3->indices[4] = 3;
    data1_dim3->indices[5] = 5;
    data1_dim4->indices[0] = 6;
    data1_dim4->indices[1] = 7;
    data1_dim4->indices[2] = 8;
    /*
    data2
    dims: (1, 6, 7, 0), 3, (8, 9) 
    offsets: [99, 88, 77], [66, 55, 44], [33, 22]
    indices: [(1, 6, 7, -1), (1, 6, 7, 0), (1, -6, 7, 0)] × [777, 666, 3] × [(8, 9), (9, 9)]
     */
    /*
        offset0: 0
        dim information: rank 1
        dims[5] ---name index ---->
        (1, 6, 7, 0, 3)
        offsets[1]----->
        132   
        indices[1][5]---->
        1   6   7   0   3   
        dim information: rank 2
        dims[2] ---name index ---->
        (8, 9)
        offsets[1]----->
        33   
        indices[1][2]---->
        8   9   
    */
    dimdesc data2_dim1 = dimdesc_new(4, 3);
    dimdesc data2_dim2 = dimdesc_new(1, 3);
    dimdesc data2_dim3 = dimdesc_new(2, 2);
    data2->dims[0] = data2_dim1;
    data2->dims[1] = data2_dim2;
    data2->dims[2] = data2_dim3;
    data2_dim1->dims[0] = 1;
    data2_dim1->dims[1] = 6;
    data2_dim1->dims[2] = 7;
    data2_dim1->dims[3] = 0;
    data2_dim2->dims[0] = 3;
    data2_dim3->dims[0] = 8;
    data2_dim3->dims[1] = 9;
    data2_dim1->offsets[0] = 99;
    data2_dim1->offsets[1] = 88;
    data2_dim1->offsets[2] = 77;
    data2_dim2->offsets[0] = 66;
    data2_dim2->offsets[1] = 55;
    data2_dim2->offsets[2] = 44;
    data2_dim3->offsets[0] = 33;
    data2_dim3->offsets[1] = 22;
    data2_dim1->indices[0] = 1;
    data2_dim1->indices[1] = 6;
    data2_dim1->indices[2] = 7;
    data2_dim1->indices[3] = -1;
    data2_dim1->indices[4] = 1;
    data2_dim1->indices[5] = 6;
    data2_dim1->indices[6] = 7;
    data2_dim1->indices[7] = 0;
    data2_dim1->indices[8] = 1;
    data2_dim1->indices[9] = -6;
    data2_dim1->indices[10] = 7;
    data2_dim1->indices[11] = 0;
    data2_dim2->indices[0] = 777;
    data2_dim2->indices[1] = 666;
    data2_dim2->indices[2] = 3;
    data2_dim3->indices[0] = 8;
    data2_dim3->indices[1] = 9;
    data2_dim3->indices[2] = 9;
    data2_dim3->indices[3] = 9;

    /*
    data3
    dims: (2, 9, 5), (8, 4) 
    offsets: [-1, 1, 2], [3, 5, -3]
    indices: [(2, -9, 5), (2, 9, 3), (2, 9, 5)], [(8, 4), (4, 8), (2, 4)]
     */
    /*
        offset0: 0
        dim information: rank 1
        dims[5] ---name index ---->
        (2, 9, 5, 8, 4)
        offsets[1]----->
        5   
        indices[1][5]---->
        2   9   5   8   4    
    */
    dimdesc data3_dim1 = dimdesc_new(3, 3);
    dimdesc data3_dim2 = dimdesc_new(2, 3);
    data3->dims[0] = data3_dim1;
    data3->dims[1] = data3_dim2;
    data3_dim1->dims[0] = 2;
    data3_dim1->dims[1] = 9;
    data3_dim1->dims[2] = 5;
    data3_dim2->dims[0] = 8;
    data3_dim2->dims[1] = 4;
    data3_dim1->offsets[0] = -1;
    data3_dim1->offsets[1] = 1;
    data3_dim1->offsets[2] = 2;
    data3_dim2->offsets[0] = 3;
    data3_dim2->offsets[1] = 5;
    data3_dim2->offsets[2] = -3;
    data3_dim1->indices[0] = 2;
    data3_dim1->indices[1] = -9;
    data3_dim1->indices[2] = 5;
    data3_dim1->indices[3] = 2;
    data3_dim1->indices[4] = 9;
    data3_dim1->indices[5] = 3;
    data3_dim1->indices[6] = 2;
    data3_dim1->indices[7] = 9;
    data3_dim1->indices[8] = 5;
    data3_dim2->indices[0] = 8;
    data3_dim2->indices[1] = 4;
    data3_dim2->indices[2] = 4;
    data3_dim2->indices[3] = 8;
    data3_dim2->indices[4] = 2;
    data3_dim2->indices[5] = 4;

    /*
        offset0: 0
        dim information: rank 1
        dims[2] ---name index ---->
        (10, 11)
        offsets[2]----->
        -1   1   
        indices[2][2]---->
        10   11   
        10   10   
    */
    dimdesc data4_dim1 = dimdesc_new(2, 2);
    data4->dims[0] = data4_dim1;
    data4_dim1->dims[0] = 10;
    data4_dim1->dims[1] = 11;
    data4_dim1->offsets[0] = -1;
    data4_dim1->offsets[1] = 1;
    data4_dim1->indices[0] = 10;
    data4_dim1->indices[1] = 11;
    data4_dim1->indices[2] = 10;
    data4_dim1->indices[3] = 10;

    datadesc_set res = datadesc_find_common(data_set1);
    datadesc_set_show(res);


    // 测试2
    /*
        datadesc: rank 1######################
        offset0: 0
        dim information: rank 1
        dims[3] ---name index ---->
        (0, 4, 1)
        offsets[3]----->
        0   1   2   
        indices[3][3]---->
        0   3   1   
        0   4   1   
        1   4   1   
        ######################################


        datadesc: rank 2######################
        offset0: 0
        dim information: rank 1
        dims[4] ---name index ---->
        (1, 2, 3, 4)
        offsets[3]----->
        1   2   3   
        indices[3][4]---->
        1   3   3   4   
        1   0   0   3   
        1   2   3   3   
        ######################################
    */
    printf("\n\n\n\n\n-----test2-----\n\n\n\n\n\n");
    datadesc_set data_set2 = datadesc_set_new(2);
    datadesc data5 = datadesc_new(1);
    datadesc data6 = datadesc_new(1);
    data_set2->p[0] = data5;
    data_set2->p[1] = data6;
    dimdesc data5_dim1 = dimdesc_new(3, 3);
    dimdesc data6_dim1 = dimdesc_new(4, 4);
    data5->dims[0] = data5_dim1;
    data6->dims[0] = data6_dim1;
    data5_dim1->dims[0] = 0;
    data5_dim1->dims[1] = 4;
    data5_dim1->dims[2] = 1;
    data6_dim1->dims[0] = 1;
    data6_dim1->dims[1] = 2;
    data6_dim1->dims[2] = 3;
    data6_dim1->dims[3] = 4;
    data5_dim1->offsets[0] = 0;
    data5_dim1->offsets[1] = 1;
    data5_dim1->offsets[2] = 2;
    data6_dim1->offsets[0] = 0;
    data6_dim1->offsets[1] = 1;
    data6_dim1->offsets[2] = 2;
    data6_dim1->offsets[3] = 3;
    data5_dim1->indices[0] = 0;
    data5_dim1->indices[1] = 3;
    data5_dim1->indices[2] = 1;
    data5_dim1->indices[3] = 0;
    data5_dim1->indices[4] = 4;
    data5_dim1->indices[5] = 1;
    data5_dim1->indices[6] = 1;
    data5_dim1->indices[7] = 4;
    data5_dim1->indices[8] = 1;
    data6_dim1->indices[0] = 2;
    data6_dim1->indices[1] = 2;
    data6_dim1->indices[2] = 3;
    data6_dim1->indices[3] = 4;
    data6_dim1->indices[4] = 1;
    data6_dim1->indices[5] = 3;
    data6_dim1->indices[6] = 3;
    data6_dim1->indices[7] = 4;
    data6_dim1->indices[8] = 1;
    data6_dim1->indices[9] = 0;
    data6_dim1->indices[10] = 0;
    data6_dim1->indices[11] = 3;
    data6_dim1->indices[12] = 1;
    data6_dim1->indices[13] = 2;
    data6_dim1->indices[14] = 3;
    data6_dim1->indices[15] = 3;

    datadesc_set res2 = datadesc_find_common(data_set2);
    datadesc_set_show(res2);
}