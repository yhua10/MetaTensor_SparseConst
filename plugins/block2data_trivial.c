
#include	"plugins.h"


datadesc_set	block2data_trivial(target t, block ob)
{
	emit_start(t);
	emit_assert(ob->type == block_type_datadesc_set);
	emit_finish();

	return	ob->p.d;
}


