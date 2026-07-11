#ifndef F_LOCAL_GC_COMPAT_H_
#define F_LOCAL_GC_COMPAT_H_

#include <stdlib.h>

static inline void *GC_malloc(size_t size)
{
	return malloc(size);
}

static inline void *GC_realloc(void *ptr, size_t size)
{
	return realloc(ptr, size);
}

#endif
