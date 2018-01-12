#include <atomic>
#include <stdint.h>


std::atomic<uint32_t> canStart;

struct Result
{
	uint64_t opCount;
	uint64_t timeMs;
};


#include "smmalloc.h"
#ifdef SMMALLOC_STATS_SUPPORT
void printDebug(sm_allocator heap)
{
	size_t globalMissesCount = heap->GetGlobalMissCount();
	printf("Global Misses : %zu\n", globalMissesCount);

	size_t bucketsCount = heap->GetBucketsCount();
	for (size_t bucketIndex = 0; bucketIndex < bucketsCount; bucketIndex++)
	{
		uint32_t elementsCount = heap->GetBucketElementsCount(bucketIndex);
		uint32_t elementsSize = heap->GetBucketElementSize(bucketIndex);
		printf("Bucket[%zu], Elements[%d], SizeOf[%d] -----\n", bucketIndex, elementsCount, elementsSize);
		const sm::AllocatorStats* stats = heap->GetBucketStats(bucketIndex);
		if (!stats)
		{
			continue;
		}

		printf("    Cache Hits : %zu\n", stats->cacheHitCount.load());
		printf("    Hits : %zu\n", stats->hitCount.load());
		printf("    Misses : %zu\n", stats->missCount.load());
		printf("    Operations : %zu\n", stats->cacheHitCount.load() + stats->hitCount.load() + stats->missCount.load());

	}
}
#else
void printDebug(sm_allocator heap)
{}
#endif


#define ALLOCATOR_TEST_NAME smmalloc
#define HEAP sm_allocator
#define CREATE_HEAP _sm_allocator_create(5, (48 * 1024 * 1024))
#define DESTROY_HEAP printDebug(heap); _sm_allocator_destroy(heap)
#define ON_THREAD_START _sm_allocator_thread_cache_create(heap, sm::CACHE_WARM, { 16384, 131072, 131072, 131072, 131072 })
#define ON_THREAD_FINISHED _sm_allocator_thread_cache_destroy(heap)
#define MALLOC(size, align) _sm_malloc(heap, size, align)
#define FREE(p) _sm_free(heap, p)
#include "PerfTestImpl.h"
#undef ALLOCATOR_TEST_NAME
#undef HEAP
#undef CREATE_HEAP
#undef DESTROY_HEAP
#undef ON_THREAD_START
#undef ON_THREAD_FINISHED
#undef MALLOC
#undef FREE

#define ALLOCATOR_TEST_NAME smmalloc_notc
#define HEAP sm_allocator
#define CREATE_HEAP _sm_allocator_create(5, (48 * 1024 * 1024))
#define DESTROY_HEAP printDebug(heap); _sm_allocator_destroy(heap)
#define ON_THREAD_START
#define ON_THREAD_FINISHED 
#define MALLOC(size, align) _sm_malloc(heap, size, align)
#define FREE(p) _sm_free(heap, p)
#include "PerfTestImpl.h"
#undef ALLOCATOR_TEST_NAME
#undef HEAP
#undef CREATE_HEAP
#undef DESTROY_HEAP
#undef ON_THREAD_START
#undef ON_THREAD_FINISHED
#undef MALLOC
#undef FREE



#define ALLOCATOR_TEST_NAME crt
#define HEAP void*
#define CREATE_HEAP nullptr
#define DESTROY_HEAP 
#define ON_THREAD_START 
#define ON_THREAD_FINISHED 
#define MALLOC(size, align) _mm_malloc(size, align)
#define FREE(p) _mm_free(p)
#include "PerfTestImpl.h"
#undef ALLOCATOR_TEST_NAME
#undef HEAP
#undef CREATE_HEAP
#undef DESTROY_HEAP
#undef ON_THREAD_START
#undef ON_THREAD_FINISHED
#undef MALLOC
#undef FREE


#include "rpmalloc\rpmalloc.h"
#define ALLOCATOR_TEST_NAME rpmalloc
#define HEAP int
#define CREATE_HEAP rpmalloc_initialize()
#define DESTROY_HEAP rpmalloc_finalize()
#define ON_THREAD_START rpmalloc_thread_initialize()
#define ON_THREAD_FINISHED rpmalloc_thread_finalize()
#define MALLOC(size, align) rpmemalign(align, size)
#define FREE(p) rpfree(p)
#include "PerfTestImpl.h"
#undef ALLOCATOR_TEST_NAME
#undef HEAP
#undef CREATE_HEAP
#undef DESTROY_HEAP
#undef ON_THREAD_START
#undef ON_THREAD_FINISHED
#undef MALLOC
#undef FREE



#ifdef _WIN32
#include <malloc.h>
#else
#include <memory.h>
#endif

extern "C"
{
	extern void hoardInitialize(void);
	extern void hoardFinalize(void);
	extern void hoardThreadInitialize(void);
	extern void hoardThreadFinalize(void);
	extern void* xxmalloc(size_t sz);
	extern void xxfree(void * ptr);
}


#define ALLOCATOR_TEST_NAME hoard
#define HEAP int
#define CREATE_HEAP 0; hoardInitialize()
#define DESTROY_HEAP hoardFinalize()
#define ON_THREAD_START hoardThreadInitialize()
#define ON_THREAD_FINISHED hoardThreadFinalize()
#define MALLOC(size, align) xxmalloc(/*align,*/ size)
#define FREE(p) xxfree(p)
#include "PerfTestImpl.h"
#undef ALLOCATOR_TEST_NAME
#undef HEAP
#undef CREATE_HEAP
#undef DESTROY_HEAP
#undef ON_THREAD_START
#undef ON_THREAD_FINISHED
#undef MALLOC
#undef FREE



extern "C"
{
	int dlmalloc_trim(size_t pad);
	void* dlmalloc(size_t);
	void* dlmemalign(size_t, size_t);
	void* dlrealloc(void*, size_t);
	void  dlfree(void*);
	size_t dlmalloc_usable_size(void* mem);
}

#include <mutex>
std::mutex globalMutex;

void* dl_malloc(size_t v)
{
	std::lock_guard<std::mutex> lock(globalMutex);
	return dlmalloc(v);
}

void dl_free(void* p)
{
	std::lock_guard<std::mutex> lock(globalMutex);
	dlfree(p);
}



#define ALLOCATOR_TEST_NAME dlmalloc
#define HEAP int
#define CREATE_HEAP 0
#define DESTROY_HEAP 
#define ON_THREAD_START 
#define ON_THREAD_FINISHED 
#define MALLOC(size, align) dl_malloc(/*align,*/ size)
#define FREE(p) dl_free(p)
#include "PerfTestImpl.h"
#undef ALLOCATOR_TEST_NAME
#undef HEAP
#undef CREATE_HEAP
#undef DESTROY_HEAP
#undef ON_THREAD_START
#undef ON_THREAD_FINISHED
#undef MALLOC
#undef FREE





#define ALLOCATOR_TEST_NAME dlmalloc_st
#define HEAP int
#define CREATE_HEAP 0
#define DESTROY_HEAP 
#define ON_THREAD_START 
#define ON_THREAD_FINISHED 
#define MALLOC(size, align) dlmalloc(/*align,*/ size)
#define FREE(p) dlfree(p)
#include "PerfTestImpl.h"
#undef ALLOCATOR_TEST_NAME
#undef HEAP
#undef CREATE_HEAP
#undef DESTROY_HEAP
#undef ON_THREAD_START
#undef ON_THREAD_FINISHED
#undef MALLOC
#undef FREE


void*  ltmalloc(size_t);
void   ltfree(void*);
void*  ltrealloc(void *, size_t);
void*  ltcalloc(size_t, size_t);
void*  ltmemalign(size_t, size_t);
void   ltsqueeze(size_t pad); /*return memory to system (see README.md)*/


#define ALLOCATOR_TEST_NAME ltalloc
#define HEAP void*
#define CREATE_HEAP nullptr
#define DESTROY_HEAP
#define ON_THREAD_START 
#define ON_THREAD_FINISHED 
#define MALLOC(size, align) ltmemalign(align, size)
#define FREE(p) ltfree(p)
#include "PerfTestImpl.h"
#undef ALLOCATOR_TEST_NAME
#undef HEAP
#undef CREATE_HEAP
#undef DESTROY_HEAP
#undef ON_THREAD_START
#undef ON_THREAD_FINISHED
#undef MALLOC
#undef FREE


int main(int argc, char ** argv)
{


	srand(1306);

	std::vector<size_t> randomSequence;
	randomSequence.resize(1024 * 1024);

	for (size_t i = 0; i < randomSequence.size(); i++)
	{
		//16 - 80 bytes
		size_t sz = 16 + (rand() % 64);
		randomSequence[i] = sz;
	}

	printf("name;thread num;min;max;avg\n");

	uint32_t threadsCount = 5;

	DoTest_crt(threadsCount, &randomSequence[0], randomSequence.size());
	DoTest_rpmalloc(threadsCount, &randomSequence[0], randomSequence.size());
	DoTest_hoard(threadsCount, &randomSequence[0], randomSequence.size());
	DoTest_ltalloc(threadsCount, &randomSequence[0], randomSequence.size());
	DoTest_smmalloc(threadsCount, &randomSequence[0], randomSequence.size());
	DoTest_smmalloc_notc(threadsCount, &randomSequence[0], randomSequence.size());
	DoTest_dlmalloc(threadsCount, &randomSequence[0], randomSequence.size());
	if (threadsCount == 1)
	{
		DoTest_dlmalloc_st(threadsCount, &randomSequence[0], randomSequence.size());
	}

	return 0;
}