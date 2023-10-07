#include <atomic>
#include <chrono>
#include <float.h>
#include <thread>
#include <vector>

#include <smmalloc.h>
#include <dlmalloc.h>
#include <rpmalloc.h>
#include <mutex>

#if defined(_WIN32)
#include <mimalloc.h>
#include <hoard.h>
#endif


#define PASTER(x, y) x##y
#define EVALUATOR(x, y) PASTER(x, y)

#define _STRINGIFY(a) #a
#define STRINGIFY(a) _STRINGIFY(a)

#define TEST_ENTRYPOINT EVALUATOR(DoTest_, ALLOCATOR_TEST_NAME)
#define TEST_THREADENTRY EVALUATOR(ThreadFunc_, ALLOCATOR_TEST_NAME)

struct TestResult
{
    uint64_t opCount;
    uint64_t timeMs;
};

struct PerfTestGlobals
{
    // static const int kAllocationsCount = 1000000;
    static const int kAllocationsCount = 100000;
    static const uint32_t kThreadsCount = 5;

    std::atomic<uint32_t> canStart;
    std::vector<size_t> randomSequence;

    PerfTestGlobals()
    {
        srand(1306);
        randomSequence.resize(1024 * 1024);
        for (size_t i = 0; i < randomSequence.size(); i++)
        {
            // 16 - 80 bytes
            size_t sz = 16 + (rand() % 64);
            randomSequence[i] = sz;
        }
    }

    static PerfTestGlobals& get()
    {
        static PerfTestGlobals g;
        return g;
    }
};



#ifdef SMMALLOC_STATS_SUPPORT
void printDebug(sm_allocator heap)
{
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

        printf("    Cache Hits : %zu\n", stats->cacheHitCount.load());
        printf("    Hits : %zu\n", stats->hitCount.load());
        printf("    Misses : %zu\n", stats->missCount.load());
        printf("    Operations : %zu\n", stats->cacheHitCount.load() + stats->hitCount.load() + stats->missCount.load());
    }
}
#else
void printDebug(sm_allocator) {}
#endif

// ============ smmalloc with thread cache enabled ============
#define ALLOCATOR_TEST_NAME sm
#define HEAP sm_allocator
#define CREATE_HEAP _sm_allocator_create(10, (64 * 1024 * 1024))
#define DESTROY_HEAP                                                                                                                       \
    printDebug(heap);                                                                                                                      \
    _sm_allocator_destroy(heap)
#define ON_THREAD_START                                                                                                                    \
    _sm_allocator_thread_cache_create(heap, sm::CACHE_WARM, {16384, 131072, 131072, 131072, 131072, 131072, 131072, 131072, 131072, 131072})
#define ON_THREAD_FINISHED _sm_allocator_thread_cache_destroy(heap)
#define MALLOC(size, align) _sm_malloc(heap, size, align)
#define FREE(p) _sm_free(heap, p)
#include "smmalloc_test_impl.inl"
#undef ALLOCATOR_TEST_NAME
#undef HEAP
#undef CREATE_HEAP
#undef DESTROY_HEAP
#undef ON_THREAD_START
#undef ON_THREAD_FINISHED
#undef MALLOC
#undef FREE

// ============ smmalloc with thread cache disabled ============
#define ALLOCATOR_TEST_NAME sm_tcd
#define HEAP sm_allocator
#define CREATE_HEAP _sm_allocator_create(10, (64 * 1024 * 1024))
#define DESTROY_HEAP                                                                                                                       \
    printDebug(heap);                                                                                                                      \
    _sm_allocator_destroy(heap)
#define ON_THREAD_START
#define ON_THREAD_FINISHED
#define MALLOC(size, align) _sm_malloc(heap, size, align)
#define FREE(p) _sm_free(heap, p)
#include "smmalloc_test_impl.inl"
#undef ALLOCATOR_TEST_NAME
#undef HEAP
#undef CREATE_HEAP
#undef DESTROY_HEAP
#undef ON_THREAD_START
#undef ON_THREAD_FINISHED
#undef MALLOC
#undef FREE

// ============ crt ============
// baseline
#define ALLOCATOR_TEST_NAME crt
#define HEAP void*
#define CREATE_HEAP nullptr
#define DESTROY_HEAP
#define ON_THREAD_START
#define ON_THREAD_FINISHED
#define MALLOC(size, align) malloc(size/*, align*/)
#define FREE(p) free(p)
#include "smmalloc_test_impl.inl"
#undef ALLOCATOR_TEST_NAME
#undef HEAP
#undef CREATE_HEAP
#undef DESTROY_HEAP
#undef ON_THREAD_START
#undef ON_THREAD_FINISHED
#undef MALLOC
#undef FREE

#if defined(_WIN32)

// ============ hoard ============
// https://github.com/emeryberger/Hoard
#define ALLOCATOR_TEST_NAME hoard
#define HEAP int
#define CREATE_HEAP                                                                                                                        \
    0;                                                                                                                                     \
    hoardInitialize()
#define DESTROY_HEAP hoardFinalize()
#define ON_THREAD_START hoardThreadInitialize()
#define ON_THREAD_FINISHED hoardThreadFinalize()
#define MALLOC(size, align) xxmalloc(/*align,*/ size)
#define FREE(p) xxfree(p)
#include "smmalloc_test_impl.inl"
#undef ALLOCATOR_TEST_NAME
#undef HEAP
#undef CREATE_HEAP
#undef DESTROY_HEAP
#undef ON_THREAD_START
#undef ON_THREAD_FINISHED
#undef MALLOC
#undef FREE


#endif

// ============ ltalloc ============
// https://github.com/r-lyeh-archived/ltalloc
void* ltmalloc(size_t);
void ltfree(void*);
void* ltrealloc(void*, size_t);
void* ltcalloc(size_t, size_t);
void* ltmemalign(size_t, size_t);
void ltsqueeze(size_t pad); /*return memory to system (see README.md)*/

#define ALLOCATOR_TEST_NAME lt
#define HEAP void*
#define CREATE_HEAP nullptr
#define DESTROY_HEAP
#define ON_THREAD_START
#define ON_THREAD_FINISHED
#define MALLOC(size, align) ltmemalign(align, size)
#define FREE(p) ltfree(p)
#include "smmalloc_test_impl.inl"
#undef ALLOCATOR_TEST_NAME
#undef HEAP
#undef CREATE_HEAP
#undef DESTROY_HEAP
#undef ON_THREAD_START
#undef ON_THREAD_FINISHED
#undef MALLOC
#undef FREE

// ============ rpmalloc ============
// https://github.com/mjansson/rpmalloc
#define ALLOCATOR_TEST_NAME rp
#define HEAP int
#define CREATE_HEAP rpmalloc_initialize()
#define DESTROY_HEAP rpmalloc_finalize()
#define ON_THREAD_START rpmalloc_thread_initialize()
#define ON_THREAD_FINISHED rpmalloc_thread_finalize()
#define MALLOC(size, align) rpmemalign(align, size)
#define FREE(p) rpfree(p)
#include "smmalloc_test_impl.inl"
#undef ALLOCATOR_TEST_NAME
#undef HEAP
#undef CREATE_HEAP
#undef DESTROY_HEAP
#undef ON_THREAD_START
#undef ON_THREAD_FINISHED
#undef MALLOC
#undef FREE

// ============ dlmalloc (multi threaded) ============
// http://gee.cs.oswego.edu/dl/html/malloc.html
std::mutex globalMutex;

void* dl_malloc(size_t a, size_t v)
{
    std::lock_guard<std::mutex> lock(globalMutex);
    return dlmemalign(a, v);
}

void dl_free(void* p)
{
    std::lock_guard<std::mutex> lock(globalMutex);
    dlfree(p);
}

#define ALLOCATOR_TEST_NAME dl_mt
#define HEAP int
#define CREATE_HEAP 0
#define DESTROY_HEAP
#define ON_THREAD_START
#define ON_THREAD_FINISHED
#define MALLOC(size, align) dl_malloc(align, size)
#define FREE(p) dl_free(p)
#include "smmalloc_test_impl.inl"
#undef ALLOCATOR_TEST_NAME
#undef HEAP
#undef CREATE_HEAP
#undef DESTROY_HEAP
#undef ON_THREAD_START
#undef ON_THREAD_FINISHED
#undef MALLOC
#undef FREE

// ============ dlmalloc (single threaded) ============
// http://gee.cs.oswego.edu/dl/html/malloc.html
#define ALLOCATOR_TEST_NAME dl_st
#define HEAP int
#define CREATE_HEAP 0
#define DESTROY_HEAP
#define ON_THREAD_START
#define ON_THREAD_FINISHED
#define MALLOC(size, align) dlmemalign(align, size)
#define FREE(p) dlfree(p)
#include "smmalloc_test_impl.inl"
#undef ALLOCATOR_TEST_NAME
#undef HEAP
#undef CREATE_HEAP
#undef DESTROY_HEAP
#undef ON_THREAD_START
#undef ON_THREAD_FINISHED
#undef MALLOC
#undef FREE

#if defined(_WIN32)


// ============ mimalloc ============
// https://github.com/microsoft/mimalloc
#define ALLOCATOR_TEST_NAME mi
#define HEAP int
#define CREATE_HEAP 0
#define DESTROY_HEAP
#define ON_THREAD_START
#define ON_THREAD_FINISHED
#define MALLOC(size, align) mi_malloc_aligned(size, align)
#define FREE(p) mi_free(p)
#include "smmalloc_test_impl.inl"
#undef ALLOCATOR_TEST_NAME
#undef HEAP
#undef CREATE_HEAP
#undef DESTROY_HEAP
#undef ON_THREAD_START
#undef ON_THREAD_FINISHED
#undef MALLOC
#undef FREE

#endif

void compare_allocators()
{
    printf("\n");
    printf("name\tnum_threads\tops_min\tops_max\tops_avg\ttime_min\ttime_max\n");
    DoTest_crt();
    DoTest_sm();
   
#if defined(_WIN32)
    DoTest_mi();
    DoTest_hoard();
#endif
    DoTest_lt();
    DoTest_rp();
    DoTest_dl_mt();

    if (PerfTestGlobals::kThreadsCount == 1)
    {
        DoTest_sm_tcd();
        DoTest_dl_st();
    }
    printf("\n---------------\n\n");
}
