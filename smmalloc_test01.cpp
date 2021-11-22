#include <array>
#include <gtest/gtest.h>
#include <inttypes.h>
#include <smmalloc.h>
#include <vector>

TEST(SimpleTests, Basic)
{
    sm_allocator heap = _sm_allocator_create(5, (48 * 1024 * 1024));

    void* a = _sm_malloc(heap, 34, 16);
    void* b = _sm_malloc(heap, 34, 16);
    void* c = _sm_malloc(heap, 34, 16);
    void* d = _sm_malloc(heap, 34, 16);

    EXPECT_NE(a, b);
    EXPECT_NE(b, c);
    EXPECT_NE(c, d);

    //ptrdiff_t d1 = (uintptr_t)b - (uintptr_t)a;
    //ptrdiff_t d2 = (uintptr_t)c - (uintptr_t)b;
    //ptrdiff_t d3 = (uintptr_t)d - (uintptr_t)c;

    _sm_free(heap, d);
    _sm_free(heap, c);
    _sm_free(heap, b);
    _sm_free(heap, a);

    a = _sm_malloc(heap, 34, 16);
    b = _sm_malloc(heap, 34, 16);
    c = _sm_malloc(heap, 34, 16);
    d = _sm_malloc(heap, 34, 16);

    EXPECT_NE(a, b);
    EXPECT_NE(b, c);
    EXPECT_NE(c, d);

    //d1 = (uintptr_t)b - (uintptr_t)a;
    //d2 = (uintptr_t)c - (uintptr_t)b;
    //d3 = (uintptr_t)d - (uintptr_t)c;

    _sm_free(heap, d);
    _sm_free(heap, c);
    _sm_free(heap, b);
    _sm_free(heap, a);

    _sm_allocator_destroy(heap);
}

TEST(SimpleTests, MegaAlloc)
{
    uint32_t poolSizeInBytes = 16777216;
    uint32_t poolSize = poolSizeInBytes / 16;
    sm_allocator heap = _sm_allocator_create(2, poolSizeInBytes);

    uint32_t cacheSize = 256 * 1024;
    _sm_allocator_thread_cache_create(heap, sm::CACHE_HOT, {cacheSize});

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
        EXPECT_EQ(diff, 0);

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


bool IsAligned(void* p, size_t alignment)
{
    uintptr_t v = uintptr_t(p);
    size_t lowBits = v & (alignment - 1);
    return (lowBits == 0);
}


TEST(SimpleTests, ReAlloc)
{
    sm_allocator heap = _sm_allocator_create(5, (48 * 1024 * 1024));

    void* p = _sm_malloc(heap, 17, 16);
    size_t s = _sm_msize(heap, p);
    EXPECT_NE(p, nullptr);
    EXPECT_TRUE(IsAligned(p, 16));
    EXPECT_GE(s, 17);
    p = _sm_realloc(heap, p, 20, 16);
    s = _sm_msize(heap, p);
    EXPECT_NE(p, nullptr);
    EXPECT_TRUE(IsAligned(p, 16));
    EXPECT_GE(s, 20);
    p = _sm_realloc(heap, p, 50, 16);
    s = _sm_msize(heap, p);
    EXPECT_NE(p, nullptr);
    EXPECT_TRUE(IsAligned(p, 16));
    EXPECT_GE(s, 50);
    p = _sm_realloc(heap, p, 900, 16);
    s = _sm_msize(heap, p);
    EXPECT_NE(p, nullptr);
    EXPECT_TRUE(IsAligned(p, 16));
    EXPECT_GE(s, 900);

    _sm_free(heap, p);
    p = nullptr;
    s = _sm_msize(heap, p);
    EXPECT_EQ(p, nullptr);
    EXPECT_EQ(s, 0);


    void* p2 = _sm_realloc(heap, nullptr, 15, 16);
    size_t s2 = _sm_msize(heap, p2);
    EXPECT_NE(p2, nullptr);
    EXPECT_TRUE(IsAligned(p2, 16));
    EXPECT_GE(s2, 15);

    p2 = _sm_realloc(heap, p2, 0, 16);
    s2 = _sm_msize(heap, p2);
    EXPECT_EQ(p2, nullptr);
    EXPECT_TRUE(IsAligned(p2, 16));
    EXPECT_EQ(s2, 0);

    _sm_free(heap, p2);
    s2 = _sm_msize(heap, p2);
    EXPECT_EQ(s2, 0);

    void* p3 = _sm_realloc(heap, nullptr, 1024*1024, 1);
    size_t s3 = _sm_msize(heap, p3);
    EXPECT_NE(p3, nullptr);
    EXPECT_TRUE(IsAligned(p3, sm::Allocator::kMinValidAlignment));
    EXPECT_GE(s3, 1024*1024);
    p3 = _sm_realloc(heap, p3, 4, 8);
    s3 = _sm_msize(heap, p3);
    EXPECT_NE(p3, nullptr);
    EXPECT_TRUE(IsAligned(p3, 8));
    EXPECT_GE(s3, 4);
    p3 = _sm_realloc(heap, p3, 0, 8);
    s3 = _sm_msize(heap, p3);
    EXPECT_EQ(p3, nullptr);
    EXPECT_EQ(s3, 0);
    _sm_free(heap, p3);
    s3 = _sm_msize(heap, p3);
    EXPECT_EQ(s3, 0);

    void* p4 = _sm_malloc(heap, 2 * 1024 * 1024, 32);
    size_t s4 = _sm_msize(heap, p4);
    EXPECT_NE(p4, nullptr);
    EXPECT_TRUE(IsAligned(p4, 32));
    EXPECT_GE(s4, 2 * 1024 * 1024);

    p4 = _sm_realloc(heap, p4, 1024 * 1024, 16);
    s4 = _sm_msize(heap, p4);
    EXPECT_NE(p4, nullptr);
    EXPECT_TRUE(IsAligned(p4, 16));
    EXPECT_GE(s4, 1024 * 1024);

    p4 = _sm_realloc(heap, p4, 4 * 1024 * 1024, 64);
    s4 = _sm_msize(heap, p4);
    EXPECT_NE(p4, nullptr);
    EXPECT_TRUE(IsAligned(p4, 64));
    EXPECT_GE(s4, 4 * 1024 * 1024);
    p4 = _sm_realloc(heap, p4, 0, 32);
    p4 = nullptr;
    s4 = _sm_msize(heap, p4);
    EXPECT_EQ(s4, 0);

    size_t s5 = sm::GenericAllocator::GetUsableSpace(sm::GenericAllocator::Invalid(), nullptr);
    EXPECT_EQ(s5, 0);

    _sm_allocator_destroy(heap);
}
