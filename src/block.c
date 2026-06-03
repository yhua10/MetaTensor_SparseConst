
#include	<string.h>

#include	"block.h"
#include	"gc_malloc.h"


block	block_new(enum_block_type t, ...)
{
	va_list args;

	// TODO: 是否使用hash或者其他方法复用block？这样的话所有block都是只读的，方便比较相等，但代价也比较明确。需要仔细权衡。
	block	res = malloc(sizeof(block_t));
	// FIXME: 这里需要清零无用成员吗？目前未赋值的成员有未定义的值。

	va_start(args, t);
	switch (res->type = t)
	{
		case block_type_box :
			res->p.u = va_arg(args, block);
			break;
		case block_type_con_ser :
		case block_type_con_par_int :
		case block_type_con_par_uni :
			res->p.b[0] = va_arg(args, block);	// 输出方向/头方向
			res->p.b[1] = va_arg(args, block);	// 输入方向/尾方向
			break;
		case block_type_datadesc_set :
			res->p.d = va_arg(args, datadesc_set);
			break;
		case block_type_wire :
			res->p.w[0] = va_arg(args, int_t);	// 输出端
			res->p.w[1] = va_arg(args, int_t);	// 输入端
			break;
		case block_type_int_t :
			res->p.n = va_arg(args, int);
			break;
		case block_type_double :
			res->p.f = va_arg(args, double);
			break;
	}
	va_end(args);

	return	res;
}




// 递归函数，打印 block 及其所有嵌套的 block
void print_block(block b, int indent) {
	
    // 打印缩进，用于表示嵌套层次
    for (int i = 0; i < indent; ++i) {
        printf("  			");
    }
    
    // 打印 block 类型
    printf("Block type: %s\n", get_block_type_name(b->type));
    
    // 根据 block 类型进行不同的处理
    switch (b->type) {
        case block_type_con_ser:
        case block_type_con_par_int:
        case block_type_con_par_uni:
            // 连接块，打印两个连接点的 block
            print_block(b->p.b[0], indent + 1); // 打印第一个连接点的 block
            print_block(b->p.b[1], indent + 1); // 打印第二个连接点的 block
            break;
        
        case block_type_datadesc_set:
            break;
        
        case block_type_wire:
            // 导线块，打印输入输出索引
            printf("Wire block with output index %ld and input index %ld\n",
                    b->p.w[0], b->p.w[1]);
            break;
        
        // ... 其他 block 类型的打印可以继续添加 ...
        
        default:
            // printf("Unknown or unhandled block type\n");
            break
            ;
    }
}

void print_data_block(block b, int indent) {
	
    // 打印缩进，用于表示嵌套层次
    for (int i = 0; i < indent; ++i) {
        printf("  			");
    }
    
    // 打印 block 类型
    printf("Block type: %s\n", get_block_type_name(b->type));
    
    // 根据 block 类型进行不同的处理
    switch (b->type) {
        case block_type_con_ser:
        case block_type_con_par_int:
        case block_type_con_par_uni:
            // 连接块，打印两个连接点的 block
            print_data_block(b->p.b[0], indent + 1); // 打印第一个连接点的 block
            print_data_block(b->p.b[1], indent + 1); // 打印第二个连接点的 block
            break;
        
        case block_type_datadesc_set:
            // 数据描述集合块，打印 datadesc_set
            printf("******************************************datadesc_set**********************************************\n");
            datadesc_set_show(b->p.d);
			printf("-----------------------------------------------finish--------------------------------------------\n");
            break;
        
        case block_type_wire:
            // 导线块，打印输入输出索引
            printf("Wire block with output index %ld and input index %ld\n",
                    b->p.w[0], b->p.w[1]);
            break;
        
        // ... 其他 block 类型的打印可以继续添加 ...
        
        default:
            // printf("Unknown or unhandled block type\n");
            break
            ;
    }
}


const char* get_block_type_name(enum_block_type type) {
    switch (type) {
        case block_type_first: return "block_type_first";
        case block_type_box: return "block_type_box";
        case block_type_con_ser: return "block_type_con_ser";
        case block_type_con_par_int: return "block_type_con_par_int";
        case block_type_con_par_uni: return "block_type_con_par_uni";
        case block_type_datadesc_set: return "block_type_datadesc_set";
        case block_type_wire: return "block_type_wire";
        case block_type_int_t: return "block_type_int_t";
        case block_type_double: return "block_type_double";
        // ... 添加其他具体类型名称
        case block_builtin_basic_first: return "block_builtin_basic_first";
        case block_builtin_con_ser: return "block_builtin_con_ser";
        case block_builtin_con_par_int: return "block_builtin_con_par_int";
        case block_builtin_con_par_uni: return "block_builtin_con_par_uni";
        case block_builtin_wire: return "block_builtin_wire";
        case block_builtin_wire_filter: return "block_builtin_wire_filter";
        case block_builtin_wire_revert: return "block_builtin_wire_revert";
        case block_builtin_generator_all: return "block_builtin_generator_all";
        // ... 添加其他内置类型名称
        case block_builtin_unary_first: return "block_builtin_unary_first";
        case block_builtin_unary_minus: return "block_builtin_unary_minus";
        // ... 添加其他一元操作类型名称
        case block_builtin_reduce_first: return "block_builtin_reduce_first";
        case block_builtin_reduce_sum: return "block_builtin_reduce_sum";
        case block_builtin_reduce_prod: return "block_builtin_reduce_prod";
        // ... 添加其他归约操作类型名称
        case block_builtin_compare_first: return "block_builtin_compare_first";
        case block_builtin_compare_equal: return "block_builtin_compare_equal";
        // ... 添加其他比较操作类型名称
        case block_builtin_last: return "block_builtin_last";
        default: return "unknown_block_type";
    }
}
