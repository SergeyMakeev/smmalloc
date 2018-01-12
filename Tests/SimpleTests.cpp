#include <UnitTest++.h>
#include <smmalloc.h>
#include <array>
#include <vector>
#include <inttypes.h>




SUITE(SimpleTests)
{


TEST(Basic)
{
	sm_allocator heap = _sm_allocator_create(5, (48 * 1024 * 1024));

	void* a = _sm_malloc(heap, 34, 16);
	void* b = _sm_malloc(heap, 34, 16);
	void* c = _sm_malloc(heap, 34, 16);
	void* d = _sm_malloc(heap, 34, 16);

	CHECK(a != b);
	CHECK(b != c);
	CHECK(c != d);
	
	ptrdiff_t d1 = (uintptr_t)b - (uintptr_t)a;
	ptrdiff_t d2 = (uintptr_t)c - (uintptr_t)b;
	ptrdiff_t d3 = (uintptr_t)d - (uintptr_t)c;

	_sm_free(heap, d);
	_sm_free(heap, c);
	_sm_free(heap, b);
	_sm_free(heap, a);

	a = _sm_malloc(heap, 34, 16);
	b = _sm_malloc(heap, 34, 16);
	c = _sm_malloc(heap, 34, 16);
	d = _sm_malloc(heap, 34, 16);

	CHECK(a != b);
	CHECK(b != c);
	CHECK(c != d);

	d1 = (uintptr_t)b - (uintptr_t)a;
	d2 = (uintptr_t)c - (uintptr_t)b;
	d3 = (uintptr_t)d - (uintptr_t)c;

	_sm_free(heap, d);
	_sm_free(heap, c);
	_sm_free(heap, b);
	_sm_free(heap, a);

	_sm_allocator_destroy(heap);
}


TEST(MegaAlloc)
{
	uint32_t poolSizeInBytes = 16777216;
	uint32_t poolSize = poolSizeInBytes / 16;
	sm_allocator heap = _sm_allocator_create(2, poolSizeInBytes);

	uint32_t cacheSize = 256 * 1024;
	_sm_allocator_thread_cache_create(heap, sm::CACHE_HOT, { cacheSize });

	std::vector<void*> ptrs;
	ptrs.reserve(poolSize + 2);

	int64_t opsCount = 0;
	clock_t start = clock();

#ifdef _DEBUG
	const int iterationsCount = 4;
#else
	const int iterationsCount = 512;
#endif

	for (int pass = 0; pass < iterationsCount; pass++)
	{
		size_t count = 0;
		for (;; count++)
		{
			opsCount++;

			void* p = _sm_malloc(heap, 16, 16);
			ptrs.push_back(p);
			if (_sm_mbucket(heap, p) != 0)
			{
				break;
			}
		}

		int64_t diff = poolSize - count;
		CHECK(diff == 0);

		for (size_t i = 0; i < ptrs.size(); i++)
		{
			_sm_free(heap, ptrs[i]);
			opsCount++;
		}

		ptrs.clear();
	}
	

	clock_t end = clock();
	float sec = (end - start) / (float)CLOCKS_PER_SEC;
	printf("%" PRId64 " allocs took %3.2f sec, %3.2f operations per second\n", opsCount, sec, opsCount / sec);

#ifdef SMMALLOC_STATS_SUPPORT
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

		printf("    Cache Hit : %zu\n", stats->cacheHitCount.load());
		printf("    Hits : %zu\n", stats->hitCount.load());
		printf("    Misses : %zu\n", stats->missCount.load());
		printf("    Operations : %zu\n", stats->cacheHitCount.load() + stats->hitCount.load() + stats->missCount.load());

	}
#endif

	_sm_allocator_thread_cache_destroy(heap);
	_sm_allocator_destroy(heap);
}


TEST(ReAlloc)
{
	sm_allocator heap = _sm_allocator_create(5, (48 * 1024 * 1024));

	void* p = _sm_malloc(heap, 17, 16);
	p = _sm_realloc(heap, p, 20, 16);
	p = _sm_realloc(heap, p, 50, 16);
	p = _sm_realloc(heap, p, 900, 16);
	_sm_free(heap, p);

	_sm_allocator_destroy(heap);
}

}

