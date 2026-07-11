#ifndef	F_PLUGINS_H_
#define	F_PLUGINS_H_

#include "target_base.h"


int_t	emit_for_begin(target t, datadesc_set datas, char ***idx);
int_t	emit_for_end(target t, datadesc_set datas);



datacopy_f	datacopy_foreach;
datacopy_f	datacopy_121_reference;
datacopy_f	datacopy_121_template;
datacopy_f	datacopy_121_memcpy;


block2data_f	block2data_trivial;
block2data_f	block2data_arithmetic;
block2data_f	block2data_dense;
block2data_f	block2data_sparse_const;


optimize_f	optimize_block;
#endif
