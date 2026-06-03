
#include	"plugins.h"


block	optimize_recursion(target t, block ob)
{
	block	p0, p1;

	switch (ob->type)
	{
		case block_type_box :
			p0 = optimize(t, ob->p.u);
			if (p0 != ob->p.u)
				return	optimize(t, block_new(ob->type, p0));		// NOTE: 此处故意保持原block只读，为了不给未来并行优化惹麻烦
			break;
		case block_type_con_ser :
		case block_type_con_par_int :
		case block_type_con_par_uni :
			p0 = optimize(t, ob->p.b[0]);
			p1 = optimize(t, ob->p.b[1]);
			if (p0 != ob->p.b[0] || p1 != ob->p.b[1])
				return	optimize(t, block_new(ob->type, p0, p1));
			break;
	}

	return ob;
}


