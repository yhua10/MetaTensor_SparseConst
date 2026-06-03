
#include	<stdio.h>
#include	<string.h>

#include	"plugins.h"
#include	"target_c99.h"


void	main()
{
	target		t;

	datadesc	a, b;
	char *		dima[] = {"x", "y", "z"};
	char *		dimb[] = {"z", "x", "t"};
	uint_t		lengtha[] = {2, 3, 5};
	uint_t		lengthb[] = {5, 2, 7};
	datadesc_set	sa, sb;

	int_t		res;

	FILE	*fp;
	char	*shead="#include <stdint.h>\nvoid copy(void *dst , void *src)";


	a = datadesc_new_array(region_main, "dst", endian_little, elem_type_real, 8, 3, dima, lengtha);
	b = datadesc_new_array(region_main, "src", endian_little, elem_type_real, 8, 3, dimb, lengthb);
	sa = datadesc_set_new(1);
	sa->p[0] = a;
	sb = datadesc_set_new(1);
	sb->p[0] = b;

	t = target_c99_new();
	res = datacopy_121_reference(t, sa, sb);
	printf("return value = %ld.\n", res);

	fp = fopen("dump.c", "wt");
	fwrite(shead, strlen(shead), 1, fp);
	target_c99_dump(t, fp);
	fclose(fp);

}

