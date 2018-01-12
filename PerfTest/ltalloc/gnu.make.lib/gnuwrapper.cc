#ifndef __GNUC__
#error
#endif

#include "../ltalloc.h"
#include "../ltalloc.cc"
#include <malloc.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

void *malloc(size_t size)
{
	return ltmalloc(size);
}

void free(void *ptr)
{
	return ltfree(ptr);
}

void *calloc(size_t n, size_t esize)
{
	size_t size = n * esize;
	assert(esize == 0 || size / esize == n);//overflow check
	void *result = ltmalloc(size);
	if (result && size <= MAX_BLOCK_SIZE)//memory obtained directly from the system are already zero filled
		memset(result, 0, size);
	return result;
}

void cfree(void *ptr)
{
	return ltfree(ptr);
}

void *realloc(void *ptr, size_t size)
{
	if (ptr) {
		size_t uSize = ltmsize(ptr);
		if (size <= uSize)//have nothing to do
			return ptr;
		void *newp = ltmalloc(size);
		memcpy(newp, ptr, uSize);
		ltfree(ptr);
		return newp;
	}
	else
		return ltmalloc(size);
}

void *memalign(size_t alignment, size_t size)
{
	assert(!(alignment & (alignment-1)) && "alignment must be a power of two");
	return ltmalloc((size + (alignment-1)) & ~(alignment-1));
}

int posix_memalign(void **memptr, size_t alignment, size_t size)
{
	if ((alignment % sizeof(void*)) || (alignment & (alignment-1)) || alignment == 0)
		return EINVAL;
	void *ptr = memalign(alignment, size);
    if (!ptr)
		return ENOMEM;
	*memptr = ptr;
	return 0;
}

void *aligned_alloc(size_t alignment, size_t size)
{
	assert(!(alignment & (alignment-1)) && !(size & (alignment-1)));
	return ltmalloc(size);
}

void *valloc(size_t size)
{
	return memalign(page_size(), size);
}

void *pvalloc(size_t size)
{
	return memalign(page_size(), size);
}

size_t malloc_usable_size(void *ptr)
{
	return ltmsize(ptr);
}

int malloc_trim(size_t pad)
{
	ltsqueeze(pad);
	return 0;
}

struct mallinfo mallinfo(void)
{
	struct mallinfo m = {0};
	return m;
}

int mallopt(int param, int value)
{
	(void)param;
	(void)value;
	return 1;
}

void malloc_stats(void)
{
}

void *malloc_get_state(void)
{
	return NULL;
}

int malloc_set_state(void *state)
{
	(void)state;
	return 0;
}

#ifdef __cplusplus
} //for extern "C"
#endif
