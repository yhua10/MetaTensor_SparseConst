#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "datadesc.h"
#include "gc_malloc.h"
#include "plugins.h"





// 原../src/common_data_with_desc.c 文件移到这里：


// #define DEBUG


#define block_threshold 3

//*********以下数据结构为memcpy中用到的修改后的数据结构*********//

//yuchen: 增加了exist，用于标记这个维度是否存在
struct dimdesc_memcpy_t
{
	enum_reduce_type	reduce_type;
	int_t	n_tuple;
	int_t	n_entry;
	int_t	*offsets;	// 这里的偏移量是以elem_size为单位的，与offset0不同。// FIXME: 需要仔细斟酌，这个设定是考虑到产生代码时不必来回换算，但在某些向量处理时是否有对齐问题？
    int_t exist; //用于标记这个维度是否存在
	int_t	*indices;
	// C99 6.7.2.1.18 : 柔性数组，下同
	int_t	dims[];	// 存储dimnames_g的下标
	// 随后的内存以int_t或int_t为单位，共有n_tuple+n_entry+n_tuple*n_entry个，排列顺序为：
	// int_t	dims[n_tuple]// 所有的维度， 变量映射的数字
	// int_t	offsets[n_entry] // 
	// int_t	indices[n_entry][n_tuple]
};
typedef	struct dimdesc_memcpy_t dimdesc_memcpy_t;
typedef	dimdesc_memcpy_t	* dimdesc_memcpy;

//yuchen: 增加了n_dim_capacity，用于标记这个datadesc_memcpy的dims数组的容量
struct datadesc_memcpy_t
{
	enum_region	region;
	union
	{
		void *p_direct;
		char *p_refname;
	} ref;
	int_t	offset0;	// 单位是字节。可以用这个调整对齐。

	enum_endian	endian;
	enum_elem_type	elem_type;
	int_t	elem_size;

	int_t	n_dim;
    int_t n_dim_capacity; //用于标记这个datadesc_memcpy的dims数组的容量

	dimdesc_memcpy	dims[];	// 数组可用长度为n_dim，但不必全占满，有稀疏情况时，后面冗余内存可以设置为NULL
};
typedef	struct datadesc_memcpy_t datadesc_memcpy_t;
typedef	datadesc_memcpy_t	* datadesc_memcpy;

struct datadesc_memcpy_set_t
{
	int_t		n;
	datadesc_memcpy	p[];	// 数组长度为n
};
typedef	struct datadesc_memcpy_set_t datadesc_memcpy_set_t;
typedef	datadesc_memcpy_set_t	* datadesc_memcpy_set;

dimdesc_memcpy dimdesc_memcpy_new(int_t n_tuple, int_t n_entry);
datadesc_memcpy datadesc_memcpy_new(int_t n_dim);

//**********适配memcpy部分的dimdesc new与datadesc new，之后若能与原有desc完全兼容，该部分可以删掉*********

dimdesc_memcpy dimdesc_memcpy_new(int_t n_tuple, int_t n_entry)
{
	dimdesc_memcpy p;
	p = malloc(sizeof(dimdesc_memcpy_t) + sizeof(int_t) * (n_tuple + n_entry + n_tuple * n_entry));
	p->n_tuple = n_tuple;
	p->n_entry = n_entry;
	p->offsets = p->dims + n_tuple;
	p->indices = p->offsets + n_entry;

	// NOTE: reduce_type和三个数组内容留给调用者初始化

	return p;
}

datadesc_memcpy datadesc_memcpy_new(int_t n_dim)
{
	datadesc_memcpy p;
	p = malloc(sizeof(datadesc_memcpy_t) + sizeof(dimdesc_memcpy_t *) * n_dim);
	p->n_dim = n_dim;

	// NOTE: 其他各项均由调用者负责初始化

	return p;
}

// 给出两个数组，判断这两个数组中的元素数量与元素值是否相等，但顺序可以不一致
int is_array_equal(int_t *array1, int_t *array2, int len)
{
    int i;
    int j;
    int temp;
    int *array1_copy = (int *)malloc(sizeof(int) * len);
    int *array2_copy = (int *)malloc(sizeof(int) * len);
    memcpy(array1_copy, array1, sizeof(int) * len);
    memcpy(array2_copy, array2, sizeof(int) * len);
    for (i = 0; i < len; i++)
    {
        for (j = i; j < len; j++)
        {
            if (array1_copy[i] > array1_copy[j])
            {
                temp = array1_copy[i];
                array1_copy[i] = array1_copy[j];
                array1_copy[j] = temp;
            }
            if (array2_copy[i] > array2_copy[j])
            {
                temp = array2_copy[i];
                array2_copy[i] = array2_copy[j];
                array2_copy[j] = temp;
            }
        }
    }
    for (i = 0; i < len; i++)
    {
        if (array1_copy[i] != array2_copy[i])
        {
            return 0;
        }
    }
    return 1;
}

// 给出四个数组，分别是dim1，dim2，indice1，indice2，对dim1、dim2中的元素从大到小进行排序，并且对indice1、indice2中的元素按照dim1、dim2中的元素的顺序进行排序，然后判断indice1、indice2中的元素是否相等
// 这里indices与dim的长度一致，dim表示的是维度的顺序，indices表示的是维度的下标，因此indices的长度与dim的长度一致。将indices按照dim的顺序排序之后，按照顺序对indices进行是否相等的比较。
// 在进行该过程前，实际上已经进行了dim是否相等的判断，不过这里依然进行一次判断，以防万一（也可删掉）
int is_indices_equal(int_t *dim1, int_t *dim2, int_t *indices1, int_t *indices2, int len)
{
    int i;
    int j;
    int temp;
    int *dim1_copy = (int *)malloc(sizeof(int) * len);
    int *dim2_copy = (int *)malloc(sizeof(int) * len);
    int *indices1_copy = (int *)malloc(sizeof(int) * len);
    int *indices2_copy = (int *)malloc(sizeof(int) * len);
    memcpy(dim1_copy, dim1, sizeof(int) * len);
    memcpy(dim2_copy, dim2, sizeof(int) * len);
    memcpy(indices1_copy, indices1, sizeof(int) * len);
    memcpy(indices2_copy, indices2, sizeof(int) * len);
    for (i = 0; i < len; i++)
    {
        for (j = i; j < len; j++)
        {
            if (dim1_copy[i] > dim1_copy[j])
            {
                temp = dim1_copy[i];
                dim1_copy[i] = dim1_copy[j];
                dim1_copy[j] = temp;
                // 交换indices1_copy中的元素
                temp = indices1_copy[i];
                indices1_copy[i] = indices1_copy[j];
                indices1_copy[j] = temp;
            }
            if (dim2_copy[i] > dim2_copy[j])
            {
                temp = dim2_copy[i];
                dim2_copy[i] = dim2_copy[j];
                dim2_copy[j] = temp;
                // 交换indices2_copy中的元素
                temp = indices2_copy[i];
                indices2_copy[i] = indices2_copy[j];
                indices2_copy[j] = temp;
            }
        }
    }
    for (i = 0; i < len; i++)
    {
        if (dim1_copy[i] != dim2_copy[i])
        {
            return 0;
        }
        if (indices1_copy[i] != indices2_copy[i])
        {
            return 0;
        }
    }
    return 1;
}

// 当用dim表示一个单独的数据时，判断两个数据在该中表现上是否可以被判断为“相当等的数据”。相等的概念为：数据所对应的维度顺序可以不一致，但对应维度的下标必须一致。
int is_dim_equal(dimdesc_memcpy dim1, dimdesc_memcpy dim2)
{
    if (dim1->reduce_type != dim2->reduce_type)
    {
        return 0;
    }
    if (dim1->n_tuple != dim2->n_tuple) // 维度和正常的维度是一样的
    {
        return 0;
    }
    if (dim1->n_entry != dim2->n_entry) // 正常来说里面只有一个元素，所以正常来说应该是1，不过这里依然判断一下是否相等
    {
        return 0;
    }

    //***************offset在这里没有作用，因此不做判断***********

    if ((dim1->exist != dim2->exist) || (dim1->exist != 1 || dim2->exist != 1)) // 如果两个都不存在，则判定不相等
    {
        return 0;
    }

    // is_array_equal函数返回1时，表示相等；返回0，表示不相等

    if (is_array_equal(dim1->dims, dim2->dims, dim1->n_tuple) == 0) // 判断两个维度的下标是否相等
    {
        return 0;
    }

    if (is_indices_equal(dim1->dims, dim2->dims, dim1->indices, dim2->indices, dim1->n_tuple) == 0) // 判断indices按照dim排序之后，indices中储存的下标是否相等
    {
        return 0;
    }
    return 1;
}

// initialize the datadesc_memcpy
datadesc_memcpy data_array_init(int capacity)
{
    datadesc_memcpy p;
    p = malloc(sizeof(datadesc_memcpy_t) + sizeof(dimdesc_memcpy_t *) * capacity);
    p->n_dim = 0;
    p->n_dim_capacity = capacity;
    return p;
}

// replace the value of the element of the data_array_t in the index position with the data_struct_t data. if the index is out of the range of the data_array_t, the data_array_t will be extended.
int data_array_replace(datadesc_memcpy *data_array, int index, dimdesc_memcpy data)
{
    // datadesc_memcpy is a pointer, so we need to use *data_array to get the real datadesc_memcpy
    int i = 0;
    if (index >= (*data_array)->n_dim_capacity)
    {
        int old_capacity=(*data_array)->n_dim_capacity;
        datadesc_memcpy new_data_array;
        new_data_array = (datadesc_memcpy)realloc((*data_array), sizeof(datadesc_memcpy_t) + sizeof(dimdesc_memcpy_t *) * (index * 2 + 1));
        new_data_array->n_dim_capacity = index * 2 + 1;
        // 假设每一次对总容量进行扩增时，原有capacity内的空间都已经过初始化，因此需要对新扩增的dim进行初始化，保证后续扩增空间时的正确性。
        for (i =old_capacity; i < new_data_array->n_dim_capacity; i++)
        {
            new_data_array->dims[i] = dimdesc_memcpy_new(0, 0);
            new_data_array->dims[i]->exist = 0;
        }

        (*data_array) = new_data_array;
    }
    if (index >= (*data_array)->n_dim)
    {
        (*data_array)->n_dim = index + 1;
    }
    (*data_array)->dims[index] = data;
    (*data_array)->dims[index]->exist = 1;
    return 0;
}

int print_dim(dimdesc_memcpy dim)
{
    int i, j;
    printf("n_tuple:%ld\n", dim->n_tuple);
    printf("n_entry:%ld\n", dim->n_entry);
    printf("exist:%ld\n", dim->exist);
    printf("dims:");
    for (i = 0; i < dim->n_tuple; i++)
    {
        printf("%ld ", dim->dims[i]);
    }
    printf("\n");
    printf("indices:");
    for (i = 0; i < dim->n_entry; i++)
    {
        for (j = 0; j < dim->n_tuple; j++)
        {
            printf("%ld ", dim->indices[i + j * (dim->n_entry - 1)]);
        }
    }
    printf("\n");
    printf("offsets:");
    for (i = 0; i < dim->n_entry; i++)
    {
        printf("%ld ", dim->offsets[i]);
    }

    printf("\n");
    return 0;
}

// given two data_array_t, find all common continuous subarray of the two data_array_t, save the start and end index of all common continuous subarray in the two data_array_t, and save all the start and end index in four integer array, start_index1[], end_index1[],start_index2[],end_index2[] that are given by uesr.  if there is no common continuous subarray, return -1, else return 0.if the element dim of the data_struct_t is less than 0, the data_struct_t is not in the data_array_t.
int data_array_common_subarray(datadesc_memcpy data_array1, datadesc_memcpy data_array2, int start_index1[], int end_index1[], int start_index2[], int end_index2[], int *mem_cpy_len, int single_index1[], int single_index2[], int *single_cpy_len)
{

    int i, j;
    int start1 = 0, end1 = 0, start2 = 0, end2 = 0;
    int index = 0;         // the current index of the start_index1[], end_index1[], start_index2[], end_index2[]
    int flag = 0;          // flag=0 means not find the first elements in array1 and 2 are equal, flag=1 means find the first elements in array1 and 2 are equal
    int unfinish_flag = 0; // unfinish_flag=0 means the last block is finished, unfinish_flag=1 means the last block is not finished
    *mem_cpy_len = 0;
    *single_cpy_len = 0;
    for (i = 0; i < data_array1->n_dim; i++)
    {

        for (j = 0; j < data_array2->n_dim; j++)
        {



            // dim <0 or the elements in array1 and 2 are not equal, skip
            if (flag == 0) // flag=0 means does not find the first elements in array1 and 2
            {
                if (is_dim_equal(data_array1->dims[i], data_array2->dims[j]) == 0) // element 【DOESN'T】 exits or not equal, move j
                {
                    continue;
                }
                // find the first elements in array1 and 2 are equal, save the start index, set flag=1, move i
                else //
                {
                    start1 = i;
                    start2 = j;
                    end1 = i;
                    end2 = j;
                    flag = 1;
                    i = i + 1; // move i
                    if (i >= data_array1->n_dim)
                    {
                        return 0;
                    }
                }
            }
            else
            { // flag==1, start finding block
                if (is_dim_equal(data_array1->dims[i], data_array2->dims[j]) == 0) // dim doesn't equal
                {
                    end1 = i;
                    end2 = j;

                    flag = 0;
                    unfinish_flag = 0; // the last block is finished
                    if ((end1 - start1) >= block_threshold)
                    {
                        start_index1[index] = start1;
                        end_index1[index] = end1;
                        start_index2[index] = start2;
                        end_index2[index] = end2;
                        (*mem_cpy_len)++;
                        index++;
                    }
                    else
                    {
                        int k = 0;
                        for (k = 0; k < end1 - start1; k++)
                        {
                            single_index1[*single_cpy_len] = start1 + k;
                            single_index2[*single_cpy_len] = start2 + k;
                            (*single_cpy_len)++;
                        }
                    }
                    if (i >= data_array1->n_dim)
                    {
                        return 0;
                    }
                }
                else
                { // elements are equal
                    end1 = i;
                    end2 = j;
                    unfinish_flag = 1;
                    i++;
                }
            }

            // if find the first elements in array1 and 2 are equal, save the start index
        }
    }

    //deal with the rest data between start and end
    if (unfinish_flag == 1)
    {
        if ((end1 - start1) >= block_threshold)
        {
            start_index1[index] = start1;
            end_index1[index] = end1;
            start_index2[index] = start2;
            end_index2[index] = end2;
            (*mem_cpy_len)++;
            index++;
            // 打印出当前的block，输出行号和文件名（__LINE, __FILE
        }
        else
        {
            int k = 0;
            for (k = 0; k <= end1 - start1; k++)
            {
                single_index1[*single_cpy_len] = start1 + k;
                single_index2[*single_cpy_len] = start2 + k;
                (*single_cpy_len)++;
            }
        }
    }


    return 0;
}


int data_array_common_subarray_len(datadesc_memcpy data_array1, datadesc_memcpy data_array2, int *mem_cpy_len, int *single_cpy_len)
{

    int i, j;
    int start1 = 0, end1 = 0, start2 = 0, end2 = 0;
    int index = 0;         // the current index of the start_index1[], end_index1[], start_index2[], end_index2[]
    int flag = 0;          // flag=0 means not find the first elements in array1 and 2 are equal, flag=1 means find the first elements in array1 and 2 are equal
    int unfinish_flag = 0; // unfinish_flag=0 means the last block is finished, unfinish_flag=1 means the last block is not finished
    *mem_cpy_len = 0;
    *single_cpy_len = 0;
    for (i = 0; i < data_array1->n_dim; i++)
    {

        for (j = 0; j < data_array2->n_dim; j++)
        {



            // dim <0 or the elements in array1 and 2 are not equal, skip
            if (flag == 0) // flag=0 means does not find the first elements in array1 and 2
            {
                if (is_dim_equal(data_array1->dims[i], data_array2->dims[j]) == 0) // element 【DOESN'T】 exits or not equal, move j
                {
                    continue;
                }
                // find the first elements in array1 and 2 are equal, save the start index, set flag=1, move i
                else //
                {
                    start1 = i;
                    start2 = j;
                    end1 = i;
                    end2 = j;
                    flag = 1;
                    i = i + 1; // move i
                    if (i >= data_array1->n_dim)
                    {
                        return 0;
                    }
                }
            }
            else
            { // flag==1, start finding block
                if (is_dim_equal(data_array1->dims[i], data_array2->dims[j]) == 0) // dim doesn't equal
                {
                    end1 = i;
                    end2 = j;

                    flag = 0;
                    unfinish_flag = 0; // the last block is finished
                    if ((end1 - start1) >= block_threshold)
                    {
                        (*mem_cpy_len)++;
                        index++;
                    }
                    else
                    {
                        int k = 0;
                        for (k = 0; k < end1 - start1; k++)
                        {
                            (*single_cpy_len)++;
                        }
                    }
                    if (i >= data_array1->n_dim)
                    {
                        return 0;
                    }
                }
                else
                { // elements are equal
                    end1 = i;
                    end2 = j;
                    unfinish_flag = 1;
                    i++;
                }
            }

            // if find the first elements in array1 and 2 are equal, save the start index
        }
    }

    //deal with the rest data between start and end
    if (unfinish_flag == 1)
    {
        if ((end1 - start1) >= block_threshold)
        {
            (*mem_cpy_len)++;
            index++;
            // 打印出当前的block，输出行号和文件名（__LINE, __FILE
        }
        else
        {
            int k = 0;
            for (k = 0; k <= end1 - start1; k++)
            {
                (*single_cpy_len)++;
            }
        }
    }


    return 0;
}

int print_data(datadesc_memcpy data)
{
    int i;
    printf("n_dim:%ld\n", data->n_dim);
    printf("n_dim_capacity:%ld\n", data->n_dim_capacity);
    printf("dims:\n");
    for (i = 0; i < data->n_dim; i++)
    {
        printf("========dim %d:============\n", i);
        print_dim(data->dims[i]);
    }
    return 0;
}

// 这里的dim是正常的用于表示维度的dim，而data是用来储存所有单独数据的datadesc_memcpy，而并非是和定义用法一致的datadesc_memcpy，其中存储的每一个dim均表示的是一个数据，而非一个真正的dim。即：该函数参数中的dim并不会直接放到参数中的data里，而是会将里面的每一个元素，按照offset的方式，映射到data里的dims里面。
int map_dim_into_data(datadesc_memcpy *data, dimdesc dim)
{
    int i, j, k;
    int index;
    for (i = 0; i < dim->n_entry; i++)
    {
        index = dim->offsets[i]; // dim在data中映射的位置
        dimdesc_memcpy p;
        p = dimdesc_memcpy_new(dim->n_tuple, 1);
        p->exist = 1;
        p->reduce_type = dim->reduce_type;
        // 给即将放入data的dim中的indices赋值
        for (int j = 0; j < dim->n_tuple; j++)
        {
            p->indices[j] = dim->indices[i * dim->n_tuple + j];
        }
        data_array_replace(data, index, p);
    }
    return 0;
}




//原../include/token_structure.h文件移到这里：


//create a dynamic link with a static head
typedef struct token_structure
{
    char* name;
    int count;
    struct token_structure	* next;
}	token_structure_t;

typedef token_structure_t * token_structure;

//使用该函数替换target_base.h里的token_new函数，以提升生成代码的可读性，降低调bug以及根据生成代码对后续代码进行优化的难度
//char* named_token_new(char* token_name);



/*
map the last dim into datasesc
find start_index, end_index in datadesc
*/

// 原token_structure.c文件移到这里：



// 判断两个char*指向的字符串是否相等，相等则返回0， 不相等返回-1
int cmpstr(char *str1, char *str2)
{
    int i = 0;
    // 求出两个字符串的长度
    int len1 = 0;
    int len2 = 0;
    while (str1[len1] != '\0')
    {
        len1++;
    }
    while (str2[len2] != '\0')
    {
        len2++;
    }

    // 如果两个字符串长度不相等，则直接返回-1
    if (len1 != len2)
    {
        return -1;
    }
    while (str1[i] != '\0' && str2[i] != '\0')
    {
        if (str1[i] != str2[i])
        {
            return -1;
        }
        i++;
    }
    if (str1[i] == '\0' && str2[i] == '\0')
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

// 输入一个token的名字，返回由该名字与index组成的字符串，如 definition_13、loop_21，用于区分生成的代码中不同变量的作用
char *named_token_new(char *token_name)
{
    // 采用静态链表，token_index为链表的头指针
    static token_structure token_index = NULL;
    if (token_index == NULL)
    {

        token_index = malloc(sizeof(token_structure_t));
        token_index->count = 0;
        token_index->next = NULL;
        token_index->name = malloc(sizeof(char) * 100);
        // 给token_index_name赋值为var,使用snprintf()函数，第一个参数为指向目标字符串的指针，第二个参数为目标字符串的最大长度，第三个参数为格式化字符串，第四个参数为可变参数
        snprintf(token_index->name, 100, "%s", "var");

    }

    token_structure tmp_token_index = token_index;
    while (1)
    {
        // token_name与已有的token_name匹配，匹配则count加1，返回由token_name与count组成的字符串
        if (cmpstr(tmp_token_index->name, token_name) == 0)
        {
            tmp_token_index->count++;
            // 返回由token_name与count组成的字符串
            char *token_result = malloc(sizeof(char) * 120);
            snprintf(token_result, 100, "%s_%d", token_name, tmp_token_index->count);

            return token_result;
        }
        // 如果tmp_token_index->next为空，则标明当前链表中不存在传入的token_name。新建一个token_index，返回由token_name与count组成的字符串
        if (tmp_token_index->next == NULL)
        {
            token_structure new_token_index = malloc(sizeof(token_structure_t));
            new_token_index->count = 1; // 新建的token_index的count为1，因为新建之后便已经开始计数投入使用
            new_token_index->next = NULL;
            new_token_index->name = malloc(sizeof(char) * 100);
            snprintf(new_token_index->name, 100, "%s", token_name);
            char *token_result = malloc(sizeof(char) * 120);
            snprintf(token_result, 100, "%s_%d", token_name, new_token_index->count);
            tmp_token_index->next = new_token_index;
            return token_result;
        }
        tmp_token_index = tmp_token_index->next;
    }
}


// 原修改后的datacopy_121_reference.c文件在这里：


// #define DEBUG


/*
 * 目前版本实现了在主内存上的：
 * 	拷贝、转置、类型转换、求和归约、广播，以及这些操作的组合
 * 	输出代码格式为C语言的语句块
 * 目前版本尚未实现的功能包括：
 * 	跨主内区域的操作：不在此实现
 * 	异步支持：不在此实现，但可以ffork到另外一个实现
 *	endian转换：可以增补
 *	稀疏情况：可以增补——优先实现
 * 目前版本的优化空间：
 * 	非一对一的拷贝：不在此实现
 * 	大块内存合并拷贝：不在此实现
 * 	并行等与硬件相关的深度优化：不在此实现
 * 	偏移量数组转换成简单运算：可以增补
 * 	除求和外的其他归约：可以增补
 */

int_t datacopy_121_memcpy(target t, datadesc_set dst, datadesc_set src)
{
	datadesc_set d;
	int_t n_reduce = 0;
	int_t i_dst, i_src;
	int_t i, j;



	// 使用memcpy进行复制的piece数量
	int mem_cpy_len; // index1: src , index2: dst

	// 使用点对点方式进行赋值的数据的数量
	int  single_cpy_len;

	emit_start(t);
	emit_assert(t->target_type == target_type_c_macro);
	emit_assert(dst->n == 1 && src->n == 1);
	emit_assert(dst->p[0]->region == region_main); // FIXME: 这里其实可以不必限制得如此保守，其实只要可以直接访问的内存区域，代码应该是通用的。
	emit_assert(src->p[0]->region == region_main);
	emit_assert(dst->p[0]->endian == src->p[0]->endian && src->p[0]->endian == target_query_endian(t)); // TODO: 这里需要增补实现

	d = datadesc_set_new(2);
	d->p[0] = dst->p[0];
	d->p[1] = src->p[0];
	d = datadesc_find_common(d);

	d->p[0] = datadesc_normalize(d->p[0]); // NOTE: 在目前的实现中，指针并没有改变，是在原有的内存上做的normalize
	d->p[1] = datadesc_normalize(d->p[1]);
	//*********************start 生成common piece**************
	// 生成common piece的起始位置数组，并对n_dim_capacity进行初始化
	datadesc_memcpy datadesc_sort_src = datadesc_memcpy_new(0);
	datadesc_sort_src->n_dim_capacity = 0;
	datadesc_sort_src->n_dim = 0;
	datadesc_memcpy datadesc_sort_dst = datadesc_memcpy_new(0);
	datadesc_sort_dst->n_dim_capacity = 0;
	datadesc_sort_dst->n_dim = 0;
	map_dim_into_data(&datadesc_sort_src, d->p[1]->dims[d->p[1]->n_dim - 1]);
	map_dim_into_data(&datadesc_sort_dst, d->p[0]->dims[d->p[0]->n_dim - 1]);

//get the number of pieces and single elements

int num_of_piece=0, num_of_single_ele=0;

data_array_common_subarray_len(datadesc_sort_dst, datadesc_sort_src, &num_of_piece, &num_of_single_ele);

//init dynamic array to store the index of start_index1,2, end_index1,2 and single_elements
int *start_index1 = (int *)malloc(sizeof(int) * num_of_piece);
int *start_index2 = (int *)malloc(sizeof(int) * num_of_piece);
int *end_index1 = (int *)malloc(sizeof(int) * num_of_piece);

int *end_index2 = (int *)malloc(sizeof(int) * num_of_piece);
int* single_index1 = (int *)malloc(sizeof(int) * num_of_single_ele);
int* single_index2 = (int *)malloc(sizeof(int) * num_of_single_ele);




	data_array_common_subarray(datadesc_sort_dst, datadesc_sort_src, start_index1, end_index1, start_index2, end_index2, &mem_cpy_len, single_index1, single_index2, &single_cpy_len);
	//**********************end 生成common piece****************

	char *int_type = target_query_type(t, elem_type_integer, 4); // NOTE: 循环变量和偏移量的类型，目前默认是 32bit signed integer
	char *dst_type = target_query_type(t, d->p[0]->elem_type, d->p[0]->elem_size);
	char *src_type = target_query_type(t, d->p[1]->elem_type, d->p[1]->elem_size);
	emit_assert(int_type && dst_type && src_type);

	emit_push("{");

	char *t_dst_p = named_token_new("root_dst_var"); // 目标位置的首地址
	char *t_src_p = named_token_new("root_src_var"); // 源位置的首地址
	// t_dst_i, t_src_i所保存的变量名在代码中对应的变量，在每一层循环中，都是对应的该层循环的指标的偏移量。通过t_tmp=t_src_i+src_offset_p[t_offset_i]计算得到下一层循环的偏移量, t_src_i = t_tmp	将该偏移量保存到下一层的t_src_i中
	char *t_dst_i = named_token_new("accumulated_offset_dst"); // 循环中的目标位置下标，运行时使用若干变量名，通过t_dst_i = t_tmp赋新值
	char *t_src_i = named_token_new("accumulated_offset_src"); // 循环中的源位置下标，运行时使用若干变量名，该值在每个循环中通过t_src_i = t_tmp赋新值
	emit_push("\n//part 1\n");

	emit_push(dst_type, "*", t_dst_p, "=(", dst_type, "*)((char *)(", d->p[0]->ref.p_refname, ")+", mt_itoa(d->p[0]->offset0), ");");
	emit_push(src_type, "*", t_src_p, "=(", src_type, "*)((char *)(", d->p[1]->ref.p_refname, ")+", mt_itoa(d->p[1]->offset0), ");");
	emit_push(int_type, " ", t_dst_i, "=0;");
	emit_push(int_type, " ", t_src_i, "=0;");
	emit_push("\n//part 2\n");
	// 这里以dst为循环的基准，但会同时对src和dst的高维度进行展开
	// 展开的最后一个维度是在复制piece的时候，被piece替代的维度，因此最后一层展开做特殊处理，在此之前的计算方式不变。
	for (i_dst = 0; i_dst < d->p[0]->n_dim; i_dst++) // NOTE: 对于每个目标维度，如果有对应源维度，则是正常拷贝，否则是广播。
	{
		// 如果循环到了最后一个维度，
		if (i_dst == d->p[0]->n_dim - 1)
		{
			if (mem_cpy_len > 0)
			{
				// 生成piece的起始位置数组
				dimdesc d_dst = d->p[0]->dims[i_dst];
				emit_assert(d_dst->n_tuple == 1); // TODO: 这里先实现稠密的情况，稀疏的需要增补
				emit_assert(d_dst->n_entry > 0);  // FIXME: 如果无数据可传输，是否应当回退并返回1？此处设计需要斟酌

				char *t_src_offset_piece_p = named_token_new("piece_src_start"); // 源piece的start数组
				char *t_dst_offset_piece_p = named_token_new("piece_dst_start"); // 目标piece的start数组

				emit_push(int_type, " ", t_src_offset_piece_p, "[", mt_itoa(mem_cpy_len), "]={", mt_itoa(start_index1[0]));
				for (i = 1; i < mem_cpy_len; i++)
					emit_push(",", mt_itoa(start_index1[i]));
				emit_push("};"); // 该段代码之后得到dst的偏移数组

				emit_push(int_type, " ", t_dst_offset_piece_p, "[", mt_itoa(mem_cpy_len), "]={", mt_itoa(start_index2[0]));
				for (i = 1; i < mem_cpy_len; i++)
					emit_push(",", mt_itoa(start_index2[i]));
				emit_push("};"); // 该段代码之后得到dst的偏移数组

				// 生成piece的长度数组
				char *t_piece_len_p = named_token_new("piece_length"); // piece的长度数组
				emit_push(int_type, " ", t_piece_len_p, "[", mt_itoa(mem_cpy_len), "]={", mt_itoa(end_index1[0] - start_index1[0]));
				for (i = 1; i < mem_cpy_len; i++)
					emit_push(",", mt_itoa(end_index1[i] - start_index1[i]));
				emit_push("};"); // 该段代码之后得到dst的偏移数组

				// 生成memcpy语句 ！！！！！！！！！该部分代码需要根据不同的平台进行修改！！！！！！
				char *t_offset_i = named_token_new("loop"); // 每个维度对指标的循环变量
				emit_push(int_type, " ", t_offset_i, ";");
				// 应该从最后一层for循环开始改变复制过程
				emit_push("for(", t_offset_i, "=0;", t_offset_i, "<", mt_itoa(mem_cpy_len), ";", t_offset_i, "++){");

				// 生成memcpy(t_dst_p[offset],t_src_p[offset],len)

				emit_push("memcpy(&", t_dst_p, "[", t_dst_i, "+", t_dst_offset_piece_p, "[", t_offset_i, "]],&", t_src_p, "[", t_src_i, "+", t_src_offset_piece_p, "[", t_offset_i, "]],", t_piece_len_p, "[", t_offset_i, "]*sizeof(", dst_type, "));");

				emit_push("}"); // 结束生成memcpy代码的for循环
			}
			if (single_cpy_len > 0)
			{
				// char *t_tmp = token_new(); //这一行似乎没有用，先注释掉

				// 生成点对点复制的变量的偏移量数组
				char *t_src_offset_single_p = named_token_new("single_src_start");
				char *t_dst_offset_single_p = named_token_new("single_dst_start");

				emit_push(int_type, " ", t_src_offset_single_p, "[", mt_itoa(single_cpy_len), "]={", mt_itoa(single_index1[0]));
				for (i = 1; i < single_cpy_len; i++)
					emit_push(",", mt_itoa(single_index1[i]));
				emit_push("};"); // 该段代码之后得到点对点复制的src的偏移数组

				emit_push(int_type, " ", t_dst_offset_single_p, "[", mt_itoa(single_cpy_len), "]={", mt_itoa(single_index2[0]));
				for (i = 1; i < single_cpy_len; i++)
					emit_push(",", mt_itoa(single_index2[i]));
				emit_push("};"); // 该段代码之后得到点对点复制的dst的偏移数组

				// 生成single var copy语句
				char *t_offset_single_i = named_token_new("loop"); // 每个维度对指标的循环变量
				emit_push(int_type, " ", t_offset_single_i, ";");
				emit_push("for(", t_offset_single_i, "=0;", t_offset_single_i, "<", mt_itoa(single_cpy_len), ";", t_offset_single_i, "++){");

				emit_push(t_dst_p, "[", t_dst_i, "+", t_dst_offset_single_p, "[", t_offset_single_i, "]]=", t_src_p, "[", t_src_i, "+", t_src_offset_single_p, "[", t_offset_single_i, "]];");

				emit_push("}");
				// 复制single的元素。single的元素是指，除了通过piece进行复制的元素之外，只通过点对点进行复制的元素。该段代码依然位于最后一层循环内，因为single的元素也是在最后一层循环内进行复制的。
				t_dst_offset_single_p = named_token_new("single_copy_list_dst"); // 除了通过piece，memcpy进行复制的元素之外，只通过点对点进行复制的元素的偏移量列表
				t_src_offset_single_p = named_token_new("single_copy_list_src");

				emit_push(int_type, " ", t_dst_offset_single_p, "[", mt_itoa(single_cpy_len), "]={", mt_itoa(single_index1[0]));
				for (i = 1; i < single_cpy_len; i++)
					emit_push(",", mt_itoa(single_index1[i]));
				emit_push("};"); // 该段代码之后得到除piece元素之外的点对点复制的数组

				// 通过t_src_i, t_dst_i以及piece对应的start/end index, single对应的start/end index, 以及piece/single对应的offset数组，可以计算出每个元素的偏移量，从而得到每个元素的地址，从而得到每个元素的值。
			}
			continue; // 执行完if程序块之后，便不能再执行后续代码，因此直接跳出该段for循环。不用break是因为continue之后，for循环依然可以中止，因此正常情况下效果等同于continue。用continue或许可以发现一些潜在的bug。
		}

		//********************非最后一维时执行的代码**********************
		dimdesc d_dst = d->p[0]->dims[i_dst];

		emit_assert(d_dst->n_tuple == 1); // TODO: 这里先实现稠密的情况，稀疏的需要增补
		emit_assert(d_dst->n_entry > 0);  // FIXME: 如果无数据可传输，是否应当回退并返回1？此处设计需要斟酌

		char *t_dst_offset_p = named_token_new("t_dst_offset_p"); // 每个维度的各指标的偏移量列表
		char *t_src_offset_p = named_token_new("t_src_offset_p");
		emit_push("\n//part 3\n");
		// TODO: 这里的偏移量列表有可优化的潜力，比如等差数列等可以直接计算而不查表。
		emit_push(int_type, " ", t_dst_offset_p, "[", mt_itoa(d_dst->n_entry), "]={", mt_itoa(d_dst->offsets[0]));
		for (i = 1; i < d_dst->n_entry; i++)
			emit_push(",", mt_itoa(d_dst->offsets[i]));
		emit_push("};"); // 该段代码之后得到dst的偏移数组
		emit_push("\n//part 4\n");
		// FIXME: 这里没有完整考虑稀疏的情况，需要额外处理
		// 该for循环找的是d_dst与p[1]->dims，即src的dims中相同的dim。找到了则循环在d_dst->dims[0]!=d->p[1]->dims[i_src]->dims[0]条件下退出，若找不到，则循环在i_src<d->p[1]->n_dim条件下退出，此时，i_src==d->[1]->n_dim，下面的if循环不执行。即此时不需要对该维度进行复制（因为没有相同的维度）
		// 当这里不需要生成src的偏移数组时，上面的dst偏移数组是否多余？——不多余，需要即使是空数组也要生成，因为后面的代码需要用到该数组的长度。
		// 能否保证每一个src的长度都被算出来？ ——可以保证，src中存在，但是dst中不存在的维度，则进行归约；src中不存在，dst中存在的维度，src对应的位置固定（因为不存在该维度），对dst中所有位置均进行
		for (i_src = 0; i_src < d->p[1]->n_dim && d_dst->dims[0] != d->p[1]->dims[i_src]->dims[0]; i_src++)
			;
		if (i_src < d->p[1]->n_dim)
		{
			emit_push("\n//part 4.1\n");
			dimdesc d_src = d->p[1]->dims[i_src];
			emit_assert(d_src->n_tuple == 1);
			emit_assert(d_dst->n_entry == d_src->n_entry);
			emit_push(int_type, " ", t_src_offset_p, "[", mt_itoa(d_src->n_entry), "]={");
			for (i = 0; i < d_dst->n_entry; i++)
			{
				for (j = 0; j < d_src->n_entry && d_dst->indices[i * d_dst->n_tuple] != d_src->indices[j * d_src->n_tuple]; j++)
					;
				emit_assert(j < d_src->n_entry);
				if (i > 0)
					emit_push(",");
				emit_push(mt_itoa(d_src->offsets[j]));
			}
			emit_push("};");
		}

		emit_push("\n//part 5\n");
		char *t_offset_i = named_token_new("loop"); // 每个维度对指标的循环变量

		emit_push(int_type, " ", t_offset_i, ";");
		emit_push("for(", t_offset_i, "=0;", t_offset_i, "<", mt_itoa(d_dst->n_entry), ";", t_offset_i, "++){");

		char *t_tmp = named_token_new("accumulated_offset_dst");
		emit_push(int_type, " ", t_tmp, "=", t_dst_i, "+", t_dst_offset_p, "[", t_offset_i, "];");
		t_dst_i = t_tmp;

		// 为什么这里还要判断src的维度？
		if (i_src < d->p[1]->n_dim)
		{
			t_tmp = named_token_new("accumulated_offset_src");
			emit_push(int_type, " ", t_tmp, "=", t_src_i, "+", t_src_offset_p, "[", t_offset_i, "];");
			t_src_i = t_tmp;
		}
	}


	for (i_dst = 0; i_dst < d->p[0]->n_dim - 1; i_dst++) // 这里循环的维度改为n_dim-1是因为最后一个维度为single copy与clock copy，循环的右括号再之前已经输出出去了，不需要在这里继续输出
	{
		emit_push("}");
	}

	emit_push("}");
	emit_finish();
	return 1;
}



