#include	<stdio.h>

#include	"plugins.h"
#include	"gc_malloc.h"


static int_t active_ndim(datadesc d)
{
	int_t i;

	for (i=0; i<d->n_dim && d->dims[i]; i++);
	return i;
}

static int_t data_span_elems(datadesc d)
{
	int_t i, j;
	int_t max_offset = 0;
	int_t ndim = active_ndim(d);

	for (i=0; i<ndim; i++)
	{
		int_t dim_max = 0;

		for (j=0; j<d->dims[i]->n_entry; j++)
			if (d->dims[i]->offsets[j] > dim_max)
				dim_max = d->dims[i]->offsets[j];

		max_offset += dim_max;
	}

	return max_offset + 1;
}

static double read_direct_real(datadesc d, int_t elem_offset)
{
	char *base = (char *)d->ref.p_direct + d->offset0;

	if (d->elem_size == 4)
		return *(float *)(base + elem_offset * d->elem_size);
	return *(double *)(base + elem_offset * d->elem_size);
}

static char *literal_direct_real(datadesc d, double value)
{
	char *res = malloc(80);

	if (d->elem_size == 4)
		snprintf(res, 80, "%.9ef", (float)value);
	else
		snprintf(res, 80, "%.17e", value);

	return res;
}

static int emit_direct_array(target t, datadesc d, char *type, char **array_name)
{
	int_t i;
	int_t n = data_span_elems(d);
	char *name = token_new();

	emit_start(t);

	emit_assert(d->elem_type == elem_type_real);
	emit_assert(d->elem_size == 4 || d->elem_size == 8);
	emit_push("const ", type, " ", name, "[", mt_itoa(n), "]={");
	for (i=0; i<n; i++)
	{
		if (i > 0)
			emit_push(",");
		emit_push(literal_direct_real(d, read_direct_real(d, i)));
	}
	emit_push("};");

	emit_finish();
	*array_name = name;
	return 1;
}

static	int_t	count_par_datadesc(block ob)
{
	int_t	n0, n1;

	switch (ob->type)
	{
		case block_type_datadesc_set :
			if (ob->p.d->n == 1)
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
			p[0] = ob->p.d->p[0];
			return	p + 1;

		case block_type_con_par_int :
			p = get_par_datadesc(ob->p.b[0], p);
			p = get_par_datadesc(ob->p.b[1], p);
			return	p;

		default :
			return	p;
	}
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

	type = malloc(sizeof(char *)*(nd+1));
	var = malloc(sizeof(char *)*(nd+1));
	for (i=0; i<=nd; i++)
	{
		type[i] = target_query_type(t, d->p[i]->elem_type, d->p[i]->elem_size);
		var[i] = token_new();
		emit_assert(type[i]);
	}

	d->p[nd]->region = region_main;
	d->p[nd]->ref.p_refname = token_new();
	emit_push(type[nd], "*", d->p[nd]->ref.p_refname, "=malloc(", mt_itoa(d->p[nd]->offset0), ");");
	d->p[nd]->offset0 = 0;

	emit_push("{");

	for (i=0; i<=nd; i++)
	{
		if (d->p[i]->region == region_direct)
		{
			char *array_name;

			emit_assert(i < nd);
			emit_assert(emit_direct_array(t, d->p[i], type[i], &array_name));
			emit_push(type[i], "*", var[i], "=(", type[i], "*)", array_name, ";");
		}
		else
		{
			emit_push(type[i], "*", var[i], "=(", type[i], "*)((char *)(", d->p[i]->ref.p_refname, ")+", mt_itoa(d->p[i]->offset0), ");");
		}
	}

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
			emit_assert(0);
	}

	emit_assert(emit_for_end(t, d));

	emit_push("}");

	emit_finish();

	res = datadesc_set_new(1);
	res->p[0] = d->p[nd];

	return	res;
}
