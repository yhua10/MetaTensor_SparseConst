#include    <stdio.h>
#include    <string.h>

#include    "datadesc.h"
#include    "gc_malloc.h"


void main()
{
    /* 
    dims: (0, 1, 3, 2)
    indices：[0, 1, 2] × [(1, 2), (2, 1)] × [3, 6]
    offsets: [0, 1, 4], [0, 2], [0, 6]
    修正后off: -2
    */
    // FIXME: normalize是否需要检查dimdesc->tuple重复（datadesc中任何dim不能多次出现）
    datadesc a;
    datadesc_set setA = datadesc_set_new(1);
    a = datadesc_new(2);
    a->offset0 = -2;
    a->elem_size = 2;
    dimdesc dimA;
    dimA = dimdesc_new(4, 12);
    dimA->dims[0] = 0;
    dimA->dims[1] = 1;
    dimA->dims[2] = 3;
    dimA->dims[3] = 2;
    int_t temp_offsets[12] = {0, 6, 2, 8, 1, 7, 3, 9, 4, 10, 6, 12};
    memcpy(dimA->offsets, temp_offsets, sizeof(int_t) * 12);
    int_t temp_indices[48] = {0, 1, 2, 3, 0, 1, 2, 6, 0, 2, 1, 3, 0, 2, 1, 6,
                             1, 1, 2, 3, 1, 1, 2, 6, 1, 2, 1, 3, 1, 2, 1, 6,
                             2, 1, 2, 3, 2, 1, 2, 6, 2, 2, 1, 3, 2, 2, 1, 6};
    memcpy(dimA->indices, temp_indices, sizeof(int_t) * 12 * 4);
    a->dims[0] = dimA;
    a->dims[1] = NULL;
    a = datadesc_normalize(a);
    // a的地址变化，指针数组需要重新赋值
    setA->p[0] = a;
    datadesc_set_show(setA);
    printf("---------------\n");
    // entry不遵循笛卡尔积的生成顺序，且未按照offset排序：测试排序功能，以及offset0属性的修正
    // offset/indice为负
    // (A*B)*(C*D)型，目前不能拆分，检查是否产生副作用
    /*
    dims: (3, 1, 0, 2)
    indices：[(1, -2), (5, 3)] * [(-1, 1), (0, 0), (-1, -1)]
    offsets: [2, -1] * [0, 2, -6]
    初始entry顺序：2-2, 1-3, 1-1, 2-1, 2-3, 1-2
    修正后off0: -16
    offsets: 0   3   6   8   9   11   
    排序后indices: 
        -1   3   -1   5   
        -1   -2   -1   1   
        -1   3   1   5   
        0   3   0   5   
        -1   -2   1   1   
        0   -2   0   1   
    */
    datadesc b;
    datadesc_set setB = datadesc_set_new(1);
    b = datadesc_new(1);
    b->offset0 = -2;
    b->elem_size = 2;
    dimdesc dimB;
    dimB = dimdesc_new(4, 6);
    dimB->dims[0] = 3;
    dimB->dims[1] = 1;
    dimB->dims[2] = 0;
    dimB->dims[3] = 2;
    int_t temp_indices1[24] = {5, 3, 0 , 0, 1, -2, -1, -1, 1, -2, -1, 1, 
                        5, 3, -1, 1, 5, 3, -1, -1, 1, -2, 0, 0};
    int_t temp_offsets1[6] = {1, -4, 2, -1, -7, 4};
    memcpy(dimB->indices, temp_indices1, sizeof(int_t) * 6 * 4);
    memcpy(dimB->offsets, temp_offsets1, sizeof(int_t) * 6);
    b->dims[0] = dimB;
    b = datadesc_normalize(b);
    setB->p[0] = b;
    datadesc_set_show(setB);
    printf("---------------\n");
    // indice可以拆分，offset不可拆
    /*
    在用例1的基础上，仅修改一位offset
    排序后indices: 交换后两位:
        0   1   3   2   
        1   1   3   2   
        0   2   3   1   
        1   2   3   1   
        2   1   3   2   
        0   1   6   2   
        2   2   3   1   
        1   1   6   2   
        0   2   6   1   
        1   2   6   1   
        2   1   6   2   
        2   2   6   1
    */
    datadesc c;
    datadesc_set setC = datadesc_set_new(1);
    c = datadesc_new(1);
    c->offset0 = -2;
    c->elem_size = 2;
    dimdesc dimC;
    dimC = dimdesc_new(4, 12);
    dimC->dims[0] = 0;
    dimC->dims[1] = 1;
    dimC->dims[2] = 3;
    dimC->dims[3] = 2;
    int_t temp_offsets2[12] = {0, 6, 3, 8, 1, 7, 3, 9, 4, 10, 6, 12};
    memcpy(dimC->offsets, temp_offsets2, sizeof(int_t) * 12);
    int_t temp_indices2[48] = {0, 1, 2, 3, 0, 1, 2, 6, 0, 2, 1, 3, 0, 2, 1, 6,
                             1, 1, 2, 3, 1, 1, 2, 6, 1, 2, 1, 3, 1, 2, 1, 6,
                             2, 1, 2, 3, 2, 1, 2, 6, 2, 2, 1, 3, 2, 2, 1, 6};
    memcpy(dimC->indices, temp_indices2, sizeof(int_t) * 12 * 4);
    c->dims[0] = dimC;
    c = datadesc_normalize(c);
    setC->p[0] = c;
    datadesc_set_show(setC);
    printf("---------------\n");
    // indice_tuple去重
    /*
    在用例1的基础上修改，使indice_tuple产生一次重复~(1, 1, 3, 2)
    排序后indices：
        0   1   3   2   
        1   1   3   2   
        0   2   3   1   
        1   2   3   1   
        2   1   3   2   
        0   1   6   2   
        1   1   3   2   
        1   1   6   2   
        0   2   6   1   
        1   2   6   1   
        2   1   6   2   
        2   2   6   1
    */
    datadesc d;
    datadesc_set setD = datadesc_set_new(1);
    d = datadesc_new(1);
    d->offset0 = -2;
    d->elem_size = 2;
    dimdesc dimD;
    dimD = dimdesc_new(4, 12);
    dimD->dims[0] = 0;
    dimD->dims[1] = 1;
    dimD->dims[2] = 3;
    dimD->dims[3] = 2;
    int_t temp_offsets3[12] = {0, 6, 2, 8, 1, 7, 3, 9, 4, 10, 6, 12};
    memcpy(dimD->offsets, temp_offsets3, sizeof(int_t) * 12);
    int_t temp_indices3[48] = {0, 1, 2, 3, 0, 1, 2, 6, 0, 2, 1, 3, 0, 2, 1, 6,
                             1, 1, 2, 3, 1, 1, 2, 6, 1, 2, 1, 3, 1, 2, 1, 6,
                             2, 1, 2, 3, 2, 1, 2, 6, 1, 1, 2, 3, 2, 2, 1, 6};
    memcpy(dimD->indices, temp_indices3, sizeof(int_t) * 12 * 4);
    d->dims[0] = dimD;
    d = datadesc_normalize(d);
    setD->p[0] = d;
    datadesc_set_show(setD);
    printf("---------------\n");
    // 可拆分的强数据，datadesc中具有多个dim~在dimmap中测试
}
