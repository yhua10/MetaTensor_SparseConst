#ifndef	F_GC_MALLOC_H_
#define	F_GC_MALLOC_H_

// NOTE: use The Boehm-Demers-Weiser GC Library
// find it on https://github.com/ivmai/bdwgc
// compile with -lgc flag
#include	"gc/gc.h"

#include	<stdio.h>	// FIXME: hack here if stdio is not applicable.
#define	check_null(x)	({ \
		void *p_ = (x); \
		if (p_ == NULL) fprintf(stderr, "Error : Null pointer detected in file %s at line #%d.\n", __FILE__, __LINE__); \
		p_; })

#define	malloc(x)	check_null(GC_malloc(x))
#define	calloc(n,x)	check_null(GC_malloc((n)*(x)))
#define	realloc(p__,x)	check_null(GC_realloc((p__),(x)))
#define	free(x)		do { (x) = NULL; } while (0)

#endif
