
#include "plugins.h"


int_t	datacopy_foreach(target t, datadesc_set dst, datadesc_set src)
{
	datadesc_set	d, s;
	int	i, j;

	emit_start(t);
	emit_assert(t->target_type == target_type_c_macro);
	emit_assert(dst->n>1 || src->n>1);

	emit_push("{");
	d = datadesc_set_new(1);
	s = datadesc_set_new(1);
	for (i=0; i<dst->n; i++)
		for (j=0; j<src->n; j++)
		{
			d->p[0] = dst->p[i];
			s->p[0] = src->p[j];
			emit_assert(datacopy(t, d, s));
		}
	emit_push("}");

	emit_finish();
	return	1;
}


