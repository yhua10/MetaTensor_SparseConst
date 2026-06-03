#include    <stdio.h>
#include    <string.h>

#include    "datadesc.h"
#include    "gc_malloc.h"

datadesc_set_set	datadesc_set_merge_pre(datadesc_set p);
uint_t	datadesc_subset_merge_search(datadesc_set p, void *** mutat);
datadesc_set	datadesc_subset_merge_1step(datadesc_set p, merge_info_piece merge_info);

int_t dimmap_f_odd(int_t *indices_out, int_t *indices_in) {
    if (indices_in[0] % 2 == 0) {
        return NULL;
    }
    indices_out[0] = indices_in[0];
    return 1;
}
int_t dimmap_f_even(int_t *indices_out, int_t *indices_in) {
    if (indices_in[0] % 2 == 1) {
        return NULL;
    }
    indices_out[0] = indices_in[0];
    return 1;
}


void test_datadesc_spilt() {
    char *refname1 = (char *)malloc(sizeof(char));
    char *refname2 = (char *)malloc(sizeof(char));
    datadesc_set dataset = datadesc_set_new(11);
    for (uint_t i = 0; i < 11; i++) {
        datadesc data = datadesc_new(5);
        dataset->p[i] = data;
        data->elem_size = 2;
        data->elem_type = 0;
        data->endian = 0;
        data->region = 3;
        data->ref.p_refname = refname1;
    }
    dataset->p[0]->endian = 3;
    dataset->p[3]->elem_size = 4;
    dataset->p[5]->region = 1;
    dataset->p[6]->elem_type = 2;
    dataset->p[8]->ref.p_direct = refname2;

    // subset1: p1, p4, p9, p10. 包含"维度名": 0-4 
    // subset2: p2, p7. 包含"维度名": 0-2, 4
    dimdesc p2_dim1 = dimdesc_new(4, 10);
    p2_dim1->dims[0] = 1;
    p2_dim1->dims[1] = 4;
    p2_dim1->dims[2] = 0;
    p2_dim1->dims[3] = 2;
    dataset->p[2]->dims[0] = p2_dim1;
    dataset->p[2]->dims[1] = NULL;

    dimdesc p7_dim1 = dimdesc_new(1, 2);
    p7_dim1->dims[0] = 2;
    dataset->p[7]->dims[0] = p7_dim1;
    dimdesc p7_dim2 = dimdesc_new(1, 5);
    p7_dim2->dims[0] = 1;
    dataset->p[7]->dims[1] = p7_dim2;
    dimdesc p7_dim3 = dimdesc_new(1, 4);
    p7_dim3->dims[0] = 0;
    dataset->p[7]->dims[2] = p7_dim3;
    dimdesc p7_dim4 = dimdesc_new(1, 10);
    p7_dim4->dims[0] = 4;
    dataset->p[7]->dims[3] = p7_dim4;
    dataset->p[7]->dims[4] = NULL;

    dimdesc p1_dim1 = dimdesc_new(2, 4);
    p1_dim1->dims[0] = 3;
    p1_dim1->dims[1] = 0;
    dataset->p[1]->dims[0] = p1_dim1;
    dimdesc p1_dim2 = dimdesc_new(2, 1);
    p1_dim2->dims[0] = 1;
    p1_dim2->dims[1] = 4;
    dataset->p[1]->dims[1] = p1_dim2;
    dimdesc p1_dim3 = dimdesc_new(1, 1);
    p1_dim3->dims[0] = 2;
    dataset->p[1]->dims[2] = p1_dim3;
    dataset->p[1]->dims[3] = NULL;

    dimdesc p4_dim1 = dimdesc_new(1, 4);
    p4_dim1->dims[0] = 1;
    dataset->p[4]->dims[0] = p4_dim1;
    dimdesc p4_dim2 = dimdesc_new(4, 1);
    p4_dim2->dims[0] = 0;
    p4_dim2->dims[1] = 4;
    p4_dim2->dims[2] = 3;
    p4_dim2->dims[3] = 2;
    dataset->p[4]->dims[1] = p4_dim2;
    dataset->p[4]->dims[2] = NULL;

    dimdesc p9_dim1 = dimdesc_new(3, 4);
    p9_dim1->dims[0] = 2;
    p9_dim1->dims[1] = 4;
    p9_dim1->dims[2] = 1;
    dataset->p[9]->dims[0] = p9_dim1;
    dimdesc p9_dim2 = dimdesc_new(1, 1);
    p9_dim2->dims[0] = 3;
    dataset->p[9]->dims[1] = p9_dim2;
    dimdesc p9_dim3 = dimdesc_new(1, 1);
    p9_dim3->dims[0] = 0;
    dataset->p[9]->dims[2] = p9_dim3;
    dataset->p[9]->dims[3] = NULL;

    dimdesc p10_dim1 = dimdesc_new(1, 4);
    p10_dim1->dims[0] = 0;
    dataset->p[10]->dims[0] = p10_dim1;
    dimdesc p10_dim2 = dimdesc_new(2, 3);
    p10_dim2->dims[0] = 2;
    p10_dim2->dims[1] = 1;
    dataset->p[10]->dims[1] = p10_dim2;
    dimdesc p10_dim3 = dimdesc_new(1, 2);
    p10_dim3->dims[0] = 3;
    dataset->p[10]->dims[2] = p10_dim3;
    dimdesc p10_dim4 = dimdesc_new(1, 2);
    p10_dim4->dims[0] = 4;
    dataset->p[10]->dims[3] = p10_dim4;
    dataset->p[10]->dims[4] = NULL;

    datadesc_set_set datasets = datadesc_set_merge_pre(dataset);

    /* 
        期望split结果：
            cnt：7
            subset->n：1，4，2，1，1，1，1
    */
    printf("cnt: %ld\n", datasets->n);
    for (uint_t i = 0; i < datasets->n; i++) {
        printf("subset%ld->n: %ld\n", i, datasets->p[i]->n);
    }
    for (uint_t i = 0; i < datasets->n; i++) {
        datadesc_set_show(datasets->p[i]);
        printf("----------------------------------------------------------------\n\n\n\n\n\n\n\n");
    }
}

void print_merge_info(merge_info_piece merge_info) {
    if(merge_info->merge_info_type == merge_type_1dim) {
        printf("merge_type_1dim: ");
        for (uint_t i = 0; i < merge_info->dims_cnt; i++) {
            printf("%ld ", merge_info->dims[i]);
        }
    } else if (merge_info->merge_info_type == merge_type_1step) {
        printf("merge_type_1step: ");
        printf("%ld, %ld ~ ", merge_info->data_index1, merge_info->data_index2);
        for (uint_t i = 0; i < merge_info->dims_cnt; i++) {
            printf("%ld ", merge_info->dims[i]);
        }
    }
    printf("\n");
}

void test_datadesc_subset_merge_search() {
    datadesc_set set = datadesc_set_new(6);
    // dims构造：dims[]，entry[]已经排序
    /* 
        dims: (0, 1, 2), 3, (4, 5)
        offsets: 0, 0, [0, 1, 2], [0, 8], [0, 4], [0, 16]
        indices: 0, 0, [0, 1, 2], [0, 1], [0, 1], [0, 1] 
    */
    datadesc data1 = datadesc_new(5);
    set->p[0] = data1;
    data1->offset0 = 0;
    data1->elem_size = 8;
    data1->dims[3] = NULL;
    data1->dims[4] = NULL;
    dimdesc data1_dim1 = dimdesc_new(3, 3);
    dimdesc data1_dim2 = dimdesc_new(1, 2);
    dimdesc data1_dim3 = dimdesc_new(2, 4);
    data1->dims[0] = data1_dim1;
    data1->dims[1] = data1_dim2;
    data1->dims[2] = data1_dim3;
    data1_dim1->reduce_type = reduce_type_sum;
    data1_dim2->reduce_type = reduce_type_sum;
    data1_dim3->reduce_type = reduce_type_sum;
    data1_dim1->dims[0] = 0;
    data1_dim1->dims[1] = 1;
    data1_dim1->dims[2] = 2;
    data1_dim2->dims[0] = 3;
    data1_dim3->dims[0] = 4;
    data1_dim3->dims[1] = 5;
    int_t offs1[3] = {0, 1, 2};
    memcpy(data1_dim1->offsets, offs1, sizeof(int_t) * 3);
    int_t offs2[2] = {0, 8};
    memcpy(data1_dim2->offsets, offs2, sizeof(int_t) * 2);
    int_t offs3[4] = {0, 4, 16, 20};
    memcpy(data1_dim3->offsets, offs3, sizeof(int_t) * 4);
    int_t indices1[9] = {0, 0, 0, 0, 0, 1, 0, 0, 2};
    memcpy(data1_dim1->indices, indices1, sizeof(int_t) * 9);
    int_t indices2[2] = {0, 1};
    memcpy(data1_dim2->indices, indices2, sizeof(int_t) * 2);
    int_t indices3[8] = {0, 0, 1, 0, 0, 1, 1, 1};
    memcpy(data1_dim3->indices, indices3, sizeof(int_t) * 8);
    
    // n_dim不同
    datadesc data2 = datadesc_new(3);
    set->p[1] = data2;
    data2->offset0 = 0;
    data2->elem_size = 8;
    dimdesc data2_dim1 = dimdesc_new(4, 6);
    dimdesc data2_dim2 = dimdesc_new(2, 4);
    data2->dims[0] = data2_dim1;
    data2->dims[1] = data2_dim2;
    data2->dims[2] = NULL;
    uint_t dims1[4] = {0, 1, 2, 3};
    memcpy(data2_dim1->dims, dims1, sizeof(uint_t) * 4);
    data2_dim2->dims[0] = 4;
    data2_dim2->dims[1] = 5;

    // n_dim相同但是维度错开
    datadesc data3 = datadesc_new(3);
    set->p[2] = data3;
    data3->offset0 = 0;
    data3->elem_size = 8;
    dimdesc data3_dim1 = dimdesc_new(2, 1);
    dimdesc data3_dim2 = dimdesc_new(2, 6);
    data3->dims[0] = data3_dim1;
    data3->dims[1] = data3_dim2;
    data3->dims[2] = data2_dim2;
    data3_dim1->dims[0] = 0;
    data3_dim1->dims[1] = 1;
    data3_dim2->dims[0] = 2;
    data3_dim2->dims[1] = 3;

    // reduce_type不同
    datadesc data4 = datadesc_new(3);
    set->p[3] = data4;
    data4->offset0 = 0;
    data4->elem_size = 8;
    data4->dims[0] = data1->dims[0];
    data4->dims[1] = data1->dims[1];
    dimdesc data4_dim3 = dimdesc_new(2, 4);
    data4->dims[2] = data4_dim3;
    memcpy(data4_dim3->dims, data1_dim3->dims, sizeof(uint_t) * 2);
    memcpy(data4_dim3->offsets, data1_dim3->offsets, sizeof(int_t) * 4);
    memcpy(data4_dim3->indices, data1_dim3->indices, sizeof(int_t) * 8);
    data4_dim3->reduce_type = 3;

    // data5_dim1是data1_dim1的子集，可以合并data5_dim1(0, 1, !2) & data1_dim1
    datadesc data5 = datadesc_new(4);
    set->p[4] = data5;
    data5->offset0 = 0;
    data5->elem_size = 8;
    data5->dims[3] = NULL;
    data5->dims[2] = data1->dims[1];
    data5->dims[0] = data1->dims[2];
    dimdesc data5_dim1 = dimdesc_new(3, 2);
    data5->dims[1] = data5_dim1;
    memcpy(data5_dim1->dims, data1_dim1->dims, sizeof(uint_t) * 3);
    memcpy(data5_dim1->offsets, &(data1_dim1->offsets[1]), sizeof(int_t) * 2);
    memcpy(data5_dim1->indices, &(data1_dim1->indices[3]), sizeof(int_t) * 6);
    data5_dim1->reduce_type = reduce_type_sum;
    
    /* 
    // 0：制造offsets错误
    data5_dim1->offsets[1] = 4;
    */

    /* 
    // 1：制造offset0错误
    data5->offset0 = 8; 
    */

    
    // 2：合并data5_dim1(0, 1, !2) & data1_dim1；合并data5_dim2(3) & data1_dim2
    // data1和data5的offset0并不相等，验证[有多个可合并dim的]互补数据块合并时的offset处理
    // 与1组合，验证合并时偏移量修正
    // 与0组合，由于dim2完全不同，0的错误降级成矛盾
    data5->offset0 = 32;
    dimdesc data5_dim2 = dimdesc_new(1, 3);
    data5->dims[2] = data5_dim2;
    data5_dim2->dims[0] = 3;
    int_t offs4[3] = {0, 1, 2};
    memcpy(data5_dim2->offsets, offs4, sizeof(int_t) * 3);
    int_t indiecs4[3] = {2, 3, 4};
    memcpy(data5_dim2->indices, indiecs4, sizeof(int_t) * 3);
    data5_dim2->reduce_type = reduce_type_sum;

    // data6和data1实质相同
    // 描述中offset0/offsets设置不同，且entry/dim乱序——通过normalize都能够统一
    datadesc data6 = datadesc_new(3);
    set->p[5] = data6;
    data6->offset0 = 32;
    data6->elem_size = 8;
    data6->dims[0] = data1->dims[0];
    dimdesc data6_dim2 = dimdesc_new(1, 2);
    dimdesc data6_dim3 = dimdesc_new(2, 4);
    data6->dims[2] = data6_dim2;
    data6->dims[1] = data6_dim3;
    data6_dim2->reduce_type = reduce_type_sum;
    data6_dim3->reduce_type = reduce_type_sum;
    memcpy(data6_dim2->dims, data1_dim2->dims, sizeof(uint_t) * 1);
    memcpy(data6_dim3->dims, data1_dim3->dims, sizeof(uint_t) * 2);
    memcpy(data6_dim2->offsets, data1_dim2->offsets, sizeof(int_t) * 2);
    memcpy(data6_dim3->offsets, data1_dim3->offsets, sizeof(int_t) * 4);
    memcpy(data6_dim2->indices, data1_dim2->indices, sizeof(int_t) * 2);
    memcpy(data6_dim3->indices, data1_dim3->indices, sizeof(int_t) * 8);
    for (uint_t i = 0; i < 2; i++) {
        data6_dim2->offsets[i] = data6_dim2->offsets[i] - 3;
    }
    for (uint_t i = 0; i < 4; i++) {
        data6_dim3->offsets[i]--;
    }
    uint_t tempu = data6_dim3->dims[0];
    data6_dim3->dims[0] = data6_dim3->dims[1];
    data6_dim3->dims[1] = tempu;
    int_t tempi;
    for (uint_t i = 0; i < 8; i = i + 2) {
        tempi = data6_dim3->indices[i];
        data6_dim3->indices[i] = data6_dim3->indices[i + 1];
        data6_dim3->indices[i + 1] = tempi;
    }
    tempi = data6_dim3->offsets[0];
    data6_dim3->offsets[0] = data6_dim3->offsets[2];
    data6_dim3->offsets[2] = tempi;
    tempi = data6_dim3->indices[0];
    data6_dim3->indices[0] = data6_dim3->indices[4];
    data6_dim3->indices[4] = tempi;
    tempi = data6_dim3->indices[1];
    data6_dim3->indices[1] = data6_dim3->indices[5];
    data6_dim3->indices[5] = tempi;

    merge_info_piece **mutat = (merge_info_piece **)malloc(sizeof(merge_info_piece *));
    // 事实上，merge_pre是merge_search的前提，只关心merge_pre后merge_search的表现；不经过merge_pre时，merge_search是未定义的
    datadesc_set_show(set);
    for (uint_t i = 0; i < 6; i++) {
        set->p[i] = datadesc_normalize(set->p[i]);
    }
    datadesc_set_show(set);
    uint_t merge_cnt = datadesc_subset_merge_search(set, mutat);
    for (uint_t i = 0; i < merge_cnt; i++) {
        print_merge_info((*mutat)[i]);
    }
    set = datadesc_subset_merge_1step(set, (*mutat)[0]);
    datadesc_set_show(set);
}

void test_datadesc_subset_merge_1step() {
    /*
        data1，data3先合并，由稀疏变为稠密，然后可以与data2合并
    */

    datadesc_set set = datadesc_set_new(3);

    datadesc data1 = datadesc_new(2);
    set->p[0] = data1;
    data1->offset0 = -8;
    data1->elem_size = 8;
    dimdesc data1_dim1 = dimdesc_new(2, 4);
    data1->dims[0] = data1_dim1;
    data1_dim1->reduce_type = reduce_type_sum;
    data1_dim1->dims[0] = 1;
    data1_dim1->dims[1] = 2;
    int_t indices[8] = {1, 0, 0, 0, 2, 2, 0, 1};
    memcpy(data1_dim1->indices, indices, sizeof(int_t) * 8);
    int_t offsets[4] = {2, 1, 5, 2};
    memcpy(data1_dim1->offsets, offsets, sizeof(int_t) * 4);
    dimdesc data1_dim2 = dimdesc_new(1, 1);
    data1->dims[1] = data1_dim2;
    data1_dim2->reduce_type = reduce_type_sum;
    data1_dim2->dims[0] = 0;
    data1_dim2->offsets[0] = 1;
    data1_dim2->indices[0] = 1;

    datadesc data2 = datadesc_new(2);
    set->p[2] = data2;
    data2->offset0 = 8;
    data2->elem_size = 8;
    /* // 绝不允许有data间共享dim的情况，调用normalize会产生不一致
    data2->dims[0] = data1_dim2; */
    dimdesc data2_dim2 = dimdesc_new(1, 1);
    data2->dims[0] = data2_dim2;
    data2_dim2->reduce_type = reduce_type_sum;
    data2_dim2->dims[0] = 0;
    data2_dim2->offsets[0] = 1;
    data2_dim2->indices[0] = 1;
    dimdesc data2_dim1 = dimdesc_new(2, 6);
    data2->dims[1] = data2_dim1;
    data2_dim1->reduce_type = reduce_type_sum;
    data2_dim1->dims[0] = 1;
    data2_dim1->dims[1] = 2;
    int_t indices1[12] = {1, 1, 2, 1, 2, 0, 0, 2, 0, 0, 1, 2};
    memcpy(data2_dim1->indices, indices1, sizeof(int_t) * 12);
    int_t offsets1[6] = {1, 2, 1, 1, -1, 2};
    memcpy(data2_dim1->offsets, offsets1, sizeof(int_t) * 6);

    datadesc data3 = datadesc_new(3);
    set->p[1] = data3;
    data3->offset0 = 0;
    data3->elem_size = 8;
    dimdesc data3_dim1 = dimdesc_new(1, 1);
    data3->dims[0] = data3_dim1;
    data3_dim1->reduce_type = reduce_type_sum;
    data3_dim1->dims[0] = 0;
    data3_dim1->indices[0] = 0;
    data3_dim1->offsets[0] = 0;
    dimdesc data3_dim2 = dimdesc_new(1, 3);
    data3->dims[1] = data3_dim2;
    data3_dim2->reduce_type = reduce_type_sum;
    data3_dim2->dims[0] = 1;
    data3_dim2->indices[0] = 0;
    data3_dim2->indices[1] = 1;
    data3_dim2->indices[2] = 2;
    data3_dim2->offsets[0] = 0;
    data3_dim2->offsets[1] = 1;
    data3_dim2->offsets[2] = 2;
    dimdesc data3_dim3 = dimdesc_new(1, 3);
    data3->dims[2] = data3_dim3;
    data3_dim3->reduce_type = reduce_type_sum;
    data3_dim3->dims[0] = 2;
    data3_dim3->indices[0] = 0;
    data3_dim3->indices[1] = 1;
    data3_dim3->indices[2] = 2;
    data3_dim3->offsets[0] = 0;
    data3_dim3->offsets[1] = 1;
    data3_dim3->offsets[2] = 2;

    for (uint_t i = 0; i < 3; i++) {
        set->p[i] = datadesc_normalize(set->p[i]);
    }
    datadesc_set_show(set);
    merge_info_piece **mutat = (merge_info_piece **)malloc(sizeof(merge_info_piece *));
    uint_t merge_cnt;
    merge_cnt = datadesc_subset_merge_search(set, mutat);
    for (uint_t i = 0; i < merge_cnt; i++) {
        print_merge_info((*mutat)[i]);
    }
    set = datadesc_subset_merge_1step(set, (*mutat)[0]);
    merge_cnt = datadesc_subset_merge_search(set, mutat);
    for (uint_t i = 0; i < merge_cnt; i++) {
        print_merge_info((*mutat)[i]);
    }
    set = datadesc_subset_merge_1step(set, (*mutat)[0]);
    datadesc_set_show(set);
    /*
        merge_type_1step: 0, 2 ~ 1 2 
        merge_type_1dim: 1 2 
        tuple 0 可以拆分
        merge_type_1step: 0, 1 ~ 0 
        merge_type_1dim: 0 
        datadesc: rank 1######################
        offset0: 0
        dim information: rank 1
        dims[1] ---name index ---->
        (0)
        offsets[2]----->
        0   1   
        indices[2][1]---->
        0   
        1   
        dim information: rank 2
        dims[1] ---name index ---->
        (2)
        offsets[3]----->
        0   1   2   
        indices[3][1]---->
        0   
        1   
        2   
        dim information: rank 3
        dims[1] ---name index ---->
        (1)
        offsets[3]----->
        0   1   2   
        indices[3][1]---->
        0   
        1   
        2   
        ###################################### 
    */
}

void test_datadesc_set_merge() {
    datadesc ori_data;
    char    *dims_name[4] = {"x", "y", "z", "t"};
    uint_t  dims_length[4] = {8, 8, 8, 8};
    ori_data = datadesc_new_array(region_main, "a", endian_little, elem_type_real, 8, 4, dims_name, dims_length);
    datadesc_set dataset = datadesc_set_new(16);
    uint_t dims_in1[1] = {3};
    uint_t dims_in2[1] = {2};
    uint_t dims_in3[1] = {1};
    uint_t dims_in4[1] = {0};
    // FIX: dimmap_f复用的灵活性不好：没有传入n_in&&n_out参数，无法支持逻辑复杂的整个dimmap_f
    datadesc data0 = datadesc_dimmap(ori_data, dimmap_f_even, 1, dims_in1, 1, dims_in1);
    datadesc data1 = datadesc_dimmap(ori_data, dimmap_f_odd, 1, dims_in1, 1, dims_in1);
    datadesc data00 = datadesc_dimmap(data0, dimmap_f_even, 1, dims_in2, 1, dims_in2);
    datadesc data01 = datadesc_dimmap(data0, dimmap_f_odd, 1, dims_in2, 1, dims_in2);
    datadesc data10 = datadesc_dimmap(data1, dimmap_f_even, 1, dims_in2, 1, dims_in2);
    datadesc data11 = datadesc_dimmap(data1, dimmap_f_odd, 1, dims_in2, 1, dims_in2);
    datadesc data000 = datadesc_dimmap(data00, dimmap_f_even, 1, dims_in3, 1, dims_in3);
    datadesc data001 = datadesc_dimmap(data00, dimmap_f_odd, 1, dims_in3, 1, dims_in3);
    datadesc data010 = datadesc_dimmap(data01, dimmap_f_even, 1, dims_in3, 1, dims_in3);
    datadesc data011 = datadesc_dimmap(data01, dimmap_f_odd, 1, dims_in3, 1, dims_in3);
    datadesc data100 = datadesc_dimmap(data10, dimmap_f_even, 1, dims_in3, 1, dims_in3);
    datadesc data101 = datadesc_dimmap(data10, dimmap_f_odd, 1, dims_in3, 1, dims_in3);
    datadesc data110 = datadesc_dimmap(data11, dimmap_f_even, 1, dims_in3, 1, dims_in3);
    datadesc data111 = datadesc_dimmap(data11, dimmap_f_odd, 1, dims_in3, 1, dims_in3);
    dataset->p[0] = datadesc_dimmap(data000, dimmap_f_even, 1, dims_in4, 1, dims_in4);
    dataset->p[1] = datadesc_dimmap(data000, dimmap_f_odd, 1, dims_in4, 1, dims_in4);
    dataset->p[2] = datadesc_dimmap(data001, dimmap_f_even, 1, dims_in4, 1, dims_in4);
    dataset->p[3] = datadesc_dimmap(data001, dimmap_f_odd, 1, dims_in4, 1, dims_in4);
    dataset->p[4] = datadesc_dimmap(data010, dimmap_f_even, 1, dims_in4, 1, dims_in4);
    dataset->p[5] = datadesc_dimmap(data010, dimmap_f_odd, 1, dims_in4, 1, dims_in4);
    dataset->p[6] = datadesc_dimmap(data011, dimmap_f_even, 1, dims_in4, 1, dims_in4);
    dataset->p[7] = datadesc_dimmap(data011, dimmap_f_odd, 1, dims_in4, 1, dims_in4);
    dataset->p[8] = datadesc_dimmap(data100, dimmap_f_even, 1, dims_in4, 1, dims_in4);
    dataset->p[9] = datadesc_dimmap(data100, dimmap_f_odd, 1, dims_in4, 1, dims_in4);
    dataset->p[10] = datadesc_dimmap(data101, dimmap_f_even, 1, dims_in4, 1, dims_in4);
    dataset->p[11] = datadesc_dimmap(data101, dimmap_f_odd, 1, dims_in4, 1, dims_in4);
    dataset->p[12] = datadesc_dimmap(data110, dimmap_f_even, 1, dims_in4, 1, dims_in4);
    dataset->p[13] = datadesc_dimmap(data110, dimmap_f_odd, 1, dims_in4, 1, dims_in4);
    dataset->p[14] = datadesc_dimmap(data111, dimmap_f_even, 1, dims_in4, 1, dims_in4);
    dataset->p[15] = datadesc_dimmap(data111, dimmap_f_odd, 1, dims_in4, 1, dims_in4);
    datadesc_set_set dataset_set = datadesc_set_merge_pre(dataset);
    printf("%ld\n", dataset_set->n);
    dataset = dataset_set->p[0];
    datadesc_set_show(dataset);
    merge_info_piece **mutat = (merge_info_piece **)malloc(sizeof(merge_info_piece *));
    uint_t cnt;
    uint_t single_flag = 1;
    while(single_flag) {
        single_flag = 0;
        cnt = datadesc_subset_merge_search(dataset, mutat);
        /* for (uint_t i = 0; i < cnt; i++) {
            print_merge_info((*mutat)[i]);
        } */
        for (uint_t i = 0; i < cnt; i++) {
            if ((*mutat)[i]->merge_info_type == merge_type_1step && (*mutat)[i]->dims_cnt == 1) {
                print_merge_info((*mutat)[i]);
                dataset = datadesc_subset_merge_1step(dataset, (*mutat)[i]);
                single_flag = 1;
                break;
            }
        }
    }
    dataset->p[0] = datadesc_normalize(dataset->p[0]);
    datadesc_set_show(dataset);
}

void main() {
    //  test_datadesc_spilt();
    //  test_datadesc_set_merge();
    test_datadesc_subset_merge_search();
    //  test_datadesc_subset_merge_1step();
}