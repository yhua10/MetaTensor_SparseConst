
#include	<stdio.h>
#include	<string.h>

#include	"plugins.h"
#include	"target_c99.h"


void	main()
{
	target		t;

	datadesc	a, b, c, d, e;			//	2	3	5	7	11	13
	char *		dima[] = {"x", "y", "z"};	//	x	y	z
	char *		dimb[] = {"z", "x", "t"};	//	x		z	t
	char *		dimc[] = {"u", "y", "t", "x"};	//	x	y		t	u
	char *		dimd[] = {"x"};			//	x
	char *		dime[] = {"v"};			//						v
	uint_t		lengtha[] = {2, 3, 5};
	uint_t		lengthb[] = {5, 2, 7};
	uint_t		lengthc[] = {11, 3, 7, 2};
	uint_t		lengthd[] = {2};
	uint_t		lengthe[] = {13};
	datadesc_set	dst, src;

	int_t		res;

	FILE	*fp;
	char	*shead="#include <stdint.h>\nvoid copy(void *a, void *b, void *c, void *d, void *e)";


	a = datadesc_new_array(region_main, "a", endian_little, elem_type_real, 8, 3, dima, lengtha);
	b = datadesc_new_array(region_main, "b", endian_little, elem_type_real, 4, 3, dimb, lengthb);
	c = datadesc_new_array(region_main, "c", endian_little, elem_type_real, 8, 4, dimc, lengthc);
	d = datadesc_new_array(region_main, "d", endian_little, elem_type_real, 8, 1, dimd, lengthd);
	e = datadesc_new_array(region_main, "e", endian_little, elem_type_real, 4, 1, dime, lengthe);
	dst = datadesc_set_new(3);
	dst->p[0] = a;
	dst->p[1] = c;
	dst->p[2] = d;
	src = datadesc_set_new(2);
	src->p[0] = b;
	src->p[1] = e;

	t = target_c99_new();
	res = datacopy(t, dst, src);
	printf("return value = %ld.\n", res);

	fp = fopen("dump.c", "wt");
	fwrite(shead, strlen(shead), 1, fp);
	target_c99_dump(t, fp);
	fclose(fp);

}

