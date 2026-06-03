
#include	<string.h>

#include	"datadesc.h"
#include	"plugins.h"
#include	"gc_malloc.h"


int_t	emit_for_begin(target t, datadesc_set datas, char ***idx)
{
	datadesc_set	ds;
	int_t		ndim;
	int_t *	dims;
	int_t *	nind;
	int_t **	indices;
	char **		offset;
	char *		t_offset_i;
	char *		t_tmp;
	char *		int_type;
	int_t		i, j, k, l, m;

	emit_start(t);

	int_type = target_query_type(t, elem_type_integer, 4);	// NOTE: 循环变量和偏移量的类型，目前默认是 32bit signed integer
	emit_assert(int_type);

	ds = datadesc_find_common(datas);

	for (ndim=0, i=0; i<ds->n; i++)
	{
		ds->p[i] = datadesc_normalize(ds->p[i]);
		ndim += ds->p[i]->n_dim;
	}
	//datadesc_set_show(ds);
	dims = malloc(sizeof(int_t)*ndim);		// FIXME: 这里先不free，考虑可以用自动GC
	nind = malloc(sizeof(int_t)*ndim);
	indices = malloc(sizeof(int_t*)*ndim);
	for (ndim=0, i=0; i<ds->n; i++)
		for (j=0; j<ds->p[i]->n_dim && ds->p[i]->dims[j]; j++)
		{
			emit_assert(ds->p[i]->dims[j]->n_entry>0);
			emit_assert(ds->p[i]->dims[j]->n_tuple==1);		// TODO: 这里没考虑稀疏情况，包括后面的很多代码也没有考虑稀疏情况
			for (k=0; k < ndim && dims[k] != ds->p[i]->dims[j]->dims[0]; k++);
			if (k==ndim)
			{
				dims[k] = ds->p[i]->dims[j]->dims[0];
				nind[k] = ds->p[i]->dims[j]->n_entry;
				indices[k] = malloc(sizeof(int_t)*nind[k]);
				for (l=0; l<nind[k]; l++)
					indices[k][l] = ds->p[i]->dims[j]->indices[l*1+0];
				ndim ++;
			}
		}

	*idx = malloc(sizeof(char *)*ds->n);		// NOTE: 这个是要返回的，所以不可以在这里free
	offset = malloc(sizeof(char *)*ds->n);		// FIXME: 这个需要被free
	for (i=0; i<ds->n; i++)
	{
		(*idx)[i] = token_new();
		emit_push(int_type, " ", (*idx)[i], "=0;");
		offset[i] = NULL;
	}

	for (i=0; i<ndim; i++)
	{
		for (j=0; j<ds->n; j++)
		{
			for (k=0; k < ds->p[j]->n_dim && ds->p[j]->dims[k] && dims[i] != ds->p[j]->dims[k]->dims[0]; k++);
			if (k<ds->p[j]->n_dim && ds->p[j]->dims[k])
			{
				offset[j] = token_new();
				emit_push("const ", int_type, " ", offset[j], "[", itoa(nind[i]), "]={");
				for (l=0; l<nind[i]; l++)
				{
					if (l>0)
						emit_push(",");
					for (m=0; m < ds->p[j]->dims[k]->n_entry && indices[i][l] != ds->p[j]->dims[k]->indices[m*1+0]; m++);
					//printf("(%ld(%s),%ld,%ld,%ld(%ld),%ld)\n",i, dimname(dims[i]),j,k,l, indices[i][l],m);fflush(stdout);
					emit_assert(m<ds->p[j]->dims[k]->n_entry);	// FIXME: 这个似乎不应该出问题
					emit_push(itoa(ds->p[j]->dims[k]->offsets[m]));
				}
				emit_push("};");
			}
		}
		t_offset_i = token_new();
		emit_push("for(", int_type, " ", t_offset_i, "=0;", t_offset_i, "<", itoa(nind[i]), ";", t_offset_i, "++){");
		for (j=0; j<ds->n; j++)
			if (offset[j])
			{
				t_tmp = token_new();
				emit_push(int_type, " ", t_tmp, "=", (*idx)[j], "+", offset[j], "[", t_offset_i, "];");
				(*idx)[j] = t_tmp;
				offset[j] = NULL;
			}
	}

	emit_finish();

	return	1;
}

int_t	emit_for_end(target t, datadesc_set ds)
{
	int_t *	dims;
	int_t		ndim;
	int_t		i, j, k;

	emit_start(t);

	for (ndim=0, i=0; i<ds->n; i++)
		ndim += ds->p[i]->n_dim;
	dims = malloc(sizeof(int_t)*ndim);		// FIXME: 这里先不free，考虑可以用自动GC
	for (ndim=0, i=0; i<ds->n; i++)
		for (j=0; j<ds->p[i]->n_dim && ds->p[i]->dims[j]; j++)
		{
			emit_assert(ds->p[i]->dims[j]->n_tuple==1);		// TODO: 这里没考虑稀疏情况，包括后面的很多代码也没有考虑稀疏情况
			for (k=0; k < ndim && dims[k] != ds->p[i]->dims[j]->dims[0]; k++);
			if (k==ndim)
			{
				dims[k] = ds->p[i]->dims[j]->dims[0];
				ndim ++;
				emit_push("}");
			}
		}

	emit_finish();

	return	1;
}


