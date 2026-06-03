
#include	"plugins.h"
#include	"gc_malloc.h"


static	int_t	count_par_datadesc(block ob)
{
	int_t	n0, n1;

	switch (ob->type)
	{
		case block_type_datadesc_set :
			if (ob->p.d->n == 1)		// TODO: 这里先实现只包含一个datadesc的情况，其他情况待增补
				return	1;
			else
				return	0;

		case block_type_con_par_int :
			if (!(n0 = count_par_datadesc(ob->p.b[0])))
				return	0;
			if (!(n1 = count_par_datadesc(ob->p.b[1])))
				return	0;
			return	n0 + n1;

		default :
			return	0;
	}
}

static	datadesc *	get_par_datadesc(block ob, datadesc *p)
{
	switch (ob->type)
	{
		case block_type_datadesc_set :
			p[0] = ob->p.d->p[0];		// TODO: 这里先实现只包含一个datadesc的情况，其他情况待增补
			return	p + 1;

		case block_type_con_par_int :
			p = get_par_datadesc(ob->p.b[0], p);
			p = get_par_datadesc(ob->p.b[1], p);
			return	p;
	}
	// NOTE: 不该运行到这里
}


datadesc_set	block2data_arithmetic(target t, block ob)
{
	datadesc_set	d;
	datadesc_set	res;
	char **		type;
	char **		var;
	char **		idx;
	int_t		nd;
	int_t		i;

	emit_start(t);

	emit_assert(ob->type == block_type_con_ser && ob->p.b[0]->type > block_builtin_reduce_first && ob->p.b[0]->type < block_builtin_reduce_last);

	nd = count_par_datadesc(ob->p.b[1]);
	emit_assert(nd);
	d = datadesc_set_new(nd+1);
	get_par_datadesc(ob->p.b[1], d->p);
	d->n --;
	d->p[d->n] = datadesc_find_span(d);
	d->n ++;
	d = datadesc_find_common(d);

	type = malloc(sizeof(char *)*(nd+1));		// FIXME: 先不管free的事，回头一起处理。
	var = malloc(sizeof(char *)*(nd+1));
	for (i=0; i<=nd; i++)
	{
		type[i] = target_query_type(t, d->p[i]->elem_type, d->p[i]->elem_size);
		var[i] = token_new();
	}

	d->p[nd]->region = region_main;			// NOTE: 默认在默认内存区域
	d->p[nd]->ref.p_refname = token_new();
	emit_push(type[nd], "*", d->p[nd]->ref.p_refname, "=malloc(", itoa(d->p[nd]->offset0), ");");
	d->p[nd]->offset0 = 0;

	emit_push("{");

	for (i=0; i<=nd; i++)
		emit_push(type[i], "*", var[i], "=(", type[i], "*)((char *)(", d->p[i]->ref.p_refname, ")+", itoa(d->p[i]->offset0), ");");

	emit_assert(emit_for_begin(t, d, &idx));

	switch (ob->p.b[0]->type)
	{
		case block_builtin_reduce_sum :
			emit_push(var[nd], "[", idx[nd],"]", "=0");
			for (i=0; i<nd; i++)
				emit_push("+", var[i],"[", idx[i],"]");
			emit_push(";");
			break;

		case block_builtin_reduce_prod :
			emit_push(var[nd], "[", idx[nd],"]", "=1");
			for (i=0; i<nd; i++)
				emit_push("*", var[i],"[", idx[i],"]");
			emit_push(";");
			break;

		default :
			emit_assert(0);		// TODO: 这里可以增补其他归约操作
	}

	emit_assert(emit_for_end(t, d));

	emit_push("}");

	emit_finish();

	res = datadesc_set_new(1);
	res->p[0] = d->p[nd];

	return	res;
}


