#include <array>
#include <gtest/gtest.h>
#include <inttypes.h>
#include <smmalloc.h>
#include <thread>
#include <vector>

void ThreadFunc(sm_allocator heap)
{
    SM_ASSERT(heap != nullptr);
    _sm_allocator_thread_cache_create(heap, sm::CACHE_WARM, {32, 32, 32, 32, 32});

#ifdef _DEBUG
    int iterationsCount = 2;
#else
    int iterationsCount = 1024 * 4;
#endif
    for (int pass = 0; pass < iterationsCount; pass++)
    {
        std::array<std::pair<void*, size_t>, 1024> workingSet;
        std::array<uint8_t, 1024> pattern;
        for (size_t i = 0; i < workingSet.size(); i++)
        {
            size_t bytesCount = rand() % 256;
            uint8_t ptrn = bytesCount % 256;
            pattern[i] = ptrn;
            void* p = _sm_malloc(heap, bytesCount, 16);
            workingSet[i] = std::make_pair(p, bytesCount);
            std::memset(p, ptrn, bytesCount);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(2));

        for (size_t i = 0; i < workingSet.size(); i++)
        {
            uint8_t ptrn = pattern[i];
            uint8_t* p = (uint8_t*)workingSet[i].first;
            size_t bytesCount = workingSet[i].second;

            for (size_t j = 0; j < bytesCount; j++)
            {
                EXPECT_EQ(p[j], ptrn);
            }

            std::memset(p, 0, bytesCount);
            _sm_free(heap, p);
        }
    }

    _sm_allocator_thread_cache_destroy(heap);
}

TEST(MultithreadingTests, StressTest)
{
    sm_allocator heap = _sm_allocator_create(10, (48 * 1024 * 1024));

    int threadsCount = std::thread::hardware_concurrency();
#ifdef _DEBUG
    threadsCount = 1;
#endif
    printf("%d threads created\n", threadsCount);

    std::vector<std::thread> threads;
    for (int i = 0; i < threadsCount; i++)
    {
        threads.push_back(std::thread(ThreadFunc, heap));
    }

    // wait all threads
    for (auto& t : threads)
    {
        t.join();
    }

    printf("Checking thread cache\n");

    size_t count = 0;
    size_t bucketsCount = heap->GetBucketsCount();
    for (size_t i = 0; i < bucketsCount; i++)
    {
        count += heap->GetBucketElementsCount(i);
    }

    std::vector<void*> ptrs;
    ptrs.reserve(count);

    for (int32_t bucketIndex = 0; bucketIndex < (int32_t)bucketsCount; bucketIndex++)
    {
        size_t elementSize = sm::getBucketSizeInBytesByIndex(bucketIndex);

        size_t maxCount = heap->GetBucketElementsCount(bucketIndex);

        // check available space
        size_t availCount = 0;
        void* p = nullptr;
        for (;; availCount++)
        {
            // set alignement to 1 to avoid internal size alignment (based on requested alignemnt size)
            p = _sm_malloc(heap, elementSize, 1);
            ptrs.push_back(p);
            if (_sm_mbucket(heap, p) != bucketIndex)
            {
                break;
            }
        }

        // check all allocations available (all thread caches destroyed at this moment)
        EXPECT_EQ(availCount, maxCount);

        // release memory
        for (size_t i = 0; i < ptrs.size(); i++)
        {
            _sm_free(heap, ptrs[i]);
        }
        ptrs.clear();
    }

    _sm_allocator_destroy(heap);
}

std::atomic<uint64_t> operationsCount;

void ThreadFunc2(sm_allocator heap)
{
    SM_ASSERT(heap != nullptr);
    _sm_allocator_thread_cache_create(heap, sm::CACHE_WARM, {2048, 2048, 2048, 2048, 2048});

#ifdef _DEBUG
    int iterationsCount = 2;
#else
    int iterationsCount = 1024 * 100;
#endif
    size_t opCount = 0;
    for (int pass = 0; pass < iterationsCount; pass++)
    {
        std::array<std::pair<void*, size_t>, 1024> workingSet;

        for (size_t i = 0; i < workingSet.size(); i++)
        {
            size_t bytesCount = rand() % 256;
            void* p = _sm_malloc(heap, bytesCount, 16);
            workingSet[i] = std::make_pair(p, bytesCount);
            opCount++;
        }

        for (size_t i = 0; i < workingSet.size(); i++)
        {
            uint8_t* p = (uint8_t*)workingSet[i].first;
            // size_t bytesCount = workingSet[i].second;
            _sm_free(heap, p);
            opCount++;
        }
    }

    operationsCount.fetch_add(opCount, std::memory_order_relaxed);
    _sm_allocator_thread_cache_destroy(heap);
}

TEST(MultithreadingTests, MtPerformance)
{
    sm_allocator heap = _sm_allocator_create(8, (48 * 1024 * 1024));

    operationsCount.store(0);
    int threadsCount = std::thread::hardware_concurrency();
#ifdef _DEBUG
    threadsCount = 1;
#endif
    threadsCount = std::min(8, threadsCount);
    printf("%d threads created\n", threadsCount);

    clock_t start = clock();

    std::vector<std::thread> threads;
    for (int i = 0; i < threadsCount; i++)
    {
        threads.push_back(std::thread(ThreadFunc2, heap));
    }

    // wait all threads
    for (auto& t : threads)
    {
        t.join();
    }

    clock_t end = clock();
    uint64_t allocsCount = operationsCount.load();
    float sec = (end - start) / (float)CLOCKS_PER_SEC;
    printf("%" PRId64 " mt allocs took %3.2f sec, %3.2f operations per second\n", allocsCount, sec, allocsCount / sec);

#ifdef SMMALLOC_STATS_SUPPORT
    const sm::GlobalStats& gstats = heap->GetGlobalStats();
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
    size_t bucketsCount = heap->GetBucketsCount();
    for (size_t bucketIndex = 0; bucketIndex < bucketsCount; bucketIndex++)
    {
        uint32_t elementsCount = heap->GetBucketElementsCount(bucketIndex);
        size_t elementsSize = sm::getBucketSizeInBytesByIndex(bucketIndex);
        printf("Bucket[%zu], Elements[%d], SizeOf[%zu] -----\n", bucketIndex, elementsCount, elementsSize);
        const sm::BucketStats* stats = heap->GetBucketStats(bucketIndex);
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
    _sm_allocator_destroy(heap);
}
