
#include	<stdio.h>
#include	<string.h>

#include    <datadesc.h>

// 需要预先知道n_in和n_out值，即两个数组的长度
// 需要预先知道dims_in[]和dims_out[]，即两个数组各位置元素所在维度，保证返回的indices_out是按序的
// 对于相同的indices_in[]，需要输出相同的indices_out[]
// 测试时，dimmap_f功能应该尽可能简单
int_t dimmap_f1(int_t *indices_out, int_t *indices_in){
    // n_in == 2/NULL, n_out == 3;
    int i;
    if (indices_in == NULL){
        indices_out[0] = 128;
        indices_out[1] = 2;
        indices_out[2] = -5;
        return 1;
    } 
    else{
        indices_out[0] = 100;
        if (indices_in[0] >= indices_in[1]){
            indices_out[1] = indices_in[0];
            indices_out[2] = indices_in[1];
        }
        else{
            indices_out[1] = indices_in[1];
            indices_out[2] = indices_in[0];
        }
        return 1;
    }
}


int_t dimmap_f2(int_t *indices_out, int_t *indices_in){
    // n_in == 2, n_out == 2;
    indices_out[0] = indices_in[0] + 1;
    indices_out[1] = indices_in[1] + 1;
    return 1;
}


int_t dimmap_f3(int_t *indices_out, int_t *indices_in){
    // n_in == 2, n_out == 1;
    if (indices_in[0] == 1 && indices_in[1] == -1){
        return NULL;
    }else{
        indices_out[0] = indices_in[0] + 1;
        return 1;
    }
}

int_t dimmap_f4(int_t *indices_out, int_t *indices_in){
    // n_in == 3, n_out == 2;
    if (indices_in[0] == 1 && indices_in[1] == -1 && indices_in[2] == 1 ||
        indices_in[0] == 2 && indices_in[1] == 2 && indices_in[2] == 5){
            indices_out[0] = 999;
            indices_out[1] = 996;
    }else{
        indices_out[0] = indices_in[0] + 1;
        indices_out[1] = indices_in[2] + 1;
    }
    return 1;
}

int_t dimmap_f5(int_t *indices_out, int_t *indices_in){
    // n_in == 3, n_out == 2;
    if (indices_in[0] == 1 && indices_in[1] == -1 && indices_in[2] == 1 ||
        indices_in[0] == 2 && indices_in[1] == 2 && indices_in[2] == 5){
            return NULL;
    }else{
        indices_out[0] = indices_in[0] + 1;
        indices_out[1] = indices_in[1] + 1;
    }
    return 1;
}

int_t dimmap_f6(int_t *indices_out, int_t *indices_in){
    for (int_t i = 0; i < 4; i++){
        indices_out[i] = indices_in[i];
    }
    return 1;
}

int_t dimmap_Null(int_t *indices_out, int_t *indices_in){
    return NULL;
}

void main(){
    /*
    初始datadesc:
    dims: 233, (21, 12)
    offsets: [2, -25, 6], [1, -1]
    indices: [3, 8, -66] × [(1, -1), (-1, 1)]
    offset0: 4
     */
    datadesc_set data_set = datadesc_set_new(1);
    datadesc data = datadesc_new(2);
    data_set->p[0] = data;
    dimdesc data_dim = dimdesc_new(1, 3);
    dimdesc data_dim1 = dimdesc_new(2, 2);
    data->dims[0] = data_dim;
    data->dims[1] = data_dim1;
    data->offset0 = 4;
    data->elem_size = 2;
    data_dim->dims[0] = 233;
    data_dim->offsets[0] = 2;
    data_dim->offsets[1] = -25;
    data_dim->offsets[2] = 6;
    data_dim->indices[0] = 3;
    data_dim->indices[1] = 8;
    data_dim->indices[2] = -66;
    data_dim1->dims[0] = 21;
    data_dim1->dims[1] = 12;
    data_dim1->offsets[0] = 1;
    data_dim1->offsets[1] = -1;
    data_dim1->indices[0] = 1;
    data_dim1->indices[1] = -1;
    data_dim1->indices[2] = -1;
    data_dim1->indices[3] = 1;

    uint_t dims_in[2] = {12, 233};
    uint_t dims_out[3] = {1, 5, 6};
    // dims_in[]重复元素
    uint_t dims_in1[2] = {12, 12};
    datadesc_dimmap(data, dimmap_f1, 3, dims_out, 2, dims_in1);
    // dims_in[]存在p不包含的维度
    uint_t dims_in2[2] = {12, 13};
    uint_t dims_in3[2] = {13, 233};
    datadesc_dimmap(data, dimmap_f1, 3, dims_out, 2, dims_in2);
    datadesc_dimmap(data, dimmap_f1, 3, dims_out, 2, dims_in3);
    // dims_out[]重复元素
    uint_t dims_out1[3] = {1, 6, 1};
    datadesc_dimmap(data, dimmap_f1, 3, dims_out1, 2, dims_in);
    // dims_out[]存在(dims_in[]不包含但是p包含)的维度
    uint_t dims_out2[3] = {1, 21, 5};
    datadesc_dimmap(data, dimmap_f1, 3, dims_out2, 2, dims_in);
    
    // n_in==0且n_out>0
    /*
    理论map结果:
    dims: 233, (12, 21), 6, 1, 5
    offsets: [0, 27, 31], [0, 2], 0, 0, 0
    indices: [8, 3, -66] × [(1, -1), (-1, 1)] × -5 × 128 × 2
    offset0: -48
     */
    data_set->p[0] = datadesc_dimmap(data, dimmap_f1, 3, dims_out, 0, NULL);
    datadesc_set_show(data_set);

    // n_in==0且n_out>0，dimmap_f返回NULL
    data_set->p[0] = datadesc_dimmap(data, dimmap_Null, 3, dims_out, 0, dims_in);
    if (data_set->p[0] == NULL){
        printf("NULL\n\n\n");
    }
    
    // n_in<n_out; dims_out[]维度都不在p中; 
    /* 
    理论map结果:
    dims: (5, 6, 21), 1
    offsets: [0, 2, 27, 29, 31, 33], 0
    indices: [(8, 1, -1), (8, -1, 1), (3, 1, -1), (3, -1, 1), (1, -66, -1), (-1, -66, 1)] × 100
    offset0: -48
     */
    data_set->p[0] = datadesc_dimmap(data, dimmap_f1, 3, dims_out, 2, dims_in);
    datadesc_set_show(data_set);

    // n_in==n_out; dims_out[]含有与dims_in[]共同的维度；
    /* 
    理论map结果:
    dims: 12, (1, 21)
    offsets: [0, 27, 31], [0, 2]
    indices: [9, 4, -65] × [(2, -1), (0, 1)]
    offset0: -48
     */
    uint_t dims_out3[2] = {1, 12};
    data_set->p[0] = datadesc_dimmap(data, dimmap_f2, 2, dims_out3, 2, dims_in);
    datadesc_set_show(data_set);

    // n_in>n_out; 存在完全不受影响的dim; 存在被丢弃的数据点; dims_in[]中多个元素对应同一dim
    /* 
    理论map结果:
    dims: 233, 21
    offsets: [0, 27, 31], 0
    indices: [8, 3, -66] × 0
    offset0: -44
     */
    uint_t dims_in4[2] = {12, 21};
    uint_t dims_out4[1] = {21};
    data_set->p[0] = datadesc_dimmap(data, dimmap_f3, 1, dims_out4, 2, dims_in4);
    datadesc_set_show(data_set);

    // 所有数据点都被舍弃
    data_set->p[0] = datadesc_dimmap(data, dimmap_Null, 1, dims_out4, 2, dims_in4);
    datadesc_set_show(data_set);

    // 副作用检查
    /*
    初始datadesc:
    dims: 233, (21, 12)
    offsets: [2, -25, 6], [1, -1]
    indices: [3, 8, -66] × [(1, -1), (-1, 1)]
    offset0: 4
     */
    data_set->p[0] = data;
    datadesc_set_show(data_set);

    /*
    初始datadesc1:
    dims: (66, 21, 12), 233, (76, 15, 7, 6)
    offsets: [1, -1, 5, 7], [2, -25, 6], [0, 3, 8] 
    indices: [(1, -1, 0), (-1, 1, 0), (2, 2, 1), (1, 1, 1)] × [3, 8, -66]
            × [(9, 1, 1, 0), (1, 5, 2, 0), (1, -1, -1, 1)]
    offset0: 4
     */
    datadesc_set data_set1 = datadesc_set_new(1);
    datadesc data1 = datadesc_new(3);
    data1->offset0 = 4;
    data1->elem_size = 2;
    data_set1->p[0] = data1;
    dimdesc data1_dim = dimdesc_new(3, 4);
    dimdesc data1_dim1 = dimdesc_new(1, 3);
    dimdesc data1_dim2 = dimdesc_new(4, 3);
    data1->dims[0] = data1_dim;
    data1->dims[1] = data1_dim1;
    data1->dims[2] = data1_dim2;
    uint_t temp_data1_dim_dims[3] = {66, 21, 12};
    memcpy(data1_dim->dims, temp_data1_dim_dims, sizeof(uint_t) * 3);
    int_t temp_data1_dim_offsets[4] = {1, -1, 5, 7};
    memcpy(data1_dim->offsets, temp_data1_dim_offsets, sizeof(int_t) * 4);
    int_t temp_data1_dim_indices[12] = {1, -1, 0, -1, 1, 0, 2, 2, 1, 1, 1, 1};
    memcpy(data1_dim->indices, temp_data1_dim_indices, sizeof(int_t) * 12);
    data1_dim1->dims[0] = 233;
    int_t temp_data1_dim1_offsets[3] = {2, -25, 6};
    memcpy(data1_dim1->offsets, temp_data1_dim1_offsets, sizeof(int_t) * 3);
    int_t temp_data1_dim1_indices[3] = {3, 8, -66};
    memcpy(data1_dim1->indices, temp_data1_dim1_indices, sizeof(int_t) * 3);
    uint_t temp_data1_dim2_dims[4] = {76, 15, 7, 6};
    memcpy(data1_dim2->dims, temp_data1_dim2_dims, sizeof(uint_t) * 4);
    int_t temp_data1_dim2_offsets[3] = {0, 3, 8};
    memcpy(data1_dim2->offsets, temp_data1_dim2_offsets, sizeof(int_t) * 3);
    int_t temp_data1_dim2_indices[12] = {9, 1, 1, 0, 1, 5, 2, 0, 1, -1, -1, 1};
    memcpy(data1_dim2->indices, temp_data1_dim2_indices, sizeof(int_t) * 12);


    // 验证dim_out_p剔除dims_in[]维度的实现
    /* 
    理论map结果:
    dims: 233, (6, 7, 76), (12, 21, 66)
    offsets: [0, 27, 31], [0, 3, 8], [0, 2, 6, 8]
    indices: [8, 3, -66] × [(0, 2, 9), (0, 6, 1), (1, 0, 1)] ×
            [(0, 1, -1), (0, -1, 1), (1, 2, 2), (1, 1, 1)] 
    offset0: -48
    错误实现：
    dims: (66, 21, 12), 233, (7, 76, 15)
     */
    uint_t dims_in5[2] = {15, 7};
    uint_t dims_out5[1] = {7};
    data_set1->p[0] = datadesc_dimmap(data1, dimmap_f3, 1, dims_out5, 2, dims_in5);
    datadesc_set_show(data_set1);

    // 不同indices_in，相同indices_out查重
    uint_t dims_in6[3] = {7, 21, 15};
    uint_t dims_out6[2] = {7, 15};
    data_set1->p[0] = datadesc_dimmap(data1, dimmap_f4, 2, dims_out6, 3, dims_in6);
    if(data_set1->p[0] == NULL){
        printf("NULL\n\n\n");
    }

    // 存在完全不受影响的dim; 存在被丢弃的数据点; dims_in[]中多个元素对应同一dim; dims_out[]完全被dims_in[]包含
    /* 
    理论map结果:
    dims: 233, (6, 7, 12, 15, 66, 76)
    offsets: [0, 27, 31], [0, 3, 5, 6, 8, 8, 10, 11, 14, 16] 
    indices: [8, 3, -66] × 
            [(0, 2, 0, 2, -1, 9), (0, 3, 0, 2, -1, 1), (0, 3, 0, 0, 1, 1), (0, 2, 1, 3, 2, 9), (0, 2, 1, 2, 1, 9),
            (1, 0, 0, 2, -1, 1), (1, 0, 0, 0, 1, 1), (0, 3, 1, 2, 1, 1), (1, 0, 1, 3, 2, 1), (1, 0, 1, 2, 1, 1)]
    offset0: -48
     */
    data_set1->p[0] = datadesc_dimmap(data1, dimmap_f5, 2, dims_out6, 3, dims_in6);
    datadesc_set_show(data_set1);

    // 不同indices_in，相同indices_out查重：增强
    uint_t dims_in7[4] = {7, 21, 15, 66};
    data_set1->p[0] = datadesc_dimmap(data1, dimmap_f5, 2, dims_out6, 4, dims_in7);
    if (data_set1->p[0] == NULL){
        printf("NULL\n\n\n");
    }

    // 副作用检查
    /*
    初始datadesc1:
    dims: (66, 21, 12), 233, (76, 15, 7, 6)
    offsets: [1, -1, 5, 7], [2, -25, 6], [0, 3, 8] 
    indices: [(1, -1, 0), (-1, 1, 0), (2, 2, 1), (1, 1, 1)] × [3, 8, -66]
            × [(9, 1, 1, 0), (1, 5, 2, 0), (1, -1, -1, 1)]
    offset0: 4
     */
    data_set1->p[0] = data1;
    datadesc_set_show(data_set1);

    // 覆盖input_of_dim[j]--语句的测试，实际indices不变
    /*
        dims[7] ---name index ---->
        (6, 7, 12, 15, 21, 66, 76)
        offsets[12]----->
        0   2   3   5   6   8   8   9   10   11   14   16   
        indices[12][7]---->
        0   1   0   1   1   -1   9   
        0   1   0   1   -1   1   9   
        0   2   0   5   1   -1   1   
        0   2   0   5   -1   1   1   
        0   1   1   1   2   2   9   
        1   -1   0   -1   1   -1   1   
        0   1   1   1   1   1   9   
        0   2   1   5   2   2   1   
        1   -1   0   -1   -1   1   1   
        0   2   1   5   1   1   1   
        1   -1   1   -1   2   2   1   
        1   -1   1   -1   1   1   1  
    */
    uint_t dims_in8[4] = {66, 21, 233, 15};
    uint_t dims_out7[4] = {66, 21, 233, 15}; 
    data_set1->p[0] = datadesc_dimmap(data1, dimmap_f6, 4, dims_out7, 4, dims_in8);
    datadesc_set_show(data_set1);
}