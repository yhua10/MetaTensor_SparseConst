#include	"plugins.h"
#include	"gc_malloc.h"
#include    "datadesc.h"
#include    <string.h>

static int_t calc_func_cnt = 0;
static int_t set_cnt = 0;
static int_t data_cnt = 0;
static datadesc* data_set = NULL;
static datadesc_set* set_set = NULL;
static int_t dim_max = 0;
static int_t dim_min = 0;
// 原地hash，dim即index
static int_t** dim_to_common_indices_posi = NULL;
static int_t** dim_to_common_indices_neg = NULL;
static int_t* dim_to_common_indices_cnt_posi = NULL;
static int_t* dim_to_common_indices_cnt_neg = NULL;
// 涉及到dim的datadesc_set们（
static datadesc_set** dim_to_set_set_posi = NULL;
static datadesc_set** dim_to_set_set_neg = NULL;
static int_t* dim_to_set_set_cnt_posi = NULL;
static int_t* dim_to_set_set_cnt_neg = NULL;
static char** data_to_valid_name = NULL;
static char** data_to_off_name = NULL;
static char** data_to_off_base_name = NULL;
static char** data_to_const_off_array_name = NULL;
static char** data_to_const_valid_array_name = NULL;

void unary_code_gen(target t, block ob, char *res_name);

// FIXME: 测试时，调低初始容量以测试自动扩容功能
int_t add_to_data_set(datadesc data) {
    static int_t max_data_cnt = 64;
    if (data_set == NULL) {
        data_set = (datadesc *)malloc(sizeof(datadesc) * max_data_cnt);
    } else if (data_cnt == max_data_cnt) {
        max_data_cnt *= 2;
        datadesc* data_set_new = (datadesc *)malloc(sizeof(datadesc) * max_data_cnt);
        memcpy(data_set_new, data_set, sizeof(datadesc) * data_cnt);
        data_set = data_set_new;
    }
    data_set[data_cnt++] = data;
    return data_cnt;
}


int_t add_to_set_set(datadesc_set set) {
    set->data_start_index = data_cnt;
    static int_t max_set_cnt = 32;
    if (set_set == NULL) {
        set_set = (datadesc_set *)malloc(sizeof(datadesc_set) * max_set_cnt);
    } else if (set_cnt == max_set_cnt) {
        max_set_cnt *= 2;
        datadesc_set* set_set_new = (datadesc_set *)malloc(sizeof(datadesc_set) * max_set_cnt);
        memcpy(set_set_new, set_set, sizeof(datadesc_set) * set_cnt);
        set_set = set_set_new;
    }
    set_set[set_cnt++] = set;
    return set_cnt;
}


void collect_block_datadesc(block ob) {
    if (ob->type == block_type_datadesc_set) {
        add_to_set_set(ob->p.d);
        for (int_t i = 0; i < ob->p.d->n; i++) {
            add_to_data_set(ob->p.d->p[i]);
        }
    } else if (ob->type == block_type_con_ser || ob->type == block_type_con_par_int) {
        collect_block_datadesc(ob->p.b[0]);
        collect_block_datadesc(ob->p.b[1]);
    }
    return ;
}


int_t ini_common_indices(datadesc_set set, int_t dim, int_t **common_indices, int_t **common_indices_valid) {
    int_t indices_cnt = 0;
    int_t *indices = NULL;
    int_t indices_cnt_max = 128;
    indices = (int_t *)malloc(sizeof(int_t) * indices_cnt_max);

    for (int_t i = 0; i < set->n; i++) {
        datadesc cur_data = set->p[i];
        for (int_t j = 0; j < cur_data->n_dim && cur_data->dims[j] != NULL; j++) {
            if (cur_data->dims[j]->dims[0] == dim) {

                if (indices_cnt + cur_data->dims[j]->n_entry > indices_cnt_max) {
                    indices_cnt_max *= 2;
                    int_t *indices_new = (int_t *)malloc(sizeof(int_t) * indices_cnt_max);
                    memcpy(indices_new, indices, sizeof(int_t) * indices_cnt);
                    indices = indices_new;
                }

                for (int_t k = 0; k < cur_data->dims[j]->n_entry; k++) {
                    int_t cur_indice = cur_data->dims[j]->indices[k];
                    int_t m = 0;
                    for (; m < indices_cnt && indices[m] != cur_indice; m++);
                    if (m == indices_cnt) {
                        indices[indices_cnt++] = cur_indice;
                    }
                }

            }
        }
    }

    *common_indices = (int_t *)malloc(sizeof(int_t) * indices_cnt);
    memcpy(*common_indices, indices, sizeof(int_t) * indices_cnt);
    *common_indices_valid = (int_t *)malloc(sizeof(int_t) * indices_cnt);
    for (int_t i = 0; i < indices_cnt; i++) {
        // FIXME: 语法
        (*common_indices_valid)[i] = 1;
    }

    return indices_cnt;
}


void confirm_common_indices(datadesc_set set, int_t dim, int_t *common_indices, int_t **common_indices_valid, int_t common_indices_cnt_ini) {
    int_t *tmp_common_indices_valid = (int_t *)malloc(sizeof(int_t) * common_indices_cnt_ini);
    memset(tmp_common_indices_valid, 0, sizeof(int_t) * common_indices_cnt_ini);
    
    for (int_t i = 0; i < set->n; i++) {
        datadesc cur_data = set->p[i];
        for (int_t j = 0; j < cur_data->n_dim && cur_data->dims[j] != NULL; j++) {
            if (cur_data->dims[j]->dims[0] == dim) {

                for (int_t k = 0; k < cur_data->dims[j]->n_entry; k++) {
                    for (int_t m = 0; m < common_indices_cnt_ini; m++) {
                        if (common_indices[m] == cur_data->dims[j]->indices[k]) {
                            tmp_common_indices_valid[m] = 1;
                            break;
                        }
                    }
                }
            }
        }
    }

    for (int_t i = 0; i < common_indices_cnt_ini; i++) {
        if (tmp_common_indices_valid[i] == 0) {
            (*common_indices_valid)[i] = 0;
        }
    }

    return ;
}


int_t collect_common_indices(int_t dim, int_t **res_common_indices, int_t posi) {
    
    int_t *common_indices = NULL;
    int_t *common_indices_valid = NULL;
    int_t common_indices_cnt_ini = 0;
    int_t common_indices_cnt = 0;

    int_t flag = 0;
    int_t first_flag = 1;
    int_t dim_to_set_set_cnt = 0;
    datadesc_set* dim_to_set_set_tmp = (datadesc_set *)malloc(sizeof(datadesc_set) * set_cnt);

    for (int_t i = 0; i < set_cnt; i++) {
        flag = 0;

        for (int_t j = 0; !flag && j < set_set[i]->n; j++) {
            for (int_t k = 0; k < set_set[i]->p[j]->n_dim && set_set[i]->p[j]->dims[k] != NULL; k++) {
                if (set_set[i]->p[j]->dims[k]->dims[0] == dim) {
                    if (first_flag) {
                        common_indices_cnt_ini = ini_common_indices(set_set[i], dim, &common_indices, &common_indices_valid);
                        flag = 1;
                        first_flag = 0;
                        break;
                    } else {
                        confirm_common_indices(set_set[i], dim, common_indices, &common_indices_valid, common_indices_cnt_ini);
                        flag = 1;
                        break;
                    }
                    
                }
            }
        }

        if (flag) {
            dim_to_set_set_tmp[dim_to_set_set_cnt++] = set_set[i];
        }

    }

    datadesc_set* dim_to_set_set_res = (datadesc_set *)malloc(sizeof(datadesc_set) * dim_to_set_set_cnt);
    for (int_t i = 0; i < dim_to_set_set_cnt; i++) {
        dim_to_set_set_res[i] = dim_to_set_set_tmp[i];
    }

    if (posi) {
        dim_to_set_set_cnt_posi[dim] = dim_to_set_set_cnt;
        dim_to_set_set_posi[dim] = dim_to_set_set_res;
    } else {
        dim_to_set_set_cnt_neg[0 - dim] = dim_to_set_set_cnt;
        dim_to_set_set_neg[0 - dim] = dim_to_set_set_res;
    }

    for (int_t i = 0; i < common_indices_cnt_ini; i++) {
        if (common_indices_valid[i] == 1) {
            common_indices_cnt++;
        }
    }

    // FIXME: common_indices_cnt = 0?

    *res_common_indices = (int_t *)malloc(sizeof(int_t) * common_indices_cnt);
    int_t tmp_cnt = 0;
    for (int_t i = 0; i < common_indices_cnt_ini; i++) {
        if (common_indices_valid[i] == 1) {
            (*res_common_indices)[tmp_cnt++] = common_indices[i];
        }
    }
    
    return common_indices_cnt;
}

void apply_common_indices(datadesc_set *set_set, int_t set_cnt, int_t dim, int_t common_indices_cnt, int_t *common_indices) {
    // TODO: 按照indices排序？
    // 需要copy on write，注意同步修改相关的全局数组
    for (int_t i = 0; i < set_cnt; i++) {
        for (int_t j = 0; j < set_set[i]->n; j++) {
            datadesc cur_data = set_set[i]->p[j];
            for (int_t k = 0; k < cur_data->n_dim && cur_data->dims[k] != NULL; k++) {
                if (cur_data->dims[k]->dims[0] == dim) {
                    
                    dimdesc cur_dim = cur_data->dims[k];
                    int_t match_cnt = 0;
                    for (int_t m = 0; m < cur_dim->n_entry; m++) {
                        for (int_t n = 0; n < common_indices_cnt; n++) {
                            if (cur_dim->indices[m] == common_indices[n]) {
                                cur_dim->indices[match_cnt] = cur_dim->indices[m];
                                cur_dim->offsets[match_cnt++] = cur_dim->offsets[m];
                                break;
                            }
                        }
                    }
                    // FIXME: 此时，因为common_indices应该被cur_dim包含，common_indices_cnt == match_cnt？
                        // 错误的，common_indices不一定被单个datadesc包含，而是被set包含
                    cur_dim->n_entry = match_cnt;

                }
            }
        }
    }

    return ;
}

void generator_code_gen(target t, block ob) {
    emit_start(t);

    int_t i = ob->p.b[0]->p.w[0];
    // NOTE：i = 0 - dim = dim_index in dim_to_X
    i = 0 - i;

    {
        char *off_type = "int32_t ";
        char *valid_type = "int32_t ";
        int_t cur_common_indices_cnt = dim_to_common_indices_cnt_neg[i];

        for (int_t j = 0; j < dim_to_set_set_cnt_neg[i]; j++) {
            datadesc_set cur_datadesc_set = dim_to_set_set_neg[i][j];
            for (int_t k = 0; k < cur_datadesc_set->n; k++) {
                char *tmp_const_off_array_name = token_new();
                char *tmp_const_valid_array_name = token_new();
                dimdesc cur_dim = NULL;

                emit_push("const ", off_type, tmp_const_off_array_name, "[", itoa(cur_common_indices_cnt), "] = {");
                for (int_t m = 0; m < cur_datadesc_set->p[k]->n_dim && cur_datadesc_set->p[k]->dims[m] != NULL; m++) {
                    if (cur_datadesc_set->p[k]->dims[m]->dims[0] == 0 - i) {
                        cur_dim = cur_datadesc_set->p[k]->dims[m];
                        break;
                    }
                }
                for (int_t m = 0; m < cur_common_indices_cnt; m++) {
                    if (m > 0) emit_push(", ");

                    if (cur_dim == NULL) {
                        emit_push("0");
                    } else {
                        int_t n = 0;
                        for (; n < cur_dim->n_entry; n++) {
                            if (cur_dim->indices[n] == dim_to_common_indices_neg[i][m]) {
                                emit_push(itoa(cur_dim->offsets[n]));
                                break;
                            }
                        }
                        // datadesc存在该维度，但不含有该indices: 将在valid_const_array中体现，此处用"0"在off_const_array中占位
                        if (n == cur_dim->n_entry) {
                            emit_push("0");
                        }
                    }
                }
                emit_push("};");

                emit_push("const ", valid_type, tmp_const_valid_array_name, "[", itoa(cur_common_indices_cnt), "] = {");
                for (int_t m = 0; m < cur_common_indices_cnt; m++) {
                    if (m > 0) emit_push(", ");

                    if (cur_dim == NULL) {
                        emit_push("1");
                    }else {
                        int_t n = 0;
                        for (; n < cur_dim->n_entry; n++) {
                            if (cur_dim->indices[n] == dim_to_common_indices_neg[i][m]) {
                                emit_push("1");
                                break;
                            }
                        }
                        if (n == cur_dim->n_entry) {
                            emit_push("0");
                        }
                    }
                }
                emit_push("};");

                int_t cur_data_index = cur_datadesc_set->data_start_index + k;
                data_to_const_off_array_name[cur_data_index] = tmp_const_off_array_name;
                data_to_const_valid_array_name[cur_data_index] = tmp_const_valid_array_name;
            }
        }

        char *tmp_loop_var = token_new();
        emit_push("for (int32_t ", tmp_loop_var, "=0; ", tmp_loop_var, "<", itoa(cur_common_indices_cnt), "; ",
                     tmp_loop_var, "++) {");
        
        for (int_t j = 0; j < dim_to_set_set_cnt_neg[i]; j++) {
            datadesc_set cur_datadesc_set = dim_to_set_set_neg[i][j];
            for (int_t k = 0; k < cur_datadesc_set->n; k++) {
                char *tmp_off_name = token_new();
                char *tmp_valid_name = token_new();
                int_t cur_data_index = cur_datadesc_set->data_start_index + k;
                emit_push(off_type, tmp_off_name, "=", data_to_off_name[cur_data_index], "+", data_to_const_off_array_name[cur_data_index],
                             "[", tmp_loop_var, "];");
                data_to_off_name[cur_data_index] = tmp_off_name;
                emit_push(valid_type, tmp_valid_name, "=", data_to_valid_name[cur_data_index], "&", data_to_const_valid_array_name[cur_data_index],
                             "[", tmp_loop_var, "];");
                data_to_valid_name[cur_data_index] = tmp_valid_name;
            }
        }

    }

    emit_finish();
}

// 接受整个串连块
void reduce_code_gen(target t, block ob, char *res_name) {

    int_t g_cnt = 0;
    block reduce_block = ob->p.b[1];

    // ini_res_name: 根据reduce类型不同，res_name初始值也不同
        // FIXME: 是否修改为在调用reduce_code_gen()处初始化?

    // TODO: query_type for res_name；实现前统一使用double
    char *res_type = "double ";
    char *reduce_type = NULL;

    emit_start(t);
    emit_push(res_type, res_name, "=");

    // FIXME: 需要保证，每次迭代时，R必须有操作数，否则初始值毫无意义，而应该为reduce运算也添加valid标记
    switch (ob->p.b[0]->type)
    {
        case block_builtin_reduce_sum:
        {
            emit_push("0;");
            // FIXME: 语法合法？
            reduce_type = "+=";
            break;
        }
        case block_builtin_reduce_prod:
        {
            emit_push("1;");
            reduce_type = "*=";
            break;
        }
       
    }

    // 迭代处理 reduce_block 尾部的[G]+
    while (reduce_block->type == block_type_con_ser && reduce_block->p.b[1]->type == block_type_con_ser
             && reduce_block->p.b[1]->p.b[1]->type == block_builtin_generator_all) 
    {
        g_cnt += 1;
        generator_code_gen(t, reduce_block->p.b[1]);
        reduce_block = reduce_block->p.b[0];
    }

    // FIXME: 是否有其他情况
    if (reduce_block->type == block_type_con_ser) 
    {
        char *res_name_new = token_new();

        if (reduce_block->p.b[0]->type > block_builtin_reduce_first && reduce_block->p.b[0]->type < block_builtin_reduce_last) 
            reduce_code_gen(t, reduce_block, res_name_new);
        else if (reduce_block->p.b[0]->type > block_builtin_unary_first && reduce_block->p.b[0]->type < block_builtin_unary_last)
            unary_code_gen(t, reduce_block, res_name_new);

        // FIXME: 随着运算种类的扩充，无法统一归纳为该模式
        emit_push(res_name, reduce_type, res_name_new, ";");

    } 
    else if (reduce_block->type == block_type_con_par_int) 
    {
        // FIXME: 并联2部分一定是 ( [R] - X ) ?
        char *res_name_new1 = token_new();
        char *res_name_new2 = token_new();
        reduce_code_gen(t, reduce_block->p.b[0], res_name_new1);
        reduce_code_gen(t, reduce_block->p.b[1], res_name_new2);
        emit_push(res_name, reduce_type, res_name_new1, ";");
        emit_push(res_name, reduce_type, res_name_new2, ";");
    }
    else if (reduce_block->type == block_type_datadesc_set) 
    {
        datadesc_set cur_datadesc_set = reduce_block->p.d;
        for (int_t i = 0; i < cur_datadesc_set->n; i++) {
            int_t data_index = cur_datadesc_set->data_start_index + i;
            emit_push("if (", data_to_valid_name[data_index], ")");
            emit_push(res_name, reduce_type, data_to_off_base_name[data_index],
                 "[", data_to_off_name[data_index], "];");
        }

    }
    
    for (int_t i = 0; i < g_cnt; i++) {
        emit_push("}");
    }

    emit_finish();

}

// 接受整个串连块
void unary_code_gen(target t, block ob, char *res_name) {
    
    int_t g_cnt = 0;
    block unary_block = ob->p.b[1];

    // ini_res_name: 根据reduce类型不同，res_name初始值也不同
        // FIXME: 是否修改为在调用reduce_code_gen()处初始化?

    // TODO: query_type for res_name；实现前统一使用double
    char *res_type = "double ";
    char *unary_type = NULL;

    emit_start(t);
    emit_push(res_type, res_name, "=");

    // FIXME: 需要保证，每次迭代时，U必须有操作数，否则初始值毫无意义，而应该为reduce运算也添加valid标记
    switch (ob->p.b[0]->type)
    {
        case block_builtin_unary_minus:
        {
            emit_push("0;");
            unary_type = "-=";
            break;
        }
        case block_builtin_unary_sin:
        {
            emit_push("0;");
            unary_type = "sin(";
            break;
        }
        case block_builtin_unary_cos:
        {
            emit_push("0;");
            unary_type = "cos(";
            break;
        }
    
    }

    // 迭代处理 unary_block 尾部的[G]+
    while (unary_block->type == block_type_con_ser && unary_block->p.b[1]->type == block_type_con_ser
             && unary_block->p.b[1]->p.b[1]->type == block_builtin_generator_all) 
    {
        g_cnt += 1;
        generator_code_gen(t, unary_block->p.b[1]);
        unary_block = unary_block->p.b[0];
    }

    // FIXME: 是否有其他情况
    if (unary_block->type == block_type_con_ser) 
    {
        char *res_name_new = token_new();

        if (unary_block->p.b[0]->type > block_builtin_reduce_first && unary_block->p.b[0]->type < block_builtin_reduce_last) 
            reduce_code_gen(t, unary_block, res_name_new);
        else if (unary_block->p.b[0]->type > block_builtin_unary_first && unary_block->p.b[0]->type < block_builtin_unary_last)
            unary_code_gen(t, unary_block, res_name_new);

        if (ob->p.b[0]->type == block_builtin_unary_minus)
            emit_push(res_name, unary_type, res_name_new, ";");
        else if (ob->p.b[0]->type == block_builtin_unary_sin || ob->p.b[0]->type == block_builtin_unary_cos) 
            emit_push(res_name, "=", unary_type, res_name_new, ");");

    } 
    else if (unary_block->type == block_type_datadesc_set) 
    {
        datadesc_set cur_datadesc_set = unary_block->p.d;
        for (int_t i = 0; i < cur_datadesc_set->n; i++) {
            // FIXME: 需要确保，同时间datadesc_set中有且只有一个datadesc有效
            int_t data_index = cur_datadesc_set->data_start_index + i;
            emit_push("if (", data_to_valid_name[data_index], ")");

            if (ob->p.b[0]->type == block_builtin_unary_minus)
                emit_push(res_name, unary_type, data_to_off_base_name[data_index],
                    "[", data_to_off_name[data_index], "];");
            else if (ob->p.b[0]->type == block_builtin_unary_sin || ob->p.b[0]->type == block_builtin_unary_cos)
                emit_push(res_name, "=", unary_type, data_to_off_base_name[data_index],
                    "[", data_to_off_name[data_index], "]);");

        }

    }

    for (int_t i = 0; i < g_cnt; i++) {
        emit_push("}");
    }

    emit_finish();
}

// TODO: assert
datadesc_set block2data_dense(target t, block ob) {
    
    collect_block_datadesc(ob);

    // 统计正/负维度分别有哪些
        // FIXME: datadesc.c文件中有相关信息，但是static
        // FIXME: 仅需要统计dim最大值/最小值？
    for (int_t i = 0; i < data_cnt; i++) {
        datadesc cur_data = data_set[i];
        for (int_t j = 0; j < cur_data->n_dim && cur_data->dims[j] != NULL; j++) {
            if (cur_data->dims[j]->dims[0] > dim_max) {
                dim_max = cur_data->dims[j]->dims[0];
            } else if (cur_data->dims[j]->dims[0] < dim_min) {
                dim_min = cur_data->dims[j]->dims[0];
            }
        }
    }

    // FIXME: dim不一定连续，更重要的，block涉及到的dim，并不是[1, dim_max]
    dim_to_common_indices_posi = (int_t **)malloc(sizeof(int_t *) * (dim_max + 1));
    dim_to_common_indices_neg = (int_t **)malloc(sizeof(int_t *) * (1 - dim_min));
    dim_to_common_indices_cnt_posi = (int_t *)malloc(sizeof(int_t) * (dim_max + 1));
    dim_to_common_indices_cnt_neg = (int_t *)malloc(sizeof(int_t) * (1 - dim_min));
    dim_to_set_set_posi = (datadesc_set **)malloc(sizeof(datadesc_set *) * (dim_max + 1));
    dim_to_set_set_neg = (datadesc_set **)malloc(sizeof(datadesc_set *) * (1 - dim_min));
    dim_to_set_set_cnt_posi = (int_t *)malloc(sizeof(int_t) * (dim_max + 1));
    dim_to_set_set_cnt_neg = (int_t *)malloc(sizeof(int_t) * (1 - dim_min));
    data_to_const_off_array_name = (char **)malloc(sizeof(char *) * data_cnt);
    data_to_const_valid_array_name = (char **)malloc(sizeof(char *) * data_cnt);
    data_to_off_name = (char **)malloc(sizeof(char *) * data_cnt);
    data_to_valid_name = (char **)malloc(sizeof(char *) * data_cnt);
    data_to_off_base_name = (char **)malloc(sizeof(char *) * data_cnt);
    

    for (int_t i = 1; i <= dim_max; i++) {
        int_t common_indices_cnt;
        int_t *common_indices = NULL;
        common_indices_cnt = collect_common_indices(i, &common_indices, 1);
        dim_to_common_indices_posi[i] = common_indices;
        dim_to_common_indices_cnt_posi[i] = common_indices_cnt;
        // FIXME: 是否立即apply?
        
    }

    // FIXME: 对于临时维度，解析到[G]再处理是否更好？
    for (int_t i = -1; i >= dim_min; i--) {
        int_t common_indices_cnt;
        int_t *common_indices = NULL;
        common_indices_cnt = collect_common_indices(i, &common_indices, 0);
        dim_to_common_indices_neg[0 - i] = common_indices;
        dim_to_common_indices_cnt_neg[0 - i] = common_indices_cnt;
    }

    // TODO: 构造res_datadesc_set
    datadesc_set res_set = datadesc_set_new(1);
    // FIXME: dim_max废弃后，res_data的初始化也需要大改
    // FIXME: 从data_set[0]拷贝的字段，需要重新考量

    int_t valid_dim_cnt = 0;
    for (int_t i = 1; i <= dim_max; i++) 
    {
        if (dim_to_common_indices_cnt_posi[i] != 0) 
        {
            valid_dim_cnt += 1;
        }
    }
    datadesc res_data = datadesc_new(valid_dim_cnt);
    res_set->p[0] = res_data;
    res_data->elem_size = data_set[0]->elem_size;
    res_data->elem_type = data_set[0]->elem_type;
    res_data->endian = data_set[0]->endian;
    res_data->offset0 = 0;
    res_data->region = region_main;
    // FIXME: name res_data
    res_data->ref.p_refname = "tmp_res";
    int_t cur_off_span = 1;
    // FIXME: res_data数据排布如何考虑？
    int_t cur_dim_index = 0;
    for (int_t i = 1; i <= dim_max; i++) {
        int_t cur_common_indices_cnt = dim_to_common_indices_cnt_posi[i];
        if (cur_common_indices_cnt == 0) 
        {
            continue;
        }
        dimdesc cur_dim = dimdesc_new(1, cur_common_indices_cnt);
        res_data->dims[cur_dim_index++] = cur_dim;
        cur_dim->dims[0] = i;
        cur_dim->reduce_type = data_set[0]->dims[0]->reduce_type;
        int_t cur_off = 0;
        for (int_t j = 0; j < cur_common_indices_cnt; j++) {
            cur_dim->indices[j] = dim_to_common_indices_posi[i][j];
            cur_dim->offsets[j] = cur_off;
            cur_off += cur_off_span;
        }
        cur_off_span = cur_off;
    }

    
    emit_start(t);

    // 函数声明不由这个函数处理；各datadesc的起始地址，valid初始值，off初始值
    
    int_t res_elements_cnt = 1;
    for (int_t i = 0; i < res_data->n_dim && res_data->dims[i] != NULL; i++) 
    {
        res_elements_cnt *= res_data->dims[i]->n_entry;
    }
    // 在最外层{}之外初始化res_data
    // TODO: query_type
    char *res_type = "double ";
    emit_push(res_type, "*", res_data->ref.p_refname, " = malloc(", itoa(res_data->elem_size), " * ", itoa(res_elements_cnt), ");");
    
    emit_push("{");
    
    char * res_to_off_base_name = token_new(); 
    emit_push(res_type, "*", res_to_off_base_name, "=(", res_type, "*)((char *)(",
                res_data->ref.p_refname, ")+", itoa(res_data->offset0), ");");
    // NOTE: confirm invariant: data在data_set的索引 = set->data_start_index + data在datadesc_set的索引
    for (int_t i = 0; i < data_cnt; i++) {
        data_to_off_base_name[i] = token_new();
        // TODO: query_type
        char *data_type = "double ";
        emit_push(data_type, "*", data_to_off_base_name[i], "=(", data_type, "*)((char *)(",
             data_set[i]->ref.p_refname, ")+", itoa(data_set[i]->offset0), ");");
    }

    char *res_to_off_name = token_new();
    char *off_type = "int32_t ";
    emit_push(off_type, res_to_off_name, "=0;");
    for (int_t i = 0; i < data_cnt; i++) {
        data_to_off_name[i] = token_new();
        char *off_type = "int32_t ";
        emit_push(off_type, data_to_off_name[i], "=0;");
    }

    for (int_t i = 0; i < data_cnt; i++) {
        data_to_valid_name[i] = token_new();
        char *valid_type = "int32_t ";
        emit_push(valid_type, data_to_valid_name[i], "=1;");
    }

    for (int_t i = 1; i <= dim_max; i++) {
        int_t cur_common_indices_cnt = dim_to_common_indices_cnt_posi[i];
        if (cur_common_indices_cnt == 0) 
        {
            continue;
        }
        // 外层循环
        char *off_type = "int32_t ";
        char *valid_type = "int32_t ";
        
        char *tmp_res_const_off_array_name = token_new();
        emit_push("const ", off_type, tmp_res_const_off_array_name, "[", itoa(cur_common_indices_cnt), "] = {");
        for (int_t j = 0; j < res_data->n_dim && res_data->dims[j] != NULL; j++) {
            if (res_data->dims[j]->dims[0] == i) {
                dimdesc cur_dim = res_data->dims[j];
                for (int_t k = 0; k < cur_common_indices_cnt; k++) {
                    if (k > 0) emit_push(", ");
                    int_t m = 0;
                    for (; m < cur_dim->n_entry; m++) {
                        if (cur_dim->indices[m] == dim_to_common_indices_posi[i][k]) {
                            emit_push(itoa(cur_dim->offsets[m]));
                            break;
                        }
                    }
                }
                break;
            }
        }
        emit_push("};");

        for (int_t j = 0; j < dim_to_set_set_cnt_posi[i]; j++) {
            datadesc_set cur_datadesc_set = dim_to_set_set_posi[i][j];
            for (int_t k = 0; k < cur_datadesc_set->n; k++) {
                char *tmp_const_off_array_name = token_new();
                char *tmp_const_valid_array_name = token_new();
                dimdesc cur_dim = NULL;

                emit_push("const ", off_type, tmp_const_off_array_name, "[", itoa(cur_common_indices_cnt), "] = {");
                for (int_t m = 0; m < cur_datadesc_set->p[k]->n_dim && cur_datadesc_set->p[k]->dims[m] != NULL; m++) {
                    if (cur_datadesc_set->p[k]->dims[m]->dims[0] == i) {
                        cur_dim = cur_datadesc_set->p[k]->dims[m];
                        break;
                    }
                }
                for (int_t m = 0; m < cur_common_indices_cnt; m++) {
                    if (m > 0) emit_push(", ");

                    if (cur_dim == NULL) {
                        emit_push("0");
                    } else {
                        int_t n = 0;
                        for (; n < cur_dim->n_entry; n++) {
                            if (cur_dim->indices[n] == dim_to_common_indices_posi[i][m]) {
                                emit_push(itoa(cur_dim->offsets[n]));
                                break;
                            }
                        }
                        // datadesc存在该维度，但不含有该indices: 将在valid_const_array中体现，此处用"0"在off_const_array中占位
                        if (n == cur_dim->n_entry) {
                            emit_push("0");
                        }
                    }
                }
                emit_push("};");

                emit_push("const ", valid_type, tmp_const_valid_array_name, "[", itoa(cur_common_indices_cnt), "] = {");
                for (int_t m = 0; m < cur_common_indices_cnt; m++) {
                    if (m > 0) emit_push(", ");

                    if (cur_dim == NULL) {
                        emit_push("1");
                    }else {
                        int_t n = 0;
                        for (; n < cur_dim->n_entry; n++) {
                            if (cur_dim->indices[n] == dim_to_common_indices_posi[i][m]) {
                                emit_push("1");
                                break;
                            }
                        }
                        if (n == cur_dim->n_entry) {
                            emit_push("0");
                        }
                    }
                }
                emit_push("};");

                int_t cur_data_index = cur_datadesc_set->data_start_index + k;
                data_to_const_off_array_name[cur_data_index] = tmp_const_off_array_name;
                data_to_const_valid_array_name[cur_data_index] = tmp_const_valid_array_name;
            }
        }

        char *tmp_loop_var = token_new();
        emit_push("for (int32_t ", tmp_loop_var, "=0; ", tmp_loop_var, "<", itoa(cur_common_indices_cnt), "; ",
                     tmp_loop_var, "++) {");
        
        char *res_tmp_off_name = token_new();
        emit_push(off_type, res_tmp_off_name, "=", res_to_off_name, "+", tmp_res_const_off_array_name,
                    "[", tmp_loop_var, "];");
        res_to_off_name = res_tmp_off_name;

        for (int_t j = 0; j < dim_to_set_set_cnt_posi[i]; j++) {
            datadesc_set cur_datadesc_set = dim_to_set_set_posi[i][j];
            for (int_t k = 0; k < cur_datadesc_set->n; k++) {
                char *tmp_off_name = token_new();
                char *tmp_valid_name = token_new();
                int_t cur_data_index = cur_datadesc_set->data_start_index + k;
                emit_push(off_type, tmp_off_name, "=", data_to_off_name[cur_data_index], "+", data_to_const_off_array_name[cur_data_index],
                             "[", tmp_loop_var, "];");
                data_to_off_name[cur_data_index] = tmp_off_name;
                emit_push(valid_type, tmp_valid_name, "=", data_to_valid_name[cur_data_index], "&", data_to_const_valid_array_name[cur_data_index],
                             "[", tmp_loop_var, "];");
                data_to_valid_name[cur_data_index] = tmp_valid_name;
            }
        }

    }

    if (ob->type == block_type_con_ser) {
        
        // FIXME: 递归生成代码，入口是[U]/[R]
        char *res_name = token_new();

        if (ob->p.b[0]->type > block_builtin_reduce_first && ob->p.b[0]->type < block_builtin_reduce_last)
        {
            reduce_code_gen(t, ob, res_name);
        } 
        else if (ob->p.b[0]->type > block_builtin_unary_first && ob->p.b[0]->type < block_builtin_unary_last) 
        {
            unary_code_gen(t, ob, res_name);
        }

        emit_push(res_to_off_base_name, "[", res_to_off_name, "] = ", res_name, ";");

    }

    for (int_t i = 1; i <= dim_max; i++) {
        int_t cur_common_indices_cnt = dim_to_common_indices_cnt_posi[i];
        if (cur_common_indices_cnt == 0) 
        {
            continue;
        }
        // 外层循环end
        emit_push("}");
    }

    emit_push("}");
    emit_finish();

    return res_set;
}