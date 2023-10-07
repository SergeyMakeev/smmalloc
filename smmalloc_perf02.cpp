#include <ubench.h>
#include <vector>
#include <smmalloc.h>
#include <dlmalloc.h>
#include <rpmalloc.h>

#if defined(_WIN32)
#include <mimalloc.h>
#include <hoard.h>
#endif


struct UBenchGlobals
{
    static const int kNumAllocations = 10000000;
    static const int kWorkingsetSize = 10000;

    std::vector<size_t> randomSequence;
    std::vector<void*> workingSet;

    UBenchGlobals()
    {
        srand(2345);

        // num allocations
        randomSequence.resize(kNumAllocations);
        for (size_t i = 0; i < randomSequence.size(); i++)
        {
            // 16 - 80 bytes
            size_t sz = 16 + (rand() % 64);
            randomSequence[i] = sz;
        }

        // working set
        workingSet.resize(kWorkingsetSize);
        memset(workingSet.data(), 0, sizeof(void*) * workingSet.size());
    }

    static UBenchGlobals& get()
    {
        static UBenchGlobals g;
        return g;
    }
};

// smmalloc ubench test
UBENCH_EX(PerfTest, smmalloc_10m)
{
    UBenchGlobals& g = UBenchGlobals::get();
    size_t wsSize = g.workingSet.size();

    sm_allocator space = _sm_allocator_create(20, (48 * 1024 * 1024));

    UBENCH_DO_BENCHMARK()
    {
        size_t freeIndex = 0;
        size_t allocIndex = wsSize - 1;

        for (size_t i = 0; i < g.randomSequence.size(); i++)
        {
            size_t numBytesToAllocate = g.randomSequence[i];
            void* ptr = _sm_malloc(space, numBytesToAllocate, 1);
            memset(ptr, 33, numBytesToAllocate);

            g.workingSet[allocIndex % wsSize] = ptr;
            void* ptrToFree = g.workingSet[freeIndex % wsSize];
            _sm_free(space, ptrToFree);
            g.workingSet[freeIndex % wsSize] = nullptr;

            allocIndex++;
            freeIndex++;
        }

        for (size_t i = 0; i < wsSize; i++)
        {
            _sm_free(space, g.workingSet[i]);
            g.workingSet[i] = nullptr;
        }
    }

#ifdef SMMALLOC_STATS_SUPPORT
    const sm::GlobalStats& gstats = space->GetGlobalStats();
    size_t numAllocationAttempts = gstats.totalNumAllocationAttempts.load();
    size_t numAllocationsServed = gstats.totalAllocationsServed.load();
    size_t numAllocationsRouted = gstats.totalAllocationsRoutedToDefaultAllocator.load();
    double servedPercentage = (numAllocationAttempts == 0) ? 0.0 : (double(numAllocationsServed) / double(numAllocationAttempts) * 100.0);
    double routedPercentage = (numAllocationAttempts == 0) ? 0.0 : (double(numAllocationsRouted) / double(numAllocationAttempts) * 100.0);
    printf("Allocation attempts: %zu\n", numAllocationAttempts);
    printf("Allocations served: %zu (%3.2f%%)\n", numAllocationsServed, servedPercentage);
    printf("Allocated using default malloc: %zu (%3.2f%%)\n", numAllocationsRouted, routedPercentage);
    printf("  - Because of size: %zu\n", gstats.routingReasonBySize.load());
    printf("  - Because of saturation: %zu\n", gstats.routingReasonSaturation.load());
    size_t bucketsCount = space->GetBucketsCount();
    for (size_t bucketIndex = 0; bucketIndex < bucketsCount; bucketIndex++)
    {
        uint32_t elementsCount = space->GetBucketElementsCount(bucketIndex);
        size_t elementsSize = sm::getBucketSizeInBytesByIndex(bucketIndex);
        printf("Bucket[%zu], Elements[%d], SizeOf[%zu] -----\n", bucketIndex, elementsCount, elementsSize);
        const sm::BucketStats* stats = space->GetBucketStats(bucketIndex);
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

    _sm_allocator_destroy(space);
}

// crt ubench test
UBENCH_EX(PerfTest, crt_10m)
{
    UBenchGlobals& g = UBenchGlobals::get();
    size_t wsSize = g.workingSet.size();

    UBENCH_DO_BENCHMARK()
    {
        size_t freeIndex = 0;
        size_t allocIndex = wsSize - 1;

        for (size_t i = 0; i < g.randomSequence.size(); i++)
        {
            size_t numBytesToAllocate = g.randomSequence[i];
            void* ptr = malloc(numBytesToAllocate);
            memset(ptr, 33, numBytesToAllocate);

            g.workingSet[allocIndex % wsSize] = ptr;
            void* ptrToFree = g.workingSet[freeIndex % wsSize];
            free(ptrToFree);
            g.workingSet[freeIndex % wsSize] = nullptr;

            allocIndex++;
            freeIndex++;
        }

        for (size_t i = 0; i < wsSize; i++)
        {
            free(g.workingSet[i]);
            g.workingSet[i] = nullptr;
        }
    }
}

// dlmalloc ubench test
UBENCH_EX(PerfTest, dlmalloc_10m)
{
    UBenchGlobals& g = UBenchGlobals::get();
    size_t wsSize = g.workingSet.size();

    UBENCH_DO_BENCHMARK()
    {
        size_t freeIndex = 0;
        size_t allocIndex = wsSize - 1;

        for (size_t i = 0; i < g.randomSequence.size(); i++)
        {
            size_t numBytesToAllocate = g.randomSequence[i];
            void* ptr = dlmemalign(16, numBytesToAllocate);
            memset(ptr, 33, numBytesToAllocate);

            g.workingSet[allocIndex % wsSize] = ptr;
            void* ptrToFree = g.workingSet[freeIndex % wsSize];
            dlfree(ptrToFree);
            g.workingSet[freeIndex % wsSize] = nullptr;

            allocIndex++;
            freeIndex++;
        }

        for (size_t i = 0; i < wsSize; i++)
        {
            dlfree(g.workingSet[i]);
            g.workingSet[i] = nullptr;
        }
    }
}

#if defined(_WIN32)
// hoard ubench test
UBENCH_EX(PerfTest, hoard_malloc_10m)
{
    UBenchGlobals& g = UBenchGlobals::get();
    size_t wsSize = g.workingSet.size();

    hoardInitialize();

    UBENCH_DO_BENCHMARK()
    {
        size_t freeIndex = 0;
        size_t allocIndex = wsSize - 1;

        for (size_t i = 0; i < g.randomSequence.size(); i++)
        {
            size_t numBytesToAllocate = g.randomSequence[i];
            void* ptr = xxmalloc(numBytesToAllocate);
            memset(ptr, 33, numBytesToAllocate);

            g.workingSet[allocIndex % wsSize] = ptr;
            void* ptrToFree = g.workingSet[freeIndex % wsSize];
            xxfree(ptrToFree);
            g.workingSet[freeIndex % wsSize] = nullptr;

            allocIndex++;
            freeIndex++;
        }

        for (size_t i = 0; i < wsSize; i++)
        {
            xxfree(g.workingSet[i]);
            g.workingSet[i] = nullptr;
        }
    }

    hoardFinalize();
}

// mamalloc ubench test
UBENCH_EX(PerfTest, mi_malloc_10m)
{
    UBenchGlobals& g = UBenchGlobals::get();
    size_t wsSize = g.workingSet.size();

    UBENCH_DO_BENCHMARK()
    {
        size_t freeIndex = 0;
        size_t allocIndex = wsSize - 1;

        for (size_t i = 0; i < g.randomSequence.size(); i++)
        {
            size_t numBytesToAllocate = g.randomSequence[i];
            void* ptr = mi_malloc(numBytesToAllocate);
            memset(ptr, 33, numBytesToAllocate);

            g.workingSet[allocIndex % wsSize] = ptr;
            void* ptrToFree = g.workingSet[freeIndex % wsSize];
            mi_free(ptrToFree);
            g.workingSet[freeIndex % wsSize] = nullptr;

            allocIndex++;
            freeIndex++;
        }

        for (size_t i = 0; i < wsSize; i++)
        {
            mi_free(g.workingSet[i]);
            g.workingSet[i] = nullptr;
        }
    }

}

#endif

