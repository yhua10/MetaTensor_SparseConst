
#include	<stddef.h>	// for NULL

#include	"complex.c"	// FIXME: 先凑合一下，回头再整理


block	block_gamma_matrix()
{
	static	datadesc_set	res = NULL;
	static	int	const_gamma_matrix[] = {1, -1};

	if (!res)
	{
		res = datadesc_set_new(1);
		res->p[0] = datadesc_new(1);
		res->p[0]->region = region_direct;
		res->p[0]->ref.p_direct = const_gamma_matrix;
		res->p[0]->offset0 = 0;
		res->p[0]->endian = endian_local;
		res->p[0]->elem_type = elem_type_integer;
		res->p[0]->elem_size = sizeof(int);
		res->p[0]->dims[0] = dimdesc_new(4, 16);
		res->p[0]->dims[0]->reduce_type = reduce_type_sum;

		res->p[0]->dims[0]->dims[0] = dimname_new("mu");
		res->p[0]->dims[0]->dims[1] = dimname_new("d_row");
		res->p[0]->dims[0]->dims[2] = dimname_new("d_col");
		res->p[0]->dims[0]->dims[3] = dimname_new("complex");

		// QDP++/MILC/CPS使用的DeGrandRossi基
		//					entry,	offset,	mu,	d_row,	d_col,	complex
		dimdesc_set_entry(res->p[0]->dims[0],	0,	0,	0,	0,	3,	1);
		dimdesc_set_entry(res->p[0]->dims[0],	1,	0,	0,	1,	2,	1);
		dimdesc_set_entry(res->p[0]->dims[0],	2,	1,	0,	2,	1,	1);
		dimdesc_set_entry(res->p[0]->dims[0],	3,	1,	0,	3,	0,	1);

		dimdesc_set_entry(res->p[0]->dims[0],	4,	1,	1,	0,	3,	0);
		dimdesc_set_entry(res->p[0]->dims[0],	5,	0,	1,	1,	2,	0);
		dimdesc_set_entry(res->p[0]->dims[0],	6,	0,	1,	2,	1,	0);
		dimdesc_set_entry(res->p[0]->dims[0],	7,	1,	1,	3,	0,	0);

		dimdesc_set_entry(res->p[0]->dims[0],	8,	0,	2,	0,	2,	1);
		dimdesc_set_entry(res->p[0]->dims[0],	9,	1,	2,	1,	3,	1);
		dimdesc_set_entry(res->p[0]->dims[0],	10,	1,	2,	2,	0,	1);
		dimdesc_set_entry(res->p[0]->dims[0],	11,	0,	2,	3,	1,	1);

		dimdesc_set_entry(res->p[0]->dims[0],	12,	0,	3,	0,	2,	0);
		dimdesc_set_entry(res->p[0]->dims[0],	13,	0,	3,	1,	3,	0);
		dimdesc_set_entry(res->p[0]->dims[0],	14,	0,	3,	2,	0,	0);
		dimdesc_set_entry(res->p[0]->dims[0],	15,	0,	3,	3,	1,	0);
	}

	return	block_new(block_type_datadesc_set, res);
}

block	block_I4()
{
	static	datadesc_set	res = NULL;
	static	int	const_I4[] = {1};
	int	i;

	if (!res)
	{
		res = datadesc_set_new(1);
		res->p[0] = datadesc_new(1);
		res->p[0]->region = region_direct;
		res->p[0]->ref.p_direct = const_I4;
		res->p[0]->offset0 = 0;
		res->p[0]->endian = endian_local;
		res->p[0]->elem_type = elem_type_integer;
		res->p[0]->elem_size = sizeof(int);
		res->p[0]->dims[0] = dimdesc_new(3, 4);
		res->p[0]->dims[0]->reduce_type = reduce_type_sum;

		res->p[0]->dims[0]->dims[0] = dimname_new("d_row");
		res->p[0]->dims[0]->dims[1] = dimname_new("d_col");
		res->p[0]->dims[0]->dims[2] = dimname_new("complex");

		for (i=0; i<4; i++)
			dimdesc_set_entry(res->p[0]->dims[0], i, 0, i, i, 0);
	}

	return	block_new(block_type_datadesc_set, res);
}

block	block_delta(int n[4])
{
	static	int	const_delta[] = {1};
	static	char *	dims[] = {"x_row", "y_row", "z_row", "t_row", "x_col", "y_col", "z_col", "t_col"};
	datadesc_set	res;
	int		i, j;

	res = datadesc_set_new(4);
	for (i=0; i<4; i++)
	{
		res->p[i] = datadesc_new(1);
		res->p[i]->region = region_direct;
		res->p[i]->ref.p_direct = const_delta;
		res->p[i]->offset0 = 0;
		res->p[i]->endian = endian_local;
		res->p[i]->elem_type = elem_type_integer;
		res->p[i]->elem_size = sizeof(int);
		res->p[i]->dims[0] = dimdesc_new(3, n[i]);
		res->p[i]->dims[0]->reduce_type = reduce_type_sum;

		res->p[i]->dims[0]->dims[0] = dimname_new("mu");
		res->p[i]->dims[0]->dims[1] = dimname_new(dims[i]);
		res->p[i]->dims[0]->dims[2] = dimname_new(dims[4+i]);

		for (j=0; j<n[i]; j++)
			dimdesc_set_entry(res->p[i]->dims[0], j, 0, i, j, (j+1)%n[i]);
	}

	return	block_new(block_type_datadesc_set, res);
}


block	block_redim(int n, char* dimsin[], char* dimsout[])
{
	block	res;
	int	i;

	res = block_new(block_type_wire, dimname_new(dimsout[0]), dimname_new(dimsin[0]));
	for (i=1; i<n; i++)
		res = block_new(block_type_con_par_int,
				res,
				block_new(block_type_wire, dimname_new(dimsout[i]), dimname_new(dimsin[i])));

	return  res;
}

block	block_generator(int n, char* dims[])
{
	block	res;
	int	i;

	res = block_new(block_type_con_ser,
			block_new(block_type_wire, dimname_new(dims[0]), dimname_new("")),
			block_new(block_builtin_generator_all));
	for (i=1; i<n; i++)
		res = block_new(block_type_con_par_int,
				res,
				block_new(block_type_con_ser,
					block_new(block_type_wire, dimname_new(dims[i]), dimname_new("")),
					block_new(block_builtin_generator_all)));

	return  res;
}

block	block_vectorizer()
{
	static	block	res = NULL;
	static	char *	dimsin[] = {"x_row", "y_row", "z_row", "t_row", "d_row", "c_row"};
	static	char *	dimsout[] = {"x", "y", "z", "t", "d", "c"};

	if (!res)
		res = block_redim(sizeof(dimsin)/sizeof(dimsin[0]), dimsin, dimsout);

	return  res;
}

block	block_unvectorizer()
{
	static	block	res = NULL;
	static	char *	dimsin[] = {"x", "y", "z", "t", "d", "c", "x", "y", "z", "t", "d", "c"};
	static	char *	dimsout[] = {"x_row", "y_row", "z_row", "t_row", "d_row", "c_row", "x_col", "y_col", "z_col", "t_col", "d_col", "c_col"};

	if (!res)
		res = block_redim(sizeof(dimsin)/sizeof(dimsin[0]), dimsin, dimsout);

	return  res;
}

block	block_transpose()
{
	static	block	res = NULL;
	static	char *	dimsin[] = {"x_row", "y_row", "z_row", "t_row", "d_row", "c_row", "x_col", "y_col", "z_col", "t_col", "d_col", "c_col"};
	static	char *	dimsout[] = {"x_col", "y_col", "z_col", "t_col", "d_row", "c_col", "x_row", "y_row", "z_row", "t_row", "d_row", "c_row"};

	if (!res)
		res = block_redim(sizeof(dimsin)/sizeof(dimsin[0]), dimsin, dimsout);

	return  res;
}

block	do_matrix_mult(block l, block r)
{
	static	char *	dimsin[] = {"tmp_x_sum", "tmp_y_sum", "tmp_z_sum", "tmp_t_sum", "tmp_d_sum", "tmp_c_sum"};
	static	char *	dimsoutl[] = {"x_col", "y_col", "z_col", "t_col", "d_col", "c_col"};
	static	char *	dimsoutr[] = {"x_row", "y_row", "z_row", "t_row", "d_row", "c_row"};

	return	block_new(block_type_con_ser,
			block_new(block_builtin_reduce_sum),
			block_new(block_type_con_ser,
				block_new(block_type_con_ser,
					block_new(block_builtin_reduce_prod),
					block_new(block_type_con_par_int,
						block_new(block_type_con_ser,
							l,
							block_redim(sizeof(dimsin)/sizeof(dimsin[0]), dimsin, dimsoutl)),
						block_new(block_type_con_ser,
							r,
							block_redim(sizeof(dimsin)/sizeof(dimsin[0]), dimsin, dimsoutr)))),
				block_generator(sizeof(dimsin)/sizeof(dimsin[0]), dimsin)));
}


block	do_block_dslash(block U, int n[4])
{
	U			// 把xyzt指标看作是列指标
		= block_new(block_type_con_ser,
				U,
				block_vectorizer());

	block	I_p_gam		// 1 + gamma_mu
		= block_new(block_type_con_ser,
				block_new(block_builtin_reduce_sum),
				block_new(block_type_con_par_uni,
					block_I4,
					block_gamma_matrix()));

	block	I_m_gam		// 1 - gamma_mu
		= block_new(block_type_con_ser,
				block_new(block_builtin_reduce_sum),
				block_new(block_type_con_par_uni,
					block_I4,
					block_new(block_type_con_ser,
						block_new(block_builtin_unary_minus),
						block_gamma_matrix())));

	block	term_1		// (1 + gamma_mu) U_mu(x_col) delta_mu(x_col+mu, x_row)
		= do_complex_mult(I_p_gam,
				do_complex_mult(U, block_delta(n)));

	block	term_2		// (1 - gamma_mu) U^dag_mu(x_row) delta_mu(x_col-mu, x_row)
		= do_complex_mult(I_m_gam,
				do_complex_mult(
					block_new(block_type_con_ser,
						do_complex_conj(U),
						block_transpose()),
					block_new(block_type_con_ser,
						block_delta(n),
						block_transpose())));

	block	dslash		// \sum_mu (term_1 + term_2)
		= block_new(block_type_con_ser,
				block_new(block_builtin_reduce_sum),
				block_new(block_type_con_ser,
					block_new(block_type_con_par_int,
						term_1,
						term_2),
					block_new(block_type_con_ser,
						block_new(block_type_wire, dimname_new("mu"), dimname_new("")),
						block_new(block_builtin_generator_all))));

	return	dslash;
}

block	apply_dslash(block U, block phi, int n[4])
{
	block	dslash = do_block_dslash(U, n);

	phi = block_new(block_type_con_ser,
			phi,
			block_vectorizer());

	return	block_new(block_type_con_ser,
			do_matrix_mult(dslash, phi),
			block_unvectorizer());
}

block	do_example(int nx, int ny, int nz, int nt)
{
	int		n[4] = {nx, ny, nz, nt};
	char *		dimU[] = {"x", "y", "z", "t", "mu", "c_row", "c_col", "complex"};
	int_t		lenU[] = {nx, ny, nz, nt, 4, 3, 3, 2};
	char *		dimphi[] = {"x", "y", "z", "t", "d", "c", "complex"};
	int_t		lenphi[] = {nx, ny, nz, nt, 4, 3, 2};
	datadesc_set	res, U, phi;

	U = datadesc_set_new(1);
	U->p[0] = datadesc_new_array(region_main, "U", endian_local, elem_type_real, 8, 8, dimU, lenU);
	phi = datadesc_set_new(1);
	phi->p[0] = datadesc_new_array(region_main, "phi", endian_local, elem_type_real, 8, 7, dimphi, lenphi);

	return	apply_dslash(block_new(block_type_datadesc_set, U), block_new(block_type_datadesc_set, phi), n);
}



