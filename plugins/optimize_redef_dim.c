
#include	"plugins.h"
#include    "gc_malloc.h"

// TODO:	重写这个函数
//	目前需要处理这种情况：
//		树中含有导线，且导线的左侧实际接往dataset，右侧实际接往G
//			此时应当清除掉dataset和g中间的导线，再在G左侧增加串联一个导线，把dataset中对应的dim和导线的左侧一起设置为一个新的临时维度(用dimtmp_new函数，得到的维度号是负数)，导线的右侧为空。
//			需要处理好dataset、导线、G各自有可能并联的情况
//			处理完之后，树中的所有G都应当与一个导线串联绑定
//	前述的“实际接往”是相对于直接串联而言的，应当通过分析树结构和块之间的所有端口连接情况才能确认
//	具体实现方法建议：
//		1、先把所有(端口维度号不是负数的)导线都替换成两个导线的串联，中间用临时维度连接：
//			[a-b] => [a-*] - [*-b]
//		2、设计包含各种情况的公式，使形如[a*-*]的导线向左（!=向内）移动，形如[*-b]的导线向右移动（比较w[1]和0，判断移动方向）。公式举例：
//			( x - y ) - [a*-*] => 分类讨论：
//				当y是导线，不处理：
//					( x - [b*-*] ) - [a*-*]
//				( x_{a*} - {a*}_y_{?} ) - [a*-*] => 无事发生
//				( x_{a*} - {-}_y_{a*} ) - [a*-*] => ( x - [a*-*] ) - ( y - [a*-*] ) : 当且仅当x/y的输入存在a*，则需要为x/y分配[a*-*]，避免循环
//			( [a*-*] - x ) - y => [a*-*] - ( x - y )
//				FIX： 由 ( wire-G ) 引入；对x，y是否需要更严格的限定，避免死循环？
//			x - ( [a*-*] - y ) => ( x - [a*-*] ) - y
//				FIX： 由 ( wire-G ) 引入；对x，y是否需要更严格的限定，避免死循环？事实上，包含了:
//					x - ( [a*-*] - [*-b] ) => ( x - [a*-*] ) - [*-b]
//			( x & y ) - [a*-*] => ( x - [a*-*] ) & ( y - [a*-*] )
//			[b*-a*] - [a*-*] => [b*-*]
//			[*-a] - [b*-*] => [b*-*] - [*-a]
//			{维度包含a*的dataset} - [a*-*] => {维度包含*的dataset}
//			{维度不包含a*的dataset} - [a*-*] => {old_dataset}
//			……
//		TODO: 有待商榷
//			x - ( [a-*] & y) => ( x - [a-*] ) - y			// 因为*是临时名字，y不包含*端口 FIXME: 还需要y的输出不含a
//				FIX: 语义可理解，但是目前，&的两边必有dataset？
//			{无输入端口是a的块} - [a-*] => {无输入端口是a的块}
//			右移：
//			[*-b] - {无输出端口是b的块} => [*-b] & {无输出端口是b的块}	// FIXME: 还需要考虑这个块的输出端口的限制
//			[*1-b] & [*2-b] => [*1-b], 同时把树里出现的所有*2替换为*1(包括导线和dataset)	// FIXME: 这个公式是否正确？
//			……
//		3、按公式多次扫描树，直到无法发生改动为止（要注意别循环改动，维度号是负数的是特殊的临时维度）
//	处理之后，是否可以保证结果？
//		1、所有[G]应当会变成[*-]-G
//		2、除了最右面和与G绑定以外，不再有导线出现
//

// 存储剩余的导线信息
static int_t * wire_left;
static int_t * wire_right;
static int_t wire_cnt;
// 静态变量，用于生成唯一的负数索引
static int_t unique_temp_index = 0;

int_t contain_dim_output(block ob, int_t dim);

int_t contain_dim_input(block ob, int_t dim) {
	if (ob->type == block_type_datadesc_set) {
		datadesc_set dataset = ob->p.d;
		for (int_t i = 0; i < dataset->n; i++) {
			for (int_t j = 0; j < dataset->p[i]->n_dim && dataset->p[i]->dims[j] != NULL; j++) {
				for (int_t k = 0; k < dataset->p[i]->dims[j]->n_tuple; k++) {
					if (dataset->p[i]->dims[j]->dims[k] == dim) {
						return 1;
					}
				}
			}
		}
		return 0;
	}

	if (ob->type == block_type_wire) {
		return (ob->p.w[1] == dim);
	}

	if (ob->type == block_type_con_par_int) {
		return contain_dim_input(ob->p.b[0], dim) || contain_dim_input(ob->p.b[1], dim);
	}

	if (ob->type == block_type_con_ser) {
		if (contain_dim_input(ob->p.b[1], dim)) {
			return 1;
		}
		if (contain_dim_output(ob->p.b[1], dim)) {
			return 0;
		}
		return contain_dim_input(ob->p.b[0], dim);
	}

	return 0;
}

int_t contain_dim_output(block ob, int_t dim) {
	if (ob->type == block_type_datadesc_set) {
		return 0;
	}

	if (ob->type == block_type_wire) {
		return (ob->p.w[0] == dim);
	}

	if (ob->type == block_type_con_ser) {
		if (contain_dim_output(ob->p.b[0], dim)) {
			return 1;
		}
		if (contain_dim_input(ob->p.b[0], dim)) {
			return 0;
		}
		return contain_dim_output(ob->p.b[1], dim);
	}

	// FIXME: 并联 = 存在datadesc？
	if (ob->type == block_type_con_par_int) {
		return contain_dim_output(ob->p.b[0], dim) || contain_dim_output(ob->p.b[1], dim);
	}

	return 0;
}


// 分离导线块，用一个临时维度替换原始的导线块
block seprate_wire(target t, block ob) {
    //if (ob == NULL) {
      //  return ob;
    //}

    // 检查是否为导线块
    if ((ob)->type == block_type_wire) {
        // 检查端口维度号是否都不是>=0
        if (ob->p.w[0] >= 0 && ob->p.w[1] >= 0) {
            // 创建一个临时维度索引，确保每次调用时都是唯一的负数
            unique_temp_index--;
            int_t temp_index = unique_temp_index;

            // 创建新的导线块，连接原始输入到临时维度
            block wire1 = block_new(block_type_wire, ob->p.w[0], temp_index);

            // 创建新的导线块，连接临时维度到原始输出
            block wire2 = block_new(block_type_wire, temp_index, ob->p.w[1]);

            // 构建串联块，将两个新导线块串联起来
            ob = block_new(block_type_con_ser, wire1, wire2);
        }
    }

    // 如果 block 包含子 block（例如在串联或并联块中），递归处理它们
    switch (ob->type) {
        case block_type_con_ser:
        case block_type_con_par_int:
        case block_type_con_par_uni:
            ob->p.b[0] = seprate_wire(t, ob->p.b[0]); // 递归处理第一个子块
            ob->p.b[1] = seprate_wire(t, ob->p.b[1]); // 递归处理第二个子块
            break;
        // 如果有其他包含子 block 的类型，在这里添加相应的处理
        // ...
        default:
            break;
    }
    return ob;
}

block	optimize_redef_dim_pattern(target t, block ob, int_t* flag)
{
	int	i, j, k;

	// FIX: 各pattern目前并无重合，但是允许重合？如果有交叉部分，注意考量pattern判别的顺序

	if (ob->p.b[0]->type == block_type_datadesc_set && ob->p.b[1]->type == block_type_wire && ob->p.b[1]->p.w[1] < 0)
	{
		//	{维度包含a*的dataset} - [a*-*] => {维度包含*的dataset}
		//	{维度不包含a*的dataset} - [a*-*] => {old_dataset}
		for (i=0; i<ob->p.b[0]->p.d->n; i++)
		// for dim i
			for (j=0; j<ob->p.b[0]->p.d->p[i]->n_dim && ob->p.b[0]->p.d->p[i]->dims[j]; j++)
			// for tuple
				for (k=0; k<ob->p.b[0]->p.d->p[i]->dims[j]->n_tuple; k++)
				// 导线，0为输出名字（左），1为输入名字（右）
					if (ob->p.b[0]->p.d->p[i]->dims[j]->dims[k] == ob->p.b[1]->p.w[0])
					{
						ob->p.b[0]->p.d->p[i]->dims[j]->dims[k] = ob->p.b[1]->p.w[1];		// NOTE: 此处修改了dimdesc_t的内容 
						//修改后应判断是否有重复维度！即是否有其他dim = p.w[1]；若有则合并
						//这里需要修改吗？
						for(int_t n = 0 ; n < ob->p.b[0]->p.d->p[i]->n_dim && ob->p.b[0]->p.d->p[i]->dims[n] ; n++){
										if(ob->p.b[0]->p.d->p[i]->dims[j]->dims[n] ==  ob->p.b[0]->p.d->p[i]->dims[j]->dims[k] && n!=k ){
											datadesc_merge_dimb2a(ob->p.d->p[i],n,k);
										} 	
									}
					}
		// FIX：两种情况，都需要将flag置1
		*flag = 1;
		return ob->p.b[0];		
	}

	if (ob->p.b[0]->type == block_type_wire && ob->p.b[1]->type == block_type_wire) {
		if (ob->p.b[0]->p.w[1] == ob->p.b[1]->p.w[0]) {
			// [b*-a*] - [a*-*] => [b*-*]
			if (ob->p.b[1]->p.w[1] < 0) {
				ob->p.b[0]->p.w[1] = ob->p.b[1]->p.w[1];
				*flag = 1;
				return ob->p.b[0];
			}
		} else {
			// [*-a] - [b*-*] => [b*-*] - [*-a]
			if (ob->p.b[0]->p.w[1] > 0 && ob->p.b[1]->p.w[1] < 0) {
				block wire_a = ob->p.b[0];
				ob->p.b[0] = ob->p.b[1];
				ob->p.b[1] = wire_a;
				*flag = 1;
				return ob;
			}
		}
	}

	if (ob->p.b[0]->type == block_type_con_ser && ob->p.b[0]->p.b[0]->type == block_type_wire && ob->p.b[0]->p.b[0]->p.w[1] < 0) {
		// ( [a*-*] - x ) - y => [a*-*] - ( x - y )
		*flag = 1;
		return block_new(block_type_con_ser, 
					ob->p.b[0]->p.b[0],
					block_new(block_type_con_ser, 
						ob->p.b[0]->p.b[1], 
						ob->p.b[1]));
	}

	if (ob->p.b[1]->type == block_type_con_ser && ob->p.b[1]->p.b[0]->type == block_type_wire && ob->p.b[1]->p.b[0]->p.w[1] < 0) {
		// x - ( [a*-*] - y ) => ( x - [a*-*] ) - y
		*flag = 1;
		return block_new(block_type_con_ser, 
					block_new(block_type_con_ser, 
						ob->p.b[0], 
						ob->p.b[1]->p.b[0]), 
					ob->p.b[1]->p.b[1]);
	}

	if (ob->p.b[0]->type == block_type_con_par_int && ob->p.b[1]->type == block_type_wire && ob->p.b[1]->p.w[1] < 0) {
		//	( x & y ) - [a*-*] => ( x - [a*-*] ) & ( y - [a*-*] )
		block x_wire = block_new(block_type_con_ser, ob->p.b[0]->p.b[0], ob->p.b[1]);
		block y_wire = block_new(block_type_con_ser, ob->p.b[0]->p.b[1], ob->p.b[1]);
		block par_int = block_new(block_type_con_par_int, x_wire, y_wire);
		*flag = 1;
		return par_int;
	}

	if (ob->p.b[0]->type == block_type_con_ser && ob->p.b[1]->type == block_type_wire && ob->p.b[1]->p.w[1] < 0) {
		//	( x - y ) - [a*-*] => 分类讨论：

		if (ob->p.b[0]->p.b[1]->type == block_type_wire && ob->p.b[0]->p.b[1]->p.w[1] < 0) {
			//		check: 当y是导线，不处理
			//			( x - [b*-*] ) - [a*-*]
			
			return ob;

		} else {

			int_t dim = ob->p.b[1]->p.w[0];
			int_t y_input = contain_dim_input(ob->p.b[0]->p.b[1], dim);
			int_t x_input = contain_dim_input(ob->p.b[0]->p.b[0], dim);
			int_t y_output = contain_dim_output(ob->p.b[0]->p.b[1], dim);

			if (y_output && x_input) {
				//	( x_{a*} - {a*}_y_{?} ) - [a*-*] => 无事发生
				return ob;
			}

			//	( x_{a*} - {-}_y_{a*} ) - [a*-*] => ( x - [a*-*] ) - ( y - [a*-*] ) : 当且仅当x/y的输入存在a*，则需要为x/y分配[a*-*]；注意，避免循环
			
			block x_new, y_new;

			if (x_input) {
				x_new = block_new(block_type_con_ser, 
							ob->p.b[0]->p.b[0], 
							ob->p.b[1]);
			} else {
				x_new = ob->p.b[0]->p.b[0];
			}

			if (y_input) {
				y_new = block_new(block_type_con_ser, 
							ob->p.b[0]->p.b[1], 
							ob->p.b[1]);
			} else {
				y_new = ob->p.b[0]->p.b[1];
			}

			*flag = 1;
			return block_new(block_type_con_ser, 
						x_new, 
						y_new);
		
		}
	}
		

	return	ob;
}

block	optimize_redef_dim_recur(target t, block ob, int_t* flag) 
{
	if (ob->type == block_type_con_ser) {
		// 处理后，ob有可能不再是串连
		ob = optimize_redef_dim_pattern(t, ob, flag);
	}

	if (ob->type == block_type_con_ser || ob->type == block_type_con_par_int) {
		ob->p.b[0] = optimize_redef_dim_recur(t, ob->p.b[0], flag);
		ob->p.b[1] = optimize_redef_dim_recur(t, ob->p.b[1], flag);
	}

	return ob;
}

block	optimize_redef_dim_remove_wire(target t, block ob, int_t *flag) {
	// FIXME: ob->p.b[1]->p.w[1] >= 0，避免G前的wire被处理
	if (ob->p.b[1]->type == block_type_wire && ob->p.b[1]->p.w[1] >= 0) {
		wire_left[wire_cnt] = ob->p.b[1]->p.w[0];
		wire_right[wire_cnt++] = ob->p.b[1]->p.w[1];
		*flag = 1;
		return ob->p.b[0];
	}
	return ob;
}

block	optimize_redef_dim_remove(target t, block ob, int_t *flag) {
	if (ob->type == block_type_con_ser) {
		ob = optimize_redef_dim_remove_wire(t, ob, flag);
	}

	if (ob->type == block_type_con_ser || ob->type == block_type_con_par_int) {
		ob->p.b[0] = optimize_redef_dim_remove(t, ob->p.b[0], flag);
		ob->p.b[1] = optimize_redef_dim_remove(t, ob->p.b[1], flag);
	}

	return ob;
}

block	optimize_redef_dim_rename(target t, block ob) {
	switch (ob->type)
	{
		case block_type_datadesc_set:
		{
			for (int_t i = 0; i < ob->p.d->n; i++) {
				for (int_t j = 0; j < ob->p.d->p[i]->n_dim && ob->p.d->p[i]->dims[j]; j++) {
					for (int_t k = 0; k < ob->p.d->p[i]->dims[j]->n_tuple; k++) {
						int_t dim_index = ob->p.d->p[i]->dims[j]->dims[k];
						if (dim_index < 0) {
							for (int_t m = 0; m < wire_cnt; m++) {
						
								if (wire_left[m] == dim_index) {
									ob->p.d->p[i]->dims[j]->dims[k] = wire_right[m];
									// 对于datadesc的每个临时维度，最多被rename改名一次
									// 改为可以多次rename;rename后再merge
									break;
								}
								
							}

						}
					}
				}
				//下面进行merge操作
				//TODO: 加上for t 进行稀疏判断
				int_t l = 0, n = 0;
				for(n = 0 ; n < ob->p.d->p[i]->n_dim && ob->p.d->p[i]->dims[n];n++){
					for(l = n+1 ; l < ob->p.d->p[i]->n_dim && ob->p.d->p[i]->dims[l] ; l++){
						if(ob->p.d->p[i]->dims[n]->dims[0] ==  ob->p.d->p[i]->dims[l]->dims[0]){
							//判断是否需要修剪dim
							//ob->p.d->p[i] = datadesc_merge_dimb2a(ob->p.d->p[i],n,l);
							ob->p.d->p[i] = datadesc_merge_dimb2a(ob->p.d->p[i],n,l);
						}
					} 	
				}
			}
			break;
		}

		case block_type_con_ser:
		case block_type_con_par_int:
		{
			ob->p.b[0] = optimize_redef_dim_rename(t, ob->p.b[0]);
			ob->p.b[1] = optimize_redef_dim_rename(t, ob->p.b[1]);
			break;
		}

		default:
			break;
	}

	return ob;
}

block	optimize_redef_dim(target t, block ob) 
{
	// FIXME: 不重置临时维度计数
	// unique_temp_index = -1;
	ob = seprate_wire(t, ob);
	
	int_t* flag = (int_t *)malloc(sizeof(int_t));
	while (1) {
		*flag = 0;
		ob = optimize_redef_dim_recur(t, ob, flag);
		if (*flag == 0) break;
	}

	// FIXME: 空间分配或许可以更精准
	wire_left = (int_t *)malloc(sizeof(int_t) * (0 - unique_temp_index));
	wire_right = (int_t *)malloc(sizeof(int_t) * (0 - unique_temp_index));
	wire_cnt = 0;
	// 回收[*-a]
	// FIXME: 或许不需要迭代
	while (1) {
		*flag = 0;
		ob = optimize_redef_dim_remove(t, ob, flag);
		if (*flag == 0) break;
	}

	// [*-a]改名下发
	ob = optimize_redef_dim_rename(t, ob);
	return ob;
}


