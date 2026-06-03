
#include	<string.h>

#include	"datadesc.h"
#include	"gc_malloc.h"


static int_t	n_dims_g = 0;
static int_t	n_dims_tmp = -1;
static char **	dimnames_g = NULL;

// 单个entry在各维度(tuple)的indice组合构成indice_tuple
struct indice_tuple {
	int_t n_tuple;
	int_t offset;
	int_t indices[];
};
typedef struct indice_tuple indice_tuple;

// indice_tuple相等，则返回0
int_t compare_indice_tuple(indice_tuple *a, indice_tuple *b)
{
	if (a->n_tuple != b->n_tuple)
	{
		return -1;
	}
	int_t i;
	for (i=0; i<a->n_tuple; i++)
	{
		if (a->indices[i] != b->indices[i])
		{
			return -1;
		}
	}
	return 0;
}

// 若找到indice_tuple，则返回其在set中的索引；否则返回-1
int_t find_indice_tuple(indice_tuple *indice_tuple_set[], int_t indice_tuple_cnt, indice_tuple *a)
{
	int_t i;
	for (i=0; i<indice_tuple_cnt; i++)
	{
		if (compare_indice_tuple(indice_tuple_set[i], a) == 0)
		{
			return i;
		}
	}
	return -1;
}

indice_tuple *copy_indice_tuple(indice_tuple *base_indice_tuple)
{
	int_t n_tuple;
	n_tuple = base_indice_tuple->n_tuple;
	indice_tuple *new_indice_tuple = (indice_tuple *)malloc(sizeof(indice_tuple)+sizeof(int_t)*(n_tuple));
	new_indice_tuple->n_tuple = n_tuple;
	new_indice_tuple->offset = base_indice_tuple->offset;
	memcpy(new_indice_tuple->indices, base_indice_tuple->indices, sizeof(int_t)*n_tuple);
	return new_indice_tuple;
}

/*
	返回datadesc->dims中第一个NULL的位置；如果没有NULL，则返回datadesc->n_dim
*/
int_t query_dims_cnt(datadesc p) {
	int_t i;
	for (i = 0; i < p->n_dim && p->dims[i]; i++);
	return i;
}

int_t	dimname_new(const char *p)
{
	static	int_t	n_dims_max = 128;
	int_t	i;

	if (dimnames_g == NULL)
		dimnames_g = malloc(sizeof(char *)*n_dims_max);
	if (n_dims_g == n_dims_max)
	{
		n_dims_max *= 2;
		char **	p_tmp = malloc(sizeof(char *)*n_dims_max);
		memcpy(p_tmp, dimnames_g, sizeof(char *)*n_dims_g);
		dimnames_g = p_tmp;
	}

	for (i=0; i<n_dims_g && strcmp(dimnames_g[i], p); i++);
	if (i<n_dims_g)
		return	i;

	dimnames_g[n_dims_g] = malloc(strlen(p)+1);
	strcpy(dimnames_g[n_dims_g], p);

	return	n_dims_g ++;
}

const char *	dimname(int_t index)
{
	return	dimnames_g[index];
}

int_t	dimtmp_new()
{
	return	n_dims_tmp--;
}

dimdesc	dimdesc_new(int_t n_tuple, int_t n_entry)
{
	dimdesc	p;
	p = malloc(sizeof(dimdesc_t)+sizeof(int_t)*(n_tuple+n_entry+n_tuple*n_entry));
	p->n_tuple = n_tuple;
	p->n_entry = n_entry;
	p->offsets = p->dims + n_tuple;
	p->indices = p->offsets + n_entry;
 
	// NOTE: reduce_type和三个数组内容留给调用者初始化

	return p;
}

datadesc	datadesc_new(int_t n_dim)
{
	datadesc	p;
	p = malloc(sizeof(datadesc_t)+sizeof(dimdesc_t*)*n_dim);
	p->n_dim = n_dim;

	// NOTE: 其他各项均由调用者负责初始化

	return p;
}

datadesc	datadesc_new_array(enum_region region, void *p_ref, enum_endian endian, enum_elem_type elem_type, int_t elem_size, int_t n_dim, char **p_dimname, int_t *p_dimlength)
{
	datadesc	p;
	int_t		i;		// NOTE: 有倒着循环时循环变量必须是有符号的
	int_t		j, step;

	p = datadesc_new(n_dim);	// FIXME: 可以把dims也一起打包申请内存，效率更高。如果需要的话，留待后续改进
	p->region = region;
	p->ref.p_direct = p_ref;
	p->offset0 = 0;
	p->endian = endian;
	p->elem_type = elem_type;
	p->elem_size = elem_size;

	step = 1;
	for (i=n_dim-1; i>=0; i--)
	{
		p->dims[i] = dimdesc_new(1, p_dimlength[i]);
		p->dims[i]->reduce_type = reduce_type_sum;	// FIXME: 默认为sum是否合理？
		p->dims[i]->dims[0] = dimname_new(p_dimname[i]);
		for (j=0; j<p->dims[i]->n_entry; j++)
		{
			p->dims[i]->indices[j] = j;		// NOTE: 这里实际是二维数组
			p->dims[i]->offsets[j] = j * step;
		}
		step *= p_dimlength[i];
	}

	return	p;
}

datadesc_set	datadesc_set_new(int_t n)
{
	datadesc_set	p;
	p = malloc(sizeof(datadesc_set_t)+sizeof(datadesc)*n);
	p->n = n;

	// NOTE: 指针数组各项均由调用者负责初始化

	return p;
}

// 创建并返回一个dim的拷贝
dimdesc		dimdesc_copy(dimdesc dim) {
	dimdesc res;
	res = dimdesc_new(dim->n_tuple, dim->n_entry);
	res->reduce_type = dim->reduce_type;

	memcpy(res->dims, dim->dims, 
		sizeof(int_t) * (dim->n_tuple));
	memcpy(res->offsets, dim->offsets, sizeof(int_t) * (dim->n_entry));
	memcpy(res->indices, dim->indices, sizeof(int_t) * (dim->n_entry * dim->n_tuple));
	return res;
}

// 设置dim的一个entry
void		dimdesc_set_entry(dimdesc p, int_t entry, int_t offset, ...)
{
	va_list args;
	int	i;

	p->offsets[entry] = offset;
	va_start(args, offset);
	for (i=0; i<p->n_tuple; i++)
		p->indices[entry * p->n_tuple + i] = va_arg(args, int);
	va_end(args);
}

// 创建并返回一个datadesc_set的深拷贝
datadesc_set	datadesc_set_copy(datadesc_set p) {
	datadesc_set res;
	int_t i, j;
	res = datadesc_set_new(p->n);
	for (i = 0; i < p->n; i++){
		res->p[i] = datadesc_new(p->p[i]->n_dim);
		res->p[i]->region = p->p[i]->region;
		res->p[i]->ref = p->p[i]->ref;
		res->p[i]->offset0 = p->p[i]->offset0;
		res->p[i]->endian = p->p[i]->endian;
		res->p[i]->elem_type = p->p[i]->elem_type;
		res->p[i]->elem_size = p->p[i]->elem_size;

		for (j = 0; j < res->p[i]->n_dim && p->p[i]->dims[j]; j++){
			res->p[i]->dims[j] = dimdesc_copy(p->p[i]->dims[j]);
		}
		for (; j < res->p[i]->n_dim; j++) {
			res->p[i]->dims[j] = NULL;
		}
	}
	return res;
}

/* 
	对指定dimdesc的dims[]递增排序，有助于统一同类dimdesc内dims[]内容（使用: datadesc_set_merge）
	要求dimdesc不为NULL
*/
datadesc	sort_tuple_name(datadesc p, int_t dim_index) {
	int_t swap_flag = 0;
	int_t i, j;
	dimdesc cur_dim = p->dims[dim_index];
	int_t *ori_dims = (int_t *)malloc(sizeof(int_t) * cur_dim->n_tuple);
	for (i = 0; i < cur_dim->n_tuple; i++) {
		ori_dims[i] = cur_dim->dims[i];
	}
	for (i = 0; i < cur_dim->n_tuple - 1; i++) {
		for (j = 0; j < cur_dim->n_tuple - i - 1; j++) {
			if (cur_dim->dims[j] > cur_dim->dims[j + 1]) {
				int_t temp_tuple_name;
				temp_tuple_name = cur_dim->dims[j];
				cur_dim->dims[j] = cur_dim->dims[j + 1];
				cur_dim->dims[j + 1] = temp_tuple_name;
				swap_flag = 1;
			}
		}
	}
	// dims[]顺序改变，则indices[]顺序也需要改变
	// NOTE: 如果dims[]出现重复元素（非法情况），则会导致indices[]错乱。
	if (swap_flag) {
		int_t *tuple_name_map = (int_t *)malloc(sizeof(int_t) * cur_dim->n_tuple);
		for (i = 0; i < cur_dim->n_tuple; i++) {
			for (j = 0; j < cur_dim->n_tuple; j++) {
				if (ori_dims[i] == cur_dim->dims[j]) {
					tuple_name_map[i] = j;
					break;
				}
			}
		}
		int_t *ori_indices = (int_t *)malloc(sizeof(int_t) * cur_dim->n_tuple);
		for (i = 0; i < cur_dim->n_entry; i++) {
			for (j = 0; j < cur_dim->n_tuple; j++) {
				ori_indices[j] = cur_dim->indices[i * cur_dim->n_tuple + j];
			}
			for (j = 0; j < cur_dim->n_tuple; j++) {
				cur_dim->indices[i * cur_dim->n_tuple + tuple_name_map[j]] = ori_indices[j];
			}
		}
	}
	return p;
} 

/*
	对指定dimdesc的entry根据offsets进行排序；统一设置dim->offsets[0] = 0，为了不改变数据点实际off，同步修改offset0和dim->offsets[]
	要求dimdesc不为NULL
*/
datadesc	sort_entry(datadesc p, int_t dim_index) {
	dimdesc d = p->dims[dim_index];
	int_t j, k;
	for (j=0; j<d->n_entry; j++)
	{
		for (k=j+1; k<d->n_entry; k++)
		{
			if (d->offsets[j] > d->offsets[k])
			{
				int_t	buf, ii;
				buf = d->offsets[j];
				d->offsets[j] = d->offsets[k];
				d->offsets[k] = buf;
				for (ii=0; ii<d->n_tuple; ii++)
				{
					buf = d->indices[j*d->n_tuple+ii];
					d->indices[j*d->n_tuple+ii] = d->indices[k*d->n_tuple+ii];
					d->indices[k*d->n_tuple+ii] = buf;
				}
			}
		}		
	}
	p->offset0 += p->elem_size*d->offsets[0];
	for (j=d->n_entry; j-->0;)
	{
		d->offsets[j] -= d->offsets[0];
	}
	return p;
}

/*
	对指定dimdesc递归地拆分，直至无法再拆，拆分出的每个n_tuple=1的dimdesc依次追加在dims[]（非NULL部分）末
	要求dimdesc不为NULL
	如果拆分前已经调用sort_entry，则拆分后各dimdesc的entry保持有序
	// TODO: indices = (A, B) × (C, D) 类型的拆分
*/
datadesc	datadesc_normalize_1dim(datadesc p, int_t dim_index) {
	datadesc res = p;
	dimdesc dim;
	int_t tuple, entry;
	dim = res->dims[dim_index];
	if (dim->n_tuple <= 1)
	{
		return res;
	}
	else
	{
		int_t split_flag = 0;
		// 遍历每个tuple，判断该tuple(记为A)是否可拆分: indices = A × ({tuples} - A)
		for (tuple=0; tuple < dim->n_tuple; tuple++)
		{
			// indice_tuple_A_set: tuple A的indice取值集合，采用indice_tuple数据结构
			indice_tuple **indice_tuple_A_set = (indice_tuple **)malloc(sizeof(indice_tuple *) * dim->n_entry);
			int_t indice_tuple_A_cnt = 0;
			// indice_tuple_set：indice_tuple的取值集合
			indice_tuple **indice_tuple_set = (indice_tuple **)malloc(sizeof(indice_tuple *) * dim->n_entry);
			// indice_tuple_subA_set：去掉维度A后，indice_tuple的取值集合
			indice_tuple **indice_tuple_subA_set = (indice_tuple **)malloc(sizeof(indice_tuple *) * dim->n_entry);
			int_t indice_tuple_cnt = 0;
			int_t indice_tuple_subA_cnt = 0;
			// 遍历所有entry，填充indice_tuple_A_set，indice_tuple_set和indice_tuple_subA_set
			// 事实上，indice_tuple_A_set和indice_tuple_subA_set也会按照offset顺序
			// 从entry=0获取的元素在各set中索引同样为0
			for (entry=0; entry < dim->n_entry; entry++)
			{
				indice_tuple *temp_indice_tuple_A = (indice_tuple *)malloc(sizeof(indice_tuple)+sizeof(int_t)*1);
				temp_indice_tuple_A->n_tuple = 1;
				indice_tuple *temp_indice_tuple = (indice_tuple *)malloc(sizeof(indice_tuple)+sizeof(int_t)*(dim->n_tuple));
				temp_indice_tuple->n_tuple = dim->n_tuple;
				indice_tuple *temp_indice_tuple_subA = (indice_tuple *)malloc(sizeof(indice_tuple)+sizeof(int_t)*(dim->n_tuple));
				temp_indice_tuple_subA->n_tuple = dim->n_tuple;
				int_t temp_tuple;
				int_t temp_indice;
				for (temp_tuple=0; temp_tuple < dim->n_tuple; temp_tuple++)
				{
					temp_indice = dim->indices[entry*(dim->n_tuple)+temp_tuple];
					temp_indice_tuple->indices[temp_tuple] = temp_indice;
					if (temp_tuple == tuple)
					{
						temp_indice_tuple_A->indices[0] = temp_indice;
						// -1：indice_tuple_subA保留了tuple_A的位置，但是统一为-1，消除tuple_A带来的影响
						temp_indice_tuple_subA->indices[temp_tuple] = -1;
					}
					else
					{
						temp_indice_tuple_subA->indices[temp_tuple] = temp_indice;
					}
				}
				// 这里检查了indice_tuple是否重复
				if (find_indice_tuple(indice_tuple_set, indice_tuple_cnt, temp_indice_tuple) == -1)
				{
					temp_indice_tuple->offset = dim->offsets[entry];
					indice_tuple_set[indice_tuple_cnt++] = temp_indice_tuple;
				}
				else
				{
					printf("indice_tuple 重复\n");
					// TODO: 抛出异常
				}
				if (find_indice_tuple(indice_tuple_A_set, indice_tuple_A_cnt, temp_indice_tuple_A) == -1)
				{
					indice_tuple_A_set[indice_tuple_A_cnt++] = temp_indice_tuple_A;
				}
				if (find_indice_tuple(indice_tuple_subA_set, indice_tuple_subA_cnt, temp_indice_tuple_subA) == -1)
				{
					indice_tuple_subA_set[indice_tuple_subA_cnt++] = temp_indice_tuple_subA;
				}
			}
			// indice_tuple_set ⊆ indice_tuple_subA_set × indice_tupleA_set。若二者元素个数相等，则两个集合相等，进行下一步offset检查
			// indice_tuple_cnt在合法情况下等于n_entry
			if (indice_tuple_subA_cnt * indice_tuple_A_cnt <= indice_tuple_cnt)
			{
				// dim未进行任何拆分时，base_indice_tuple_subA->offset + base_indice_tuple_A->offset = 0。因此，规定二者分别为0，保证拆分结果符合offset[0]=0的要求
				indice_tuple *base_indice_tuple_subA = indice_tuple_subA_set[0];
				base_indice_tuple_subA->offset = 0;
				indice_tuple *base_indice_tuple_A = indice_tuple_A_set[0];
				base_indice_tuple_A->offset = 0;
				// 从1开始遍历，0处元素已作为base处理
				int_t indice_tuple_A_set_entry;
				indice_tuple *base_indice_tuple_subA_modified = copy_indice_tuple(base_indice_tuple_subA);
				// 获取A的offsets："indice_tuple_subA_set[0]"与"A->entry"组合拼接，匹配indice_tuple_set[]的entry；
					// 所得indice_tuple_set[]的entry的offset就是A中对应entry的offset
				for (indice_tuple_A_set_entry=1; indice_tuple_A_set_entry < indice_tuple_A_cnt; indice_tuple_A_set_entry++)
				{
					indice_tuple *temp_indice_tuple_A = indice_tuple_A_set[indice_tuple_A_set_entry];
					base_indice_tuple_subA_modified->indices[tuple] = temp_indice_tuple_A->indices[0];
					int_t  indice_tuple_set_entry;
					indice_tuple_set_entry = find_indice_tuple(indice_tuple_set, indice_tuple_cnt, base_indice_tuple_subA_modified);
					temp_indice_tuple_A->offset = indice_tuple_set[indice_tuple_set_entry]->offset;
				}
				int_t indice_tuple_subA_set_entry;
				int_t base_indice_A = base_indice_tuple_A->indices[0];
				// 获取{tuples}-A上的offset
				for (indice_tuple_subA_set_entry=1; indice_tuple_subA_set_entry < indice_tuple_subA_cnt; indice_tuple_subA_set_entry++)
				{
					indice_tuple *temp_indice_tuple_subA = indice_tuple_subA_set[indice_tuple_subA_set_entry];
					indice_tuple *temp_indice_tuple_subA_modified = copy_indice_tuple(temp_indice_tuple_subA);
					temp_indice_tuple_subA_modified->indices[tuple] = base_indice_A;
					int_t indice_tuple_set_entry;
					indice_tuple_set_entry = find_indice_tuple(indice_tuple_set, indice_tuple_cnt, temp_indice_tuple_subA_modified);
					temp_indice_tuple_subA->offset = indice_tuple_set[indice_tuple_set_entry]->offset;
				}
				// offset检查
				int_t temp_indice_tuple_entry;
				indice_tuple *temp_indice_tuple;
				indice_tuple *temp_indice_tuple_A;
				indice_tuple *temp_indice_tuple_subA;
				int_t flag = 0;
				for (indice_tuple_subA_set_entry=0; indice_tuple_subA_set_entry < indice_tuple_subA_cnt; indice_tuple_subA_set_entry++)
				{
					temp_indice_tuple_subA = copy_indice_tuple(indice_tuple_subA_set[indice_tuple_subA_set_entry]);
					for (indice_tuple_A_set_entry=0; indice_tuple_A_set_entry < indice_tuple_A_cnt; indice_tuple_A_set_entry++)
					{
						temp_indice_tuple_A = indice_tuple_A_set[indice_tuple_A_set_entry];
						temp_indice_tuple_subA->indices[tuple] = temp_indice_tuple_A->indices[0];
						temp_indice_tuple_entry = find_indice_tuple(indice_tuple_set, indice_tuple_cnt, temp_indice_tuple_subA);
						temp_indice_tuple = indice_tuple_set[temp_indice_tuple_entry];
						if(temp_indice_tuple_subA->offset + temp_indice_tuple_A->offset != temp_indice_tuple->offset)
						{
							flag = 1;
							printf("tuple %ld 不能拆分\n", tuple);
							break;
						}
					}
					if (flag == 1)
					{
						break;
					}
				}
				if (flag == 0)
				{
					printf("tuple %ld 可以拆分\n", tuple);
					//拆分动作
					dimdesc newDim_A = dimdesc_new(1, indice_tuple_A_cnt);
					dimdesc newDim_subA = dimdesc_new(dim->n_tuple-1, indice_tuple_subA_cnt);
					newDim_A->reduce_type = dim->reduce_type;
					newDim_subA->reduce_type = dim->reduce_type;
					newDim_A->dims[0] = dim->dims[tuple];
					int_t temp_tuple;
					int_t target_tuple = 0;
					for (temp_tuple=0; temp_tuple < dim->n_tuple; temp_tuple++)
					{
						if(temp_tuple == tuple)
						{
							continue;
						}
						else
						{
							newDim_subA->dims[target_tuple] = dim->dims[temp_tuple];
							target_tuple++;
						}
					}
					for (indice_tuple_A_set_entry=0; indice_tuple_A_set_entry < indice_tuple_A_cnt; indice_tuple_A_set_entry++)
					{
						temp_indice_tuple_A = indice_tuple_A_set[indice_tuple_A_set_entry];
						newDim_A->offsets[indice_tuple_A_set_entry] = temp_indice_tuple_A->offset;
						newDim_A->indices[indice_tuple_A_set_entry] = temp_indice_tuple_A->indices[0];
					}
					for (indice_tuple_subA_set_entry=0; indice_tuple_subA_set_entry < indice_tuple_subA_cnt; indice_tuple_subA_set_entry++)
					{
						temp_indice_tuple_subA = indice_tuple_subA_set[indice_tuple_subA_set_entry];
						newDim_subA->offsets[indice_tuple_subA_set_entry] = temp_indice_tuple_subA->offset;
						target_tuple = 0;
						for (temp_tuple=0; temp_tuple < dim->n_tuple; temp_tuple++)
						{
							if (temp_tuple == tuple)
							{
								continue;
							}
							else
							{
								newDim_subA->indices[indice_tuple_subA_set_entry*(dim->n_tuple-1) + target_tuple] = temp_indice_tuple_subA->indices[temp_tuple];
								target_tuple++;
							}
						}
					}
					// 拆分会产生新dim，如果原dims[]中有NULL，则新的dim占据NULL位置；否则需要new新的datadesc，扩容dims[]
					datadesc p1;
					const int_t dims_cnt = query_dims_cnt(res);
					if (dims_cnt < res->n_dim) {
						// 原dims[]中有NULL，无需扩容
						p1 = res;
					} else {
						// 原dims[]中没有NULL，new新的datadesc以扩容dims
						p1 = datadesc_new(res->n_dim + 1);
						p1->elem_size = res->elem_size;
						p1->elem_type = res->elem_type;
						p1->endian = res->endian;
						p1->offset0 = res->offset0;
						p1->ref = res->ref;
						p1->region = res->region;
						// 循环条件中无需再判断dims[]元素是否为NULL
						for (int temp_dim_index = 0; temp_dim_index < res->n_dim; temp_dim_index++) {
							if (temp_dim_index != dim_index) {
								p1->dims[temp_dim_index] = res->dims[temp_dim_index];
							}
						}
					}
					p1->dims[dim_index] = newDim_subA;
					p1->dims[dims_cnt] = newDim_A;
					res = p1;
					split_flag = 1;
					break;
				}
			}
		}
		if (split_flag == 0)
		{
			return res;
		} else {
			return datadesc_normalize_1dim(res, dim_index);
		}
	}
}

// TODO: 是否应该把维度和指标的合法性检查也放在这个函数里？
datadesc	datadesc_normalize(datadesc p)
{
	int_t	i, j, k;
	int_t dim_index, entry, tuple;
	dimdesc	d;
	dimdesc dim;
	datadesc res = p;
	// 对dim内entry按照offset排序，且offsets[0]=0
	for (i=0; i<res->n_dim && res->dims[i]; i++)
	{
		res = sort_entry(res, i);
	}

	// 整理非稀疏dim，遍历每个dim，判断是否有可以拆分的tuple
	dim_index = 0;
	int_t ori_ndim = res->n_dim;
	while (dim_index < ori_ndim && res->dims[dim_index])
	{
		res = datadesc_normalize_1dim(res, dim_index);
		dim_index++;
	}
	// 对各dimdesc根据offsets排序（考虑到dimdesc的拆分会产生新的维度，需要在拆分结束后再执行该排序）
	for (i=0; i<res->n_dim && res->dims[i]; i++)
		for (j=i+1; j<res->n_dim && res->dims[j]; j++)
			if (res->dims[i]->offsets[res->dims[i]->n_entry-1] < res->dims[j]->offsets[res->dims[j]->n_entry-1])
			{
				d = res->dims[i];
				res->dims[i] = res->dims[j];
				res->dims[j] = d;
			}

	// 整理dims[]顺序，使同类dimsdesc的dims[]一致
	int_t swap_flag = 0;
	for (dim_index = 0; dim_index < res->n_dim && res->dims[dim_index]; dim_index++) {
		res = sort_tuple_name(res, dim_index);
	}
	return	res;
}

/*
	提取datadesc中指定dim(tuple)的指标为index的数据点，构成一个新的datadesc——相当于对datadesc做slice
	深拷贝，压缩掉原dims[]的NULL元素（仍然有可能因为dim被pick导致dims[]产生新的NULL）
*/
datadesc	datadesc_pick(datadesc p, int_t dim, int_t index)
{
	datadesc	res;
	dimdesc	df, dt;
	int_t	i, j, k, l, m, n, o;

	i = query_dims_cnt(p);	//NOTE: 保守一点，可能会多个空位。
	res = datadesc_new(i);

	res->region = p->region;
	res->ref = p->ref;
	res->offset0 = p->offset0;
	res->endian = p->endian;
	res->elem_type = p->elem_type;
	res->elem_size = p->elem_size;

	for (i=0, j=0; i < p->n_dim && p->dims[i]; i++)
	{
		df = p->dims[i];
		for (k=0; k < df->n_tuple && df->dims[k] != dim; k++);
		if (k < df->n_tuple)
		{
			if (df->n_tuple == 1)
			{
				for (l=0; l < df->n_entry && df->indices[l] != index; l++);
				if (l < df->n_entry)
				{
					// 在非稀疏的维度上找到了指标
					res->offset0 += res->elem_size*df->offsets[l];
				}
				else
					goto	empty;
			}
			else
			{
				for (l=0, m=0; l < df->n_entry; l++)
					if (df->indices[l*df->n_tuple+k] == index)
						m++;
				if (m > 0)
				{
					// 在稀疏的维度上找到了指标
					dt = dimdesc_new(df->n_tuple-1, m);
					dt->reduce_type = df->reduce_type;
					for (l=0, m=0; l < df->n_tuple; l++)
						if (l != k)
							dt->dims[m++] = df->dims[l];
					for (l=0, m=0; l < df->n_entry; l++)
						if (df->indices[l*df->n_tuple+k] == index)
						{
							dt->offsets[m] = df->offsets[l];
							for (n=0, o=0; n<df->n_tuple; n++)
								if (n != k)
									dt->indices[m*dt->n_tuple+(o++)] = df->indices[l*df->n_tuple+n];
							m++;
						}
					res->dims[j] = dt;
					j++;
				}
				else
					goto	empty;
			}
		}
		else
		{
			// 无关的维度
			dt = dimdesc_copy(df);
			res->dims[j] = dt;
			j++;
		}
	}
	// 可能存在完全被pick掉的dim，导致实际dims数减少；该dim后续的dims已经前移，在dims[]末设置NULL
	for (; j<res->n_dim; j++)
		res->dims[j] = NULL;

	return	res;

empty:
	// TODO: 这个处理是否合适？需要仔细考虑"空数据"在各种情况下的效果，下同。
	for (j=0; j<res->n_dim; j++)
		res->dims[j] = NULL;	// 为方便GC操作而擦除
	res->n_dim = 0;
	res->region = region_invalid;
	return	res;
}






/** FUNCTION: search number in arr[size]
*   return res:
*   		0   success
*   		-1  fail
*/
static int_t find_int(int_t* arr, int_t size, int_t number){
	int_t res = -1;
	int_t i = 0;
	for (i = 0; i < size; i++){
		if (arr[i] == number){
			res = 0;
			break;
		}
	}
	return res;
}


/** FUNCTION: cartesian_product: 对于p的第dim1维和dim2维，求其笛卡尔积并置于dim1，从dim2开始，用后一个dim顶替前一个dim
*   PARAMETERS: p 原datadesc； dim1 第一个维度； dim2 第二个维度（必须在函数外调整为dim1 < dim2！！！）；调用方需要保证dim不为NULL
*   RETURN:    不再构建新的datadesc，直接返回修改的原来的datadesc的地址
*/
static datadesc cartesian_product(datadesc p, int_t dim1, int_t dim2){
	// FIXME: 直接修改p指向的内容，返回值是否必要？如果不合理，需要新建一个datadesc res
	dimdesc dim_p1, dim_p2;
	dimdesc new_dim;
	int_t i, j, k, l, line;

	const int_t dims_cnt = query_dims_cnt(p);

	dim_p1 = p->dims[dim1];
	dim_p2 = p->dims[dim2];

	// 笛卡尔积的行数等于原来行数的乘积，列数等于原来列数之和
	new_dim = dimdesc_new(dim_p1->n_tuple + dim_p2->n_tuple, dim_p1->n_entry * dim_p2->n_entry);
	new_dim->reduce_type = dim_p1->reduce_type;   	// FIX: 是否合理？

	line = 0; 	// 新生成的维度的第line行
	for (i = 0; i < dim_p1->n_entry; i++){
		for (j = 0; j < dim_p2->n_entry; j++){
			new_dim->offsets[line] = dim_p1->offsets[i] + dim_p2->offsets[j]; 
			
			for (k = 0; k < dim_p1->n_tuple; k++)
				new_dim->indices[line * new_dim->n_tuple + k] = dim_p1->indices[i * dim_p1->n_tuple + k];
			for (l = 0; l < dim_p2->n_tuple; l++)
				new_dim->indices[line * new_dim->n_tuple + dim_p1->n_tuple + l] = dim_p2->indices[j * dim_p2->n_tuple + l];
			line++;
		}
	}
	
	// dims数组: dimnames_g的下标，同样是dim_p1->n_tuple + dim_p2->n_tuple
	for (k = 0; k < dim_p1->n_tuple; k++)
		new_dim->dims[k] = dim_p1->dims[k];
	for (l = 0; l < dim_p2->n_tuple; l++)
		new_dim->dims[dim_p1->n_tuple + l] = dim_p2->dims[l];

	// 选择方案二（如果后续替换为方案一，则多个调用处均需要修改）
	// 方案一：新dim放到dim1的位置，最后一个dim顶替dim2，n_dims <--- (n_dims - 1)
	//      p->dims[dim1] = new_dim;     p->dims[dim2] = p->dims[p->n_dims-1]   p->dims[p->n_dims-1] = NULL
	// 方案二：新dim放到dim1的位置，从dim2开始，用后一个dim顶替前一个dim， n_dims<---(n_dims - 1)
	//	  这里为了保证稀疏维度下的维度顺序不变，使用方案二，但是复杂度也会变高O(1)---->O(n), 这个策略被dimmap函数所用到！！！
	p->dims[dim1] = new_dim;
	for (i = dim2; i < dims_cnt - 1; i++){
		p->dims[i] = p->dims[i+1];
	}

	// 笛卡尔积会缩减datadesc中实际dims的数量，在dims[]内引入NULL
	p->dims[dims_cnt - 1] = NULL;
	// 不应该修改p->n_dim

	return p;
}

/** FUNCTION: find_dim
*   DESCRIPTION: 在p中找到dimnames_g中下标为dim的那个维度
*   RETURN: success: p中dims的数组下标[0, p->n_dim)   fail：p->n_dim（NOTE：不返回第一个NULL的下标）
*/
static int_t find_dim(datadesc p, int_t dim) {
	int_t res;
	int_t i, j;

	res = p->n_dim;

	for (i = 0; i < p->n_dim && p->dims[i]; i++){
		for (j = 0; j < p->dims[i]->n_tuple; j++){
			if (dim == p->dims[i]->dims[j]){
				res = i;
				return res;
			}
		}
	}
	return res;
}


/** FUNCTION: dim_find_common
* 	DESCRIPTION: 找出p1->dims[dim_pos1]和p2->dims[dim_pos2]在tuple交集中indices取值的交集
*				例如找出(x1,y1,z1)和(y2,z2,t2)中 y1 == y2 && z1==z2的部分
*				剔除p1->dims[dim_pos1]中在交集外的entry
*				传入的是指针，所有的修改直接在原来的数据上进行
*   WARNING: 该函数要求调用方保证pos1，pos2指向的dim不为NULL
*/
static void dim_find_common(datadesc p1, int_t dim_pos1, datadesc p2, int_t dim_pos2){
	dimdesc dim_p1, dim_p2;

	dim_p1 = p1->dims[dim_pos1];
	dim_p2 = p2->dims[dim_pos2];
	int_t i, j, k, l;
	int_t temp;

	int_t common_pos1, common_pos2;
	char equal;

	common_pos1 = common_pos2 = 0;
	for (i = 0; i < dim_p1->n_entry; i++){
		// 去dim_p2寻找dim_p1的每一行数据
		
		for (j = 0; j < dim_p2->n_entry; j++){	// 修改为从0开始，因为dim1可能有交叉冗余行
			equal = 1;			// 首先假设两行的公共分量相等，如果有分量不等，直接equal = 0 break掉
			// dim_p1 的第i行和dim_p2的第j行比较，只比较公共的列
			for (k = 0; k < dim_p1->n_tuple && equal; k++){
				for(l = 0; l < dim_p2->n_tuple && equal; l++){
					if (dim_p1->dims[k] == dim_p2->dims[l]){
						// 分量相同，进行比较
						if (dim_p1->indices[i * dim_p1->n_tuple + k] != dim_p2->indices[j * dim_p2->n_tuple + l]){
							equal = 0;
						}
					}
				}	// for 1
			}	// for 2
			if (equal) {		// dim_p1的第i行和dim_p2的第j行是common的
				// dim_p1以及dim_p2的处理不同
				// 对于dim_p1, 之前遍历的行如果没出现在dim_p2，直接用后面的common行覆盖前面的行    -----行覆盖策略
				// 对于dim_p2，由于其是被参考的，不是按顺序的，交换策略是new_pos行和j行交换        -----行交换策略
				// 这么做导致最后结果dim_p1, dim_p2的每一行都是对应的，比如(x,y,z)和(y,z,t)的(y,z)是对应的

				if (common_pos1 == i) {
					// 表示dim_p1->entry[i]之前的元素都在dim_p2中找到，直接保留即可，无需处理
					common_pos1++;
				}
				else if (common_pos1 < i){
					// 策略：行覆盖，common_pos1之所以不等于i就是因为中间有没搜到的行
					dim_p1->offsets[common_pos1] = dim_p1->offsets[i];
					for (k = 0; k < dim_p1->n_tuple; k++){
						dim_p1->indices[common_pos1 * dim_p1->n_tuple + k] = dim_p1->indices[i * dim_p1->n_tuple + k];
					}
					common_pos1++; // common_pos1不可能超过i
				}

				// common_pos1一定自增，common_pos2不一定自增
				break; // 如果需要修改dim_p2，则删除这个break，不然dim_p2会少行
			}
		}	// for 3
		
	}	// for 4
	dim_p1->n_entry = common_pos1;

	// FIXME: 覆盖+交换，使得一个双循环能够同时处理掉dim_p1和dim_p2中的单独元素。但是，既然要走两轮expand_dim，真的有必要处理dim_p2吗？目前的策略是不处理dim_p2
}

/** FUNCTION: expand_dim
*   DESCRIPTION: 以p2的第pos2-1个dim为准，扩展p1的dim，如p2的(x,y)，会导致p1的x，y变为稀疏维度
*   INPUT: p1是需要扩展的维度，p2的pos2是被参考的稀疏维度，调用方保证p2->dims[pos2]不为NULL
*   OUTPUT:
*   RETURN:
*/
static void expand_dim(datadesc p1, datadesc p2, int_t dim_pos2){
	dimdesc dim_p1, dim_p2;
	int_t i;
	int_t temp;
	int_t pos1, pos2;

	pos1 = p1->n_dim;	// p1->n_dim为初始（重置）值，表示pos未标记与p2->dims[dim_pos2]匹配的dim
	pos2 = p1->n_dim;
	dim_p2 = p2->dims[dim_pos2];
	// FIXME: 能否先在p1中找到dim_p2的全部分量，完成笛卡尔积后再统一调用一次dim_find_common？事实上多次调用dim_find_common有助于减少复杂度和内存占用？

	for (i = 0; i < dim_p2->n_tuple; i++){
		// 对于p2->dims[pos2]中的每个分量，都去p1中寻找
		if (pos1 == p1->n_dim){
			pos1 = find_dim(p1, dim_p2->dims[i]);
			// 根据现在的算法，既能处理x (x,y,z)这种情况，也能处理(x,y)  (x,y,z)这种情况
			if (pos1 == p1->n_dim)	continue;  // p1里面找不到这个维度
			dim_find_common(p1, pos1, p2, dim_pos2);	// 第一次就进行find_common
		}
		else if (pos2 == p1->n_dim){
			pos2 = find_dim(p1, dim_p2->dims[i]);
			if (pos2 == p1->n_dim) continue;
		}
		else{	// 防守逻辑，之前找到了两个分量pos1，pos2且已经做完笛卡尔积, 那么笛卡尔积的结果放在pos1，pos2直接舍弃
			pos2 = find_dim(p1, dim_p2->dims[i]);
			if (pos2 == p1->n_dim) continue;
		}
		
		// 能够在p1里找到稀疏维度里面的维度
		if (pos1 != p1->n_dim && pos2 != p1->n_dim){
			if (pos1 != pos2){
				if (pos1 > pos2){   // FIXME: 保证pos1 < pos2 有没有必要？
					temp = pos1;
					pos1 = pos2;
					pos2 = temp;
				}
				p1 = cartesian_product(p1, pos1, pos2);
				pos2 = p1->n_dim;
				// 剪切
				dim_find_common(p1, pos1, p2, dim_pos2);	// pos1 每加一个分量，就跟p2->dims[dim_pos2]进行一次find_common，避免冗余数据
			}
			else{	// 两分量在同一元组，只保留pos1
				pos2 = p1->n_dim;
			}
		}
	}
}



/** FUNCTION: compare_and_find_common     (private)
*   DESCRIPTION: 对p1和p2指向的元素进行find common操作，暂时不考虑其他datadesc，只考虑这两个的相互影响
*				所有对数据的修改，都在传入的数据上进行
*/
static void compare_and_find_common(datadesc p1, datadesc p2){
	int_t i;

	// 第一趟，p2各dim依次作为参照，p1元素改变，p2传染p1
	for (i = 0; i < p2->n_dim && p2->dims[i]; i++){
		expand_dim(p1, p2, i);
	}
	// 第二趟，p1各dim依次作为参照，p2元素改变，p1传染p2
	for (i = 0; i < p1->n_dim && p1->dims[i]; i++){
		expand_dim(p2, p1, i);
	}
}

/** FUNCTION: datadesc_find_common
*	DESCRIPTION:  策略1:原数据改动     策略2:新建数据
*/
datadesc_set	datadesc_find_common(datadesc_set p)
{
	datadesc_set res;
	int_t i, j, k;
	datadesc data_p1, data_p2;

	// 深拷贝，初始化结果集，并没有压缩datadesc->dims[]的NULL元素
	res = datadesc_set_copy(p);

	for (i = 0; i < res->n - 1; i++){		// 每一对datadesc
		for (j = i + 1; j < res->n; j++){
			compare_and_find_common(res->p[i], res->p[j]);
		}
	}

	// TODO: 预计有冗余内存，处理否？ 数据整理？

	return res;
}

datadesc	datadesc_find_span(datadesc_set p)
{
	datadesc	res;
	int_t		ndim;
	int_t		size;
	int_t		i, j, k;

	p = datadesc_find_common(p);
	for(ndim=0, i=0; i<p->n; i++)
	{
		p->p[i] = datadesc_normalize(p->p[i]);
		ndim += p->p[i]->n_dim;
	}
	res = datadesc_new(ndim);
	res->region = region_none;
	res->ref.p_direct = NULL;
	res->endian = p->p[0]->endian;		// FIXME: 这里暂时直接用第0个输入的信息，但后续应该仔细处理才行
	res->elem_type = p->p[0]->elem_type;
	res->elem_size = p->p[0]->elem_size;
	for (i=0; i<res->n_dim; i++)
		res->dims[i] = NULL;
	for(res->n_dim=0, i=0; i<p->n; i++)
		for (j=0; j<p->p[i]->n_dim && p->p[i]->dims[j]; j++)
		{
			for (k=0; k < res->n_dim && res->dims[k]->dims[0] != p->p[i]->dims[j]->dims[0]; k++);	// FIXME: 这里假设datadesc_normalize之后的顺序是固定的，且datadesc_find_common后的所有相同的维度都带完全相同的指标。
			if (k == res->n_dim)
			{
				res->dims[k] = dimdesc_copy(p->p[i]->dims[j]);		// NOTE: 新建一个dimdesc，不重用
				res->n_dim ++;
			}
		}

	for (size=1, i=res->n_dim; i-->0;)
	{
		for (j=0; j<res->dims[i]->n_entry; j++)
			res->dims[i]->offsets[j] = j*size;
		size *= res->dims[i]->n_entry;
	}
	res->offset0 = size * res->elem_size;		// NOTE: 把占用内存的总大小临时存放在这里，用于后续实例化。

	return	res;
}

void datadesc_set_show(datadesc_set p){
	int_t i, j, k, l;
	datadesc data_p;
	dimdesc dim_p;
	printf("datadesc_set with %lu datddescs : \n", p->n);
	for (i = 0; i < p->n; i++)
	{
		data_p = p->p[i];
		printf("\tdatadesc #%ld :\n", i);
		printf("offset0: %ld\n", data_p->offset0);
		for (j = 0; j < data_p->n_dim && data_p->dims[j]; j++){
			printf ("dim information: rank %ld\n", j);
			dim_p = data_p->dims[j];
			printf ("\t\tdimdesc #%ld with %lu tuples and %lu entries :\n", j, dim_p->n_tuple, dim_p->n_entry);
			printf("\t\t\tdims : \n\t\t\t\t");
			for (k = 0; k < dim_p->n_tuple; k++)
			{
				if (k>0)
					printf(", ");
				printf("%s(%ld)", dimname(dim_p->dims[k]), dim_p->dims[k]);
				//	printf("(%lu)", dim_p->dims[k]);
			}
			printf("\n");
			printf("\t\t\tindices - offsets : \n");
			for (k = 0; k < dim_p->n_entry; k++)
			{
				printf("\t\t\t\t(");
				for (l = 0; l < dim_p->n_tuple; l++)
				{
					if (l>0)
						printf(", ");
					printf("%ld", dim_p->indices[k * dim_p->n_tuple + l]); 
				}
				printf(")\t - \t%ld\n", dim_p->offsets[k]);
			}

		}
	}
}

/** * 
	datadesc_dimmap用于对数据描述中的维度和指标进行变换，其中需要输入类型为dimmap_f的回调函数
 * 	dims_in[]中的所有维度都必须是p中含有的，否则返回NULL。
 * 	dims_out[]中的任何维度要么是在dims_in[]中包含，要么是在p中不存在，否则返回NULL。这是为了避免存在维度冲突
 * 	对于所有在p中有效的indices_in[]组合，f返回的indices_out[]不可以有完全相同的，否则返回NULL。这是为了避免存在指标冲突
 * 	dimmap_f正常应当返回非零值，如果返回0，则扔掉此数据点。
 *  dims_out[]/dims_in[]：存储tuple索引的数组
 * 	tuple在dimnames_g[]的索引能唯一标识维度
*/
datadesc	datadesc_dimmap(datadesc p, dimmap_f f, int_t n_out, int_t *dims_out, int_t n_in, int_t *dims_in){
	datadesc res = NULL;
	dimdesc dim_out_p = NULL;
	dimdesc dim_in_p = NULL;
	dimdesc dim_temp = NULL;
	char flag, input_flag, output_flag;
	int_t func_res;
	int_t i, j, k, l, pos, check_pos;
	int_t temp;
	int_t* indices_in = NULL, * indices_out = NULL;
	int_t* input_of_dim = NULL;
	

	const int_t size = p->n_dim;

	if (n_out == 0 && n_in == 0){
		printf("error: n_out == 0 && n_in == 0\n");
		return NULL;
	}

	/*
	参数检查：维度存在/冲突
 	*/
	input_of_dim = (n_in > 0) ? malloc(n_in * sizeof(int_t)) : NULL;
	// dims_in[]的所有维度都必须是p中含有的，否则返回NULL
	for (i = 0; i < n_in; i++){
		for (j = 0; j < i; j++){
			// 检查dims_in[]是否重复
			if (dims_in[i] == dims_in[j]){
				printf("error: dims_in存在重复元素\n");
				return NULL;
			}
		}
		input_of_dim[i] = find_dim(p, dims_in[i]);
		if (input_of_dim[i] == size){
			printf("error: dims_in中存在p不包含的维度\n");
			return NULL;
		}
	}
	// dims_out[]的任何维度要么是在dims_in[]中包含，要么是在p中不存在，否则返回NULL。这是为了避免存在维度冲突
	for (i = 0; i < n_out; i++){
		for (j = 0; j < i; j++){
			// 检查dims_out[]是否重复
			if (dims_out[i] == dims_out[j]){
				printf("error: dims_out存在重复元素\n");
				return NULL;
			}
		}
		for (j = 0; j < n_in; j++) {
			if (dims_out[i] == dims_in[j])  break;
		}

		if (j == n_in && find_dim(p, dims_out[i]) != size){
			printf("error: dims_out中存在[dims_in不包含但是p包含]的维度\n");
			return NULL;
		}
	}

	res = (n_in == 0) ? datadesc_new(p->n_dim + 1) : datadesc_new(p->n_dim);
	res->region = p->region;
	res->ref = p->ref;
	res->offset0 = p->offset0;
	res->endian = p->endian;
	res->elem_type = p->elem_type;
	res->elem_size = p->elem_size;

	// 深拷贝datadesc
	if (n_in == 0){
		/*
			n_in==0且n_out>0：dims_out[]共同构成一个dim，插入到dims[0]，entry=1
	 	*/
		for (i = 1; i < res->n_dim && p->dims[i-1]; i++){
			res->dims[i] = dimdesc_copy(p->dims[i-1]);
		}
		for (; i < res->n_dim; i++) {
			res->dims[i] = NULL;
		}
		
		indices_out = (int_t *)malloc(n_out * sizeof(int_t));
		if (f(indices_out, NULL)) {
			res->dims[0] = dimdesc_new(n_out, 1);
			res->dims[0]->reduce_type = res->dims[1]->reduce_type;	//FIX: 是否合理？默认求和？包括n_in>0的情况，map前后res->dims[0]->reduce_type不变？
			for (i = 0; i < n_out; i++) {
				res->dims[0]->dims[i] = dims_out[i];
				res->dims[0]->indices[i] = indices_out[i];
			}
			res->dims[0]->offsets[0] = 0;
			// FIXME：normalize_1dim?
			res = datadesc_normalize(res); 
			/* printf("offset0: %ld\n", res->offset0); */
			return res;
		}
		else {
			return NULL;
		}
	}
	else {
		/* 
			n_in>0，n_out==0或n_out>0
	 	*/
		for (i = 0; i < res->n_dim && p->dims[i]; i++){
			res->dims[i] = dimdesc_copy(p->dims[i]);
		}
		for (; i < res->n_dim; i++) {
			res->dims[i] = NULL;
		}
	}

	// input_of_dim[]记录了所有需要map的dim在res->dims[]中的索引
	// 需要map的dim：包含dims_in[]中的tuple
	// input_of_dim[]内的次序其实不重要（当作set使用）
	// 将要被map的dim统一在res->dims[0]
	if (input_of_dim[0] != 0) {
		// 交换res->dims[input_of_dim[0]]和res->dims[0]
		dim_temp = res->dims[0];
		res->dims[0] = res->dims[input_of_dim[0]];
		res->dims[input_of_dim[0]] = dim_temp;
		// 更新input_of_dim[]中指向res->dims[input_of_dim[0]]和res->dims[0]的内容
		for (i = 1; i < n_in; i++) {
			if (input_of_dim[i] == 0) {
				input_of_dim[i] = input_of_dim[0];
			} else if (input_of_dim[i] == input_of_dim[0]){
				input_of_dim[i] = 0;
			}
		}
		input_of_dim[0] = 0;
	}

	/* printf("input_of_dim: \n");
	for (j = 0; j < n_in; j++) {
		printf("%ld\n", input_of_dim[j]);
	} */
	
	// 顺次笛卡尔积
	// FIXME: 顺次笛卡尔积导致dim的稀疏表示，最后需要展开？能否规避顺次笛卡尔积的过程？
	for (i = 1; i < n_in; i++) {
		flag = 1;
		for (j = 0; j < i && flag; j++) {	// 检查input_of_dim[i]对应dim是否因为有其他tuple在dims_in[]中，已经做了笛卡尔积
		// NOTE：这个检查似乎有一点奇怪，虽然能work，但是或许直接和0比较就行
			if (input_of_dim[i] == input_of_dim[j]) {
				flag = 0;
			}
		}
		if (flag) {
			res = cartesian_product(res, 0, input_of_dim[i]);
			// 做完笛卡尔积后更新input_of_dim[]内容，即更新dim_in[]到res->dims[]的索引
			for (j = 1; j < n_in; j++) {
				if (i == j) {
					continue;
				}
				// input_of_dim[i]对应dim完成了笛卡尔积，则input_of_dim[]中所有索引到该dim的元素置为0
				if (input_of_dim[j] == input_of_dim[i]) {
					input_of_dim[j] = 0;
				} else if (input_of_dim[j] > input_of_dim[i]) {	
					// 该dim完成笛卡尔积后移除，后续dim前移1格，input_of_dim[]索引同步更新
					input_of_dim[j]--;
				}
			}
			input_of_dim[i] = 0;

			/* printf("input_of_dim: \n");
			for (j = 0; j < n_in; j++) {
				printf("%ld\n", input_of_dim[j]);
			} */

		}
	}

	// 初始化输出dim的结构
	dim_out_p = dimdesc_new(res->dims[0]->n_tuple - n_in + n_out, res->dims[0]->n_entry);
	dim_out_p->reduce_type = res->dims[0]->reduce_type;
	// 向dim_out_p->dims[]插入dims_out[]
	for (i = 0; i < n_out; i++) {
		dim_out_p->dims[i] = dims_out[i];
	}
	// 向dim_out_p->dims[]插入(dims[0]->dims[]-dims_in[]-dims-out[])
	pos = n_out; 
	for (i = 0; i < res->dims[0]->n_tuple; i++) {
		flag = 1;
		// 检查第i个tuple是否在dims_out[]中
		for (j = 0; j < n_out && flag; j++) {
			if (res->dims[0]->dims[i] == dims_out[j]) {
				flag = 0;
			}
		}
		for (j = 0; j < n_in && flag; j++) {
			if (res->dims[0]->dims[i] == dims_in[j]) {
				flag = 0;
			}
		}
		if (flag) {	// 维度i不在dims_out[]/dims_in[]中
			dim_out_p->dims[pos++] = res->dims[0]->dims[i];
		}
	}


	// new space for indices_in and indices_out
	indices_in = malloc(n_in * sizeof(int_t));
	// NOTE: 对于n_out==0的情况，malloc是未定义行为
	if (n_out > 0) {
		indices_out = malloc(n_out * sizeof(int_t));
	}
	
	pos = 0;	// 已成功map的entry数量（不包含被舍弃的entry）
	dim_in_p = res->dims[0];
	
	// picked_indices_in：记录n_out==0时，被pick的indices_in
	// 当n_out==0时，需要保证有且仅有一组indices_in能够被dimmap_f筛选出，检查多选/0选的情况
	int_t * picked_indices_in = (int_t *)malloc(sizeof(int_t) * n_in);
	int_t pick_flag = 0;

	// 对于每个entry，将需要map维度的indices按照dims_in[]顺序装入indices_in[]后再调用f得到indices_out[]结果
	for (i = 0; i < dim_in_p->n_entry; i++) {	// i：对需要map的entry遍历
		
		/* printf("entry:");
		for (j = 0; j < dim_in_p->n_tuple; j++) {
			printf(" %ld", dim_in_p->indices[i * dim_in_p->n_tuple + j]);
		}	
		printf("\n"); */

		// 将需要map维度的indices按照dims_in[]顺序装入indices_in[]
		for (j = 0; j < n_in; j++) {
			for (k = 0; k < dim_in_p->n_tuple; k++) {
				if (dims_in[j] == dim_in_p->dims[k]) {
					indices_in[j] = dim_in_p->indices[i * dim_in_p->n_tuple + k];
					break;
				}
			}
		}

		if (n_out == 0) {
			// 要求dimmap_f不能操作indices_out
			func_res = f(NULL, indices_in);
		} else {
			func_res = f(indices_out, indices_in);
		}
		/* printf("func_res: %ld\n", func_res); */
		if (!func_res) {
			continue;
		}

		/* printf("in: ");
		for (j = 0; j < n_in; j++) {
			printf("%ld ", indices_in[j]);
		}
		printf("\n out: ");
		for (j = 0; j < n_out; j++) {
			printf("%ld ", indices_out[j]);
		}
		printf("\n"); */
		
		// 检查n_out==0时是否出现多选
		if (n_out == 0){
			if (!pick_flag) {
				pick_flag = 1;
				for (j = 0; j < n_in; j++) {
					picked_indices_in[j] = indices_in[j];
				}
				// 如果后续需要摘除dims_in所在dimdesc，则需要处理offset0
				if (dim_out_p->n_tuple == 0) {
					res->offset0 += dim_in_p->offsets[i] * res->elem_size;
				}
			} else {
				for (j = 0; j < n_in; j++) {
					if (picked_indices_in[j] != indices_in[j]) {
						printf("error: 对于n_out==0的情况，dimmap_f错误筛选了多组indices_in\n");
						return NULL;
					}
				}
			}
		}
		// 检查n_out>0时是否满足单射条件：对于所有在p中有效的indices_in[]组合，f返回的indices_out[]不可以有完全相同的，否则返回NULL。这是为了避免存在指标冲突
		// 每次调用dimmap_f得到indices_out[]时，检查是否与已map的entry冲突。j：已map的entry
		if (n_out > 0) {
			for (j = 0; j < pos; j++) {
				// 只需要检查一种情况：in不同out同。因为，in同out同是正确的、in同out不同不会出现、in不同out不同也是正确的
				
				// 检查entry_i的indices_out[]中每个tuple的indice，若与entry_j的对应tuple的indice都相同，则将output_flag置1，需要检查indices_in
				output_flag = 1;
				for (k = 0; k < n_out && output_flag; k++) {
					for (l = 0; l < dim_out_p->n_tuple; l++) {
						// 搜索在dim_out_p->dims[]中与dims_out[k]相同的维度，索引为l
						if (dim_out_p->dims[l] == dims_out[k]) {
							break;
						}
					}
					if (indices_out[k] != dim_out_p->indices[j * dim_out_p->n_tuple + l]) {
						output_flag = 0;
					}
				}

				// 检查out相同时，in是否相同
				if (output_flag) {
					input_flag = 1;
					// 检查entry_i的indices_in[]中每个tuple的indice，若与entry_j的对应tuple的indice不同，则将input_flag置0，此时返回NULL
					for (k = 0; k < n_in && input_flag; k++) {
						for (l = 0; l < dim_in_p->n_tuple; l++) {
							// 搜索在dim_in_p->dims[]中与dims_in[k]相同的维度，索引为l
							if(dim_in_p->dims[l] == dims_in[k]) {
								break;
							}
						}
						// indices_in[]的装载过程，保证了dims_in[]和indices_in[]维度顺序一致
						if (indices_in[k] != dim_in_p->indices[j * dim_in_p->n_tuple + l]) {
							input_flag = 0;
						}
					}
					// in不同，out相同：返回NULL
					if (output_flag && !input_flag) {
						return NULL;
					}
				}
			}
		}

		// 一旦dims_out_p舍弃过某个entry后，对于新map的entry，需要同步更新entry在dims_in_p的索引，保证在二者的索引相同，便于单射检查等
		// 即，dims_out_p->entry[pos]与dims_in_p->entry[pos]需要对应，无论n_out是否为0
		if (pos != i) {
			dim_in_p->offsets[pos] = dim_in_p->offsets[i];
			for (j = 0; j < dim_in_p->n_tuple; j++) {
				dim_in_p->indices[pos * dim_in_p->n_tuple + j] = dim_in_p->indices[i * dim_in_p->n_tuple + j];
			}
		}
		// 向dim_out_p填入成功map的entry
		dim_out_p->offsets[pos] = dim_in_p->offsets[pos];
		for (j = 0; j < dim_out_p->n_tuple; j++) {
			for (k = 0; k < n_out; k++) {
				if (dims_out[k] == dim_out_p->dims[j]) {
					break;
				}
			}
			// 如果dim_out_p->dims[j]维度是dims_out[]中的维度k，则从indices_out[k]得到entry_i在该维度的indice
			if (k < n_out) {
				dim_out_p->indices[pos * dim_out_p->n_tuple + j] = indices_out[k];
			}
			// 如果dim_out_p->dims[j]维度不在dims_out[]中，则从dims_in_p中得到entry_i在该维度的indice
			else {
				for (k = 0; k < dim_in_p->n_tuple; k++) {
					if (dim_in_p->dims[k] == dim_out_p->dims[j]) {
						break;
					}
				}
				dim_out_p->indices[pos * dim_out_p->n_tuple + j] = dim_in_p->indices[pos * dim_in_p->n_tuple + k];
			}
		}
		pos++;
	}
	// 检查n_out==0时是否出现0选
	if (n_out == 0 && pick_flag == 0) {
		printf("error: 对于n_in==0的情况，dimmap_f错误地没有筛选任何indices_in\n");
		return NULL;
	}
	
	dim_out_p->n_entry = pos;

	if (n_out != 0) {
		res->dims[0] = dim_out_p;
	} else {
		if (dim_out_p->n_tuple != 0) {
			res->dims[0] = dim_out_p;
		} else {
			// n_out==0，且dims_in所在dimdesc没有其他维度，则该dimdesc被完全摘除，datadesc包含的真实dims数减少，引入NULL
			for (i = 1; i < res->n_dim && res->dims[i]; i++) {
				res->dims[i - 1] = res->dims[i];
			}
			res->dims[i - 1] = NULL;
		}
	}
	// FIXME：normalize_1dim?
	res = datadesc_normalize(res);
	/* printf("offset0: %ld\n", res->offset0); */

end:
	input_of_dim = NULL;
	indices_in = NULL;
	indices_out = NULL;
	return res;
}

/* 
	判断datadesc是否归属于该datadesc_set(datadesc_set中各元素有相同的基本信息，以及"维度名"组成)
	如果属于，则添加到该datadesc_set
*/
int_t add_to_datadesc_set(datadesc p1, datadesc_set set) {
	datadesc p2 = set->p[0];
	if (p1->elem_size != p2->elem_size)
		return 0;
	if (p1->elem_type != p2->elem_type)
		return 0;
	if (p1->endian!= p2->endian)
		return 0;
	if (p1->region != p2->region)
		return 0;
	// FIXME: ref的校验
	if (p1->ref.p_refname != p2->ref.p_refname)
		return 0;
	int_t p1_tuple_cnt = 0;
	int_t p2_tuple_cnt = 0;
	for (int_t i = 0; i < p1->n_dim && p1->dims[i]; i++) {
		p1_tuple_cnt += p1->dims[i]->n_tuple;
	}
	for (int_t i = 0; i < p2->n_dim && p2->dims[i]; i++) {
		p2_tuple_cnt += p2->dims[i]->n_tuple;
	}
	if (p1_tuple_cnt != p2_tuple_cnt) {
		return 0;
	}
	// 在"维度名"数量相等的前提下，校验"维度名"组成：p1的维度是否都能在p2中被找到
	for (int_t i = 0; i < p1->n_dim && p1->dims[i]; i++) {
		for (int_t j = 0; j < p1->dims[i]->n_tuple; j++) {
			int_t tuple_name = p1->dims[i]->dims[j];
			int_t flag = 0;
			for (int_t m = 0; !flag && m < p2->n_dim && p2->dims[m]; m++) {
				for (int_t n = 0; n < p2->dims[m]->n_tuple; n++) {
					if (tuple_name == p2->dims[m]->dims[n]) {
						flag = 1;
						break;
					}
				}
			}
			if (!flag) {
				return 0;
			}
		}
	}
	// 将p1加入到set，set已容量初始化为总datadesc_set的容量，无需扩容
	int_t cnt = set->n;
	set->p[cnt] = p1;
	set->n = cnt + 1;
	return 1;
}

/* 
	划分datadesc_set：根据各datadesc的基本信息以及"维度名"组成，将有可能合并的datadesc划分在同一个子集
*/
int_t split_datadesc_set(datadesc_set p, datadesc_set *datadesc_sets) {
	int_t datadesc_set_cnt = 0;
	int_t flag;
	datadesc_set new_datadesc_set;
	for (int_t i = 0; i < p->n; i++) {
		flag = 0;
		for (int_t j = 0; j < datadesc_set_cnt; j++) {
			if (add_to_datadesc_set(p->p[i], datadesc_sets[j])) {
				flag = 1;
				break;
			}
		}
		if (!flag) {
			// FIXME：申请过多冗余空间
			new_datadesc_set = datadesc_set_new(p->n);
			new_datadesc_set->p[0] = p->p[i];
			new_datadesc_set->n = 1;
			datadesc_sets[datadesc_set_cnt++] = new_datadesc_set;
		}
	}
	return datadesc_set_cnt;
}

/*
	判断两个datadesc能否合并，并将相应信息添加到merge_info_1step_set中，返回"合并信息"/NULL
	调用要求：p1和p2有相同的基本信息和"维度名"，且经过normalize统一描述
	已实现: 121，n2n合并统计
	TODO: 12n
*/
merge_info_piece get_merge_possibility(datadesc p1, datadesc p2, int_t p1_index, int_t p2_index) {
	// 偏移量没有对齐，不可能合并
	if ((p1->offset0 - p2->offset0) % ((int_t)p1->elem_size) != 0) {
		return NULL;
	}
	// 统计p->dims[]中不为NULL的维度数量
	const int_t p1_ndim = query_dims_cnt(p1);
	const int_t p2_ndim = query_dims_cnt(p2);
	if (p1_ndim != p2_ndim) {
		return NULL;
	}
	// p1_dim_map: 标记p1_dim与哪个p2_dim匹配; p1_dim只会对应0/1个p2_dim，不会对应到超过1个p2_dim，反之亦然
	int_t *p1_dim_map = (int_t *)malloc(sizeof(int_t) * p1_ndim);
	int_t *p2_dim_map = (int_t *)malloc(sizeof(int_t) * p2_ndim);
	for (int_t i = 0; i < p1_ndim; i++) {
		p1_dim_map[i] = -1;
	}
	for (int_t i = 0; i < p2_ndim; i++) {
		p2_dim_map[i] = -1;
	}
	// 首先根据dimdesc->dims[]匹配p1_dim和p2_dim，只有匹配上的dim才可能构成[完全对应/不完全对应]关系
	// 如果存在不能匹配上的dim，或者匹配上但reduce type不同，则说明p1和p2存在不能构成[完全对应/不完全对应]关系的dim，无法合并
	// 完全对应：完全相同的两个dim；不完全对应：仅有部分entry被一方独占，其他信息相同，拥有相同indices的entry其真实offset也相同
	for (int_t i = 0; i < p1_ndim; i++) {
		dimdesc p1_dim = p1->dims[i];
		for (int_t j = 0; j < p2_ndim; j++) {
			if (p2_dim_map[j] != -1) {
				continue;
			}
			dimdesc p2_dim = p2->dims[j];
			if (p1_dim->n_tuple != p2_dim->n_tuple) {
				continue;
			}
			int_t dim_map_flag = 1;
			// 比较dims[]是否相同：事先已调用normalize对dims[]排序，因此，只需要比较对应位置元素
			for (int_t m = 0; m < p1_dim->n_tuple; m++) {
				if (p1_dim->dims[m] != p2_dim->dims[m]) {
					dim_map_flag = 0;
					break;
				}
			}
			if (dim_map_flag) {
				if (p1_dim->reduce_type != p2_dim->reduce_type) {
					return NULL;
				}
				p1_dim_map[i] = j;
				p2_dim_map[j] = i;
				break;
			}
		}
		if (p1_dim_map[i] == -1) {
			return NULL;
		}
	}
	
	int_t off0_off = 0;
	// p1_dim_flag：完全对应：0，不完全对应：1，不对应：-1
	int_t *p1_dim_flag = (int_t *)malloc(sizeof(int_t) * p1_ndim);
	int_t *p2_dim_flag = (int_t *)malloc(sizeof(int_t) * p2_ndim);
	for (int_t i = 0; i < p1_ndim; i++) {
		p1_dim_flag[i] = -1;
	}
	for (int_t i = 0; i < p2_ndim; i++) {
		p2_dim_flag[i] = -1;
	}
	// 判断dims[]匹配的两个dimdesc的关系：完全对应/不完全对应/不对应；如果出现不对应（即，出现矛盾/错误entry，offs对不上），立刻return 0
	// 矛盾：p1，p2实际上没有描述相同的数据点，但是在相同dim上off冲突，虽然合法，但没有合并描述的可能
	// 错误：p1，p2描述了相同的数据点，但是off不同，是非法的情况
	/* 
		校验offsets，offset0和indices
		offsets以及offset0的校验：
			首先校验offsets，对于维度dim[x]，任意entry[y]，需要满足
			A->dim[x]->entry[y]->offset - B->dim[x]->entry[y]->offset = dim_offs_off 为常数，
			然后用该常数修正offset0
		校验完所有维度的offsets后，校验修正后的offset0是否相等
		MOTE: 如果不完全对应的p1_dim和p2_dim不包含相同指标，则p1和p2并不包含真正一样的数据点，不存在某个数据点的off矛盾的情况；
				因此，不需要验证offs0的偏移，只需要验证各dim下dim_offs_off是否是常数
	*/
	// 标记是否每个维度都至少有一个相同的entry，决定是否需要验证offs0
	int_t all_dim_entry_first_match = 1;
	for (int_t i = 0; i < p1_ndim; i++) {
		dimdesc p1_dim = p1->dims[i];
		dimdesc p2_dim = p2->dims[p1_dim_map[i]];
		// 首先检测p1_dim和p2_dim能否完全对应
		if (p1_dim->n_entry == p2_dim->n_entry) {
			int_t off_flag = 1;
			for (int_t m = 1; m < p1_dim->n_entry; m++) {
				// 事先已调用normalize对entry排序，并统一了offsets[0]为0，因此，如果p1_dim == p2_dim，offsets[]应该完全相同
				if (p1_dim->offsets[m] != p2_dim->offsets[m]) {
					off_flag = 0;
					break;
				}
			}
			if (off_flag) {
				int_t indices_flag = 1;
				for (int_t m = 0; m < (p1_dim->n_entry * p1_dim->n_tuple); m++) {
					// 事先已调用normalize对dims[]和entry排序，因此，如果p1_dim == p2_dim，indices[]应该完全相同
					if (p1_dim->indices[m] != p2_dim->indices[m]) {
						indices_flag = 0;
						break;
					}
				}
				if (indices_flag) {
					p1_dim_flag[i] = 0;
					p2_dim_flag[p1_dim_map[i]] = 0;
				}
			}
		}
		
		// 检测p1_dim和p2_dim是否不完全对应
		// 如果已经完全对应，则跳过不完全对应的检查
		if (p1_dim_flag[i] == 0) {
			continue;
		}
		int_t entry_first_match = 0;
		int_t dim_offs_off = 0;
		for (int_t m = 0; m < p1_dim->n_entry; m++) {
			for (int_t n = 0; n < p2_dim->n_entry; n++) {
				int_t j;
				for (j = 0; j < p1_dim->n_tuple; j++) {
					if (p1_dim->indices[p1_dim->n_tuple * m + j] != p2_dim->indices[p2_dim->n_tuple * n + j]) {
						break;
					}
				}
				if (j == p1_dim->n_tuple) {
					// p1_dim_entry[m]和p2_dim_entry[n]的indices相同: 
					if (entry_first_match == 0) {
						entry_first_match = 1;
						dim_offs_off = p1_dim->offsets[m] - p2_dim->offsets[n];
					} else {
						if (p1_dim->offsets[m] - p2_dim->offsets[n] != dim_offs_off) {
							// 出现了矛盾/错误entry（indices相同但offs不同），则p1_dim和p2_dim不对应，p1和p2不能合并
							return NULL;
						}
					}
				}
			}
		}
		// 至此，p1_dim和p2_dim没有出现矛盾/错误entry（导致中途return 0），就至少是不完全对应
		off0_off += dim_offs_off;
		p1_dim_flag[i] = 1;
		p2_dim_flag[p1_dim_map[i]] = 1;
		if (entry_first_match == 0) {
			all_dim_entry_first_match = 0;
		}
	}
	
	// 至此，所有dim都至少有不完全对应关系；如果不放心，可以冗余地做验证：p1_dim_flag[i] != -1
	// 如果需要验证offset0，且修正后的offset0不同，不能合并
	if (all_dim_entry_first_match == 1 && p2->offset0 - p1->offset0 != off0_off * p1->elem_size) {
		return NULL;
	}

	// 构造合并信息
	int_t merge_tuple_cnt = 0;
	for (int_t i = 0; i < p1_ndim; i++) {
		if (p1_dim_flag[i] == 1) {
			merge_tuple_cnt += p1->dims[i]->n_tuple;
		}
	}
	// p1，p2完全一致，无需合并
	if (merge_tuple_cnt == 0) {
		return NULL;
	}
	merge_info_piece merge_info = (merge_info_piece)malloc(sizeof(merge_info_piece_t) + sizeof(int_t) * merge_tuple_cnt);
	merge_info->merge_info_type = merge_type_1step;
	merge_info->dims_cnt = merge_tuple_cnt;
	int_t k = 0;
	for (int_t i = 0; i < p1_ndim; i++) {
		if (p1_dim_flag[i] == 1) {
			for (int_t j = 0; j < p1->dims[i]->n_tuple; j++) {
				merge_info->dims[k++] = p1->dims[i]->dims[j];
			}
		}
	}
	merge_info->data_index1 = p1_index;
	merge_info->data_index2 = p2_index;
	return merge_info;
}

/*
	根据tuples[]，得到p需要做笛卡尔积的dims[]；修改p，执行笛卡尔积，将笛卡尔积展开的维度放在dims[0]
*/
datadesc	recurrent_cartesian_product(datadesc p, int_t tuple_cnt, int_t *tuples) {
	datadesc res = p;
	int_t *merge_dims_p = (int_t *)malloc(sizeof(int_t) * tuple_cnt);
	int_t merge_dim_cnt_p = 0;
	for (int_t i = 0; i < tuple_cnt; i++) {
		int_t tuple_name = tuples[i];
		// 求j的值，使得tuples[i]被p->dims[j]包含，则p->dims[j]是一个待合并维度
		int_t j;
		for (j = 0; j < res->n_dim && res->dims[j]; j++) {
			int_t tuple_match_flag = 0;
			for (int_t k = 0; k < res->dims[j]->n_tuple; k++) {
				if (tuple_name == res->dims[j]->dims[k]) {
					tuple_match_flag = 1;
					break;
				}
			}
			if (tuple_match_flag == 1) {
				break;
			}
		}
		// 判断p->dims[j]是否已经因为其他tuple被认定为待合并维度，保证merge_dims_p无重复元素
		int_t k;
		for (k = 0; k < merge_dim_cnt_p; k++) {
			if (merge_dims_p[k] == j) {
				break;
			}
		}
		if (k == merge_dim_cnt_p) {
			merge_dims_p[merge_dim_cnt_p++] = j;
		}
	}
	// 根据merge_dims_p[]索引，将p需要合并的维度依次做笛卡尔积展开
	// NOTE：目前没有同步处理merge_dims_p[i]之前的元素的值，则这些元素都会失效；随着循环迭代，merge_dims_p[]只有最后一个元素有效
	for (int_t i = 0; i < merge_dim_cnt_p - 1; i++) {
		// 保证merge_dims_p1[i] < merge_dims_p1[i + 1]，然后调用cartesian_product
		if (merge_dims_p[i] > merge_dims_p[i + 1]) {
			int_t temp = merge_dims_p[i];
			merge_dims_p[i] = merge_dims_p[i + 1];
			merge_dims_p[i + 1] = temp;
		}
		res = cartesian_product(res, merge_dims_p[i], merge_dims_p[i + 1]);
		// merge_dims_p1[i+1]对应维度被合并掉了，需要修改指向该维度及之后维度的索引值（和dimmap部分代码相似，但此处初始merge_dim_p不会有重复元素）
		for (int_t j = i + 2; j < merge_dim_cnt_p; j++) {
			if (merge_dims_p[j] > merge_dims_p[i + 1]) {
				merge_dims_p[j]--;
			}
		}
		merge_dims_p[i + 1] = merge_dims_p[i];
	}
	dimdesc temp_dim = res->dims[0];
	res->dims[0] = res->dims[merge_dims_p[merge_dim_cnt_p - 1]];
	res->dims[merge_dims_p[merge_dim_cnt_p - 1]] = temp_dim;
	res = sort_tuple_name(res, 0);
	res = sort_entry(res, 0);
	return res;
}

datadesc_set	datadesc_subset_merge_1step(datadesc_set p, merge_info_piece merge_info) {
	datadesc_set res = datadesc_set_copy(p);
	datadesc p1 = res->p[merge_info->data_index1];
	datadesc p2 = res->p[merge_info->data_index2];
	// FIXME: 先深拷贝，再变形？trene怎么做到COW？
	p1 = recurrent_cartesian_product(p1, merge_info->dims_cnt, merge_info->dims);
	p2 = recurrent_cartesian_product(p2, merge_info->dims_cnt, merge_info->dims);
	int_t offset0_off = p1->offset0 - p2->offset0;
	dimdesc p1_dim = p1->dims[0];
	dimdesc p2_dim = p2->dims[0];
	int_t entry_add_cnt = 0;
	int_t *entry_add_index = (int_t *)malloc(sizeof(int_t) * p2_dim->n_entry);
	for (int_t i = 0; i < p2_dim->n_entry; i++) {
		int_t j;
		for (j = 0; j < p1_dim->n_entry; j++) {
			int_t k;
			for (k = 0; k < p1_dim->n_tuple; k++) {
				if (p1_dim->indices[j * p1_dim->n_tuple + k] != p2_dim->indices[i * p2_dim->n_tuple + k]) {
					break;
				}
			}
			// p1_dim->entry[j]和p2_dim->entry[i]的indices完全匹配
			if (k == p1_dim->n_tuple) {
				break;
			}
		}
		// 找不到和p2_dim->entry[i]的indices完全匹配的entry，需要add到p1_dim
		if (j == p1_dim->n_entry) {
			entry_add_index[entry_add_cnt++] = i;
		}
	}
	dimdesc new_dim = dimdesc_new(p1_dim->n_tuple, p1_dim->n_entry + entry_add_cnt);
	new_dim->reduce_type = p1_dim->reduce_type;
	memcpy(new_dim->dims, p1_dim->dims, sizeof(int_t) * p1_dim->n_tuple);
	memcpy(new_dim->offsets, p1_dim->offsets, sizeof(int_t) * p1_dim->n_entry);
	memcpy(new_dim->indices, p1_dim->indices, sizeof(int_t) * p1_dim->n_entry * p1_dim->n_tuple);
	// 对dim的偏移量修正，注意溢出
	// 所有要合并的dim已经通过cartesian_product整合在一个dim，p1中该dim添加的entry完全体现了从p2添加的数据点
		// 因此，off0不同引发的合并时p2->dims->offs修正也只会发生在这个dim
	int_t offsets_off = offset0_off/((int_t)p1->elem_size);
	for (int_t i = 0; i < entry_add_cnt; i++) {
		new_dim->offsets[i + p1_dim->n_entry] = p2_dim->offsets[entry_add_index[i]] - offsets_off;
		for (int_t j = 0; j < new_dim->n_tuple; j++) {
			new_dim->indices[(i + p1_dim->n_entry) * new_dim->n_tuple + j] = p2_dim->indices[entry_add_index[i] * p2_dim->n_tuple + j];
		}
	}

	// 合并后datadesc可能变规整，从而用normalize拆分有意义，但仅需要对new_dim进行normalize
		// （或者p1，p2本身就规整，但因为构成包含关系，所以需要merge，在merge过程中展开为稀疏，可以用normalize拆分还原）
	// sort_entry是必要的，但是不需要sort_tuple_name——cartesian_product已经对tuple_name排序，后续操作不会破坏其顺序
	p1->dims[0] = new_dim;
	p1 = sort_entry(p1, 0);
	p1 = datadesc_normalize_1dim(p1, 0);
	// p1指向可能改变，需要重新塞进datadesc_set中
	res->p[merge_info->data_index1] = p1;

	// 摘除p2
	for (int_t i = merge_info->data_index2; i < res->n - 1; i++) {
		res->p[i] = res->p[i + 1];
	}
	res->n = res->n - 1;
	return res;
}

datadesc_set	datadesc_subset_merge_1dim() {

}

/*
	merge_1step()/merge_1dim():
		已实现: 121，n2n合并
		TODO: 12n
	(对应tr_mutat)
*/
datadesc_set	datadesc_subset_merge(datadesc_set p, merge_info_piece merge_info) {
	if (merge_info->merge_info_type == merge_type_1step) {
		return datadesc_subset_merge_1step(p, merge_info);
	} else if (merge_info->merge_info_type == merge_type_1dim) {
		return datadesc_subset_merge_1dim();
	}
	return NULL;
}

/*
	提供p的merge选项，填入mutat并返回选项个数
	调用要求：p中各datadesc有相同的基本信息和"维度名"，且经过normalize统一描述
	(对应tr_mutat_search)
*/
int_t	datadesc_subset_merge_search(datadesc_set p, void *** mutat) {
	
	int_t merge_info_1step_cnt = 0;
	// FIXME：分配了一些冗余空间
	merge_info_piece *merge_info_1step_set = (merge_info_piece *)malloc(sizeof(merge_info_piece) * p->n * p->n); 
	for (int_t i = 0; i < p->n - 1; i++) {
		for (int_t j = i + 1; j < p->n; j++) {
			merge_info_piece new_merge_info = get_merge_possibility(p->p[i], p->p[j], i, j);
			if (new_merge_info != NULL) {
				merge_info_1step_set[merge_info_1step_cnt++] = new_merge_info;
			}
		}
	}
	if (merge_info_1step_cnt == 0) {
		return 0;
	}
	int_t merge_info_1dim_cnt = 0;
	merge_info_piece *merge_info_1dim_set = (merge_info_piece *)malloc(sizeof(merge_info_piece) * merge_info_1step_cnt);
	// 扫描merge_info_1step_set，生成merge_info_1dim_set
	for (int_t i = 0; i < merge_info_1step_cnt; i++) {
		merge_info_piece merge_info = merge_info_1step_set[i];
		int_t j;
		// 判断是否与merge_info_1dim_set中已有元素重复
		for (j = 0; j < merge_info_1dim_cnt; j++) {
			if (merge_info->dims_cnt != merge_info_1dim_set[j]->dims_cnt) {
				continue;
			}
			int_t dims_match_flag = 1;
			for (int_t k = 0; k < merge_info->dims_cnt; k++) {
				if (merge_info->dims[k] != merge_info_1dim_set[j]->dims[k]) {
					dims_match_flag = 0;
					break;
				}
			}
			if (dims_match_flag == 1) {
				break;
			}
		}
		// j==merge_info_1dim_cnt，意味着并没有在merge_info_1dim_set中找到与new_merge_info相同的元素
		if (j == merge_info_1dim_cnt) {
			merge_info_piece new_merge_info = (merge_info_piece)malloc(sizeof(merge_info_piece_t) + sizeof(int_t) * merge_info->dims_cnt);
			new_merge_info->merge_info_type = merge_type_1dim;
			new_merge_info->dims_cnt = merge_info->dims_cnt;
			for (int_t k = 0; k < merge_info->dims_cnt; k++) {
				new_merge_info->dims[k] = merge_info->dims[k];
			}
			merge_info_1dim_set[merge_info_1dim_cnt++] = new_merge_info;
		}
	}
	merge_info_piece *merge_info_set = (merge_info_piece *)malloc(sizeof(merge_info_piece) * (merge_info_1step_cnt + merge_info_1dim_cnt)); 
	for (int_t i = 0; i < merge_info_1step_cnt; i++) {
		merge_info_set[i] = merge_info_1step_set[i];
	}
	for (int_t i = 0; i < merge_info_1dim_cnt; i++) {
		merge_info_set[merge_info_1step_cnt + i] = merge_info_1dim_set[i];
	}

	*mutat = merge_info_set;			// FIXME: 这里有个编译警告，需要仔细确认一下并处理
	return merge_info_1step_cnt + merge_info_1dim_cnt;
}


/*	datadesc_set：
		对同一块数据的描述，可能分散在多个datadesc中
		各datadesc维度数量可以不同
	datadesc_set_merge：
		整理datadesc_set，尝试将datadesc合并成更"规整"的形态
*/
datadesc_set_set	datadesc_set_merge_pre(datadesc_set p) {
	/*  只考虑merge包含完全相同维度名的datadesc：
	 	如果想merge维度名不完全相同的datadescA/datadescB，需要首先将A被压缩的信息还原，即获取datadesc_set全局信息；
		且A被还原的维度，各指标的offset相同，导致与B对应维度下相同指标的offset不同（除非B对应维度各指标offset相同，也能被压缩），
		同维度相同指标的offset不同，不能合并（合并后，offset任何取值都不能同时符合所有数据点）
	*/

	// 首先对所有datadesc做normalize，尽可能拆分dimdesc，并对dimdesc->entry[]，dimdesc->dims[]排序(是后续函数调用的必备前提)
	// FIXME：最低要求：sort_tuple_name，sort_entry
	for (int_t i = 0; i < p->n; i++) {
		p->p[i] = datadesc_normalize(p->p[i]);
	}

	// 然后对所有datadesc根据基本信息和"维度名"分区，只在各分区内尝试合并
	int_t datadesc_set_cnt;
	datadesc_set *datadesc_sets = (datadesc_set *)malloc(sizeof(datadesc_set) * p->n);
	datadesc_set_cnt = split_datadesc_set(p, datadesc_sets);
	datadesc_set_set res = (datadesc_set_set)malloc(sizeof(datadesc_set_set_t) + sizeof(datadesc_set) * datadesc_set_cnt);
	res->n = datadesc_set_cnt;
	for (int_t i = 0; i < datadesc_set_cnt; i++) {
		res->p[i] = datadesc_sets[i];
	}
	return res;
}


/*
传入3个参数，需要修改的datadesc，以及需要修改的dim编号，dima为整合后的味道，dimb为删除掉的维度。
TODO: 稀疏实现
copy
*/
datadesc datadesc_merge_dimb2a(datadesc data, int dima , int dimb){
	if(dima >= data->n_dim || dimb >= data->n_dim || dima < 0 || dimb < 0 ){
		printf("error: dim not exist \n");
		return 0;
	}
	if(dima == dimb  ){
		printf("error : same dim \n");
		return 0;
	}
	dimdesc a = data->dims[dima];
	dimdesc b = data->dims[dimb];
	int i, j , flag = 0;
	for(i = 0 ; i < a->n_entry; i++){
		for(j = 0 ; j < b->n_entry ; j++){
			if(a->indices[i] == b->indices[j] ){
				a->offsets[i] += b->offsets[j];
				break;
			}
			else if(j == b->n_entry-1){ //没有match到
				a->offsets[i] = -1;
				a->indices[i] = -1;
			}
		}
	}
	//整理a
	for(i = 0 ; i < a->n_entry; i++){
		if(a->indices[i] == -1 || a->offsets[i] == -1){
			flag = 0;
			for(j = i+1 ; j < a->n_entry && a->indices[j] && a->offsets[j]; j++){
				if(a->indices[j] != -1  && a->offsets[j] != -1){
					a->indices[i] = a-> indices[j];
					a->offsets[i] = a->	offsets[j];
					a->indices[j] = a->	offsets[j] = -1;
					flag = 1;
					break;
				}		
			}
			if(j >= a->n_entry-1){ //考虑i=n_entry-1 的情况
				if(flag){
					a->n_entry = i+1;//i成功matrch,i之后的所有项都未match
					break;
					}
			a->n_entry = i; //i及i之后的所有项都未match
			break;
			}
		}
	}
	for(i = data->n_dim-1; i>0 ; i--){
		if(data->dims[i]){
			//找到最后的有效dim[i],break
			break;
		}
	}
	data->dims[dimb] = data->dims[i];
	data->dims[data->n_dim-1] = NULL;


	return data;
/*
	datadesc rst  = datadesc_new(data->n_dim);

	rst->region = data->region;
	rst->ref.p_direct = data->ref.p_direct;
	rst->offset0 = data->offset0;
	rst->endian = data->endian;
	rst->elem_type = data->elem_type;
	rst->elem_size = data->elem_size;

	for (i = 0; i < data->n_dim && data->dims[i]; i++){
		rst->dims[i] = dimdesc_copy(data->dims[i]);
	}
	for (; i < data->n_dim; i++) {
		rst->dims[i] = NULL;
	}
	return rst;
*/
}


void datadesc_show(datadesc p){
	int_t i, j, k, l;
	datadesc data_p;
	dimdesc dim_p;
	data_p = p;
	printf("\tdatadesc #%ld :\n", i);// FIXME: i没初始化？
	printf("offset0: %ld\n", data_p->offset0);
	for (j = 0; j < data_p->n_dim && data_p->dims[j]; j++){
		printf ("dim information: rank %ld\n", j);
		dim_p = data_p->dims[j];
		printf ("\t\tdimdesc #%ld with %lu tuples and %lu entries :\n", j, dim_p->n_tuple, dim_p->n_entry);
		printf("\t\t\tdims : \n\t\t\t\t");
		for (k = 0; k < dim_p->n_tuple; k++)
		{
			if (k>0)
				printf(", ");
			printf("%s(%ld)", dimname(dim_p->dims[k]), dim_p->dims[k]);
			//	printf("(%lu)", dim_p->dims[k]);
		}
		printf("\n");
		printf("\t\t\tindices - offsets : \n");
		for (k = 0; k < dim_p->n_entry; k++)
		{
			printf("\t\t\t\t(");
			for (l = 0; l < dim_p->n_tuple; l++)
			{
				if (l>0)
					printf(", ");
				printf("%ld", dim_p->indices[k * dim_p->n_tuple + l]); 
			}
			printf(")\t - \t%ld\n", dim_p->offsets[k]);
		}
	}
}
//dimdesc +a -b;
/*
datadesc_copy_on_write(datadesc data,int change_dim){
	datadesc rst;
	rst = datadesc_new(data->n_dim);
	rst->region = data->region;
	rst->ref.p_direct = data->p_ref;
	rst->offset0 = data->offset0;
	rst->endian = data->endian;
	rst->elem_type = data->elem_type;
	rst->elem_size = data->elem_size;

	step = 1;
	int i = 0 ; j = 0;
	for (i=0 ; i < rst->n_dim; i--)
	{
		if(i != change_dim){
			rst->dims[i] = dimdesc_copy(data->dims[i]);
		}
		rst->dims[i] = dimdesc_new(1, p_dimlength[i]);
		p->dims[i]->reduce_type = reduce_type_sum;	// FIXME: 默认为sum是否合理？
		p->dims[i]->dims[0] = dimname_new(p_dimname[i]);
		for (j=0; j<p->dims[i]->n_entry; j++)
		{
			p->dims[i]->indices[j] = j;		// NOTE: 这里实际是二维数组
			p->dims[i]->offsets[j] = j * step;
		}
		step *= p_dimlength[i];
	}

}*/
