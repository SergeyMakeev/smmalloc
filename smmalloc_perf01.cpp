#include <atomic>
#include <chrono>
#include <float.h>
#include <thread>
#include <ubench.h>
#include <vector>

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

#include <smmalloc.h>

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
void printDebug(sm_allocator) {}
#endif

// ============ smmalloc with thread cache enabled ============
#define ALLOCATOR_TEST_NAME sm
#define HEAP sm_allocator
#define CREATE_HEAP _sm_allocator_create(5, (48 * 1024 * 1024))
#define DESTROY_HEAP                                                                                                                       \
    printDebug(heap);                                                                                                                      \
    _sm_allocator_destroy(heap)
#define ON_THREAD_START _sm_allocator_thread_cache_create(heap, sm::CACHE_WARM, {16384, 131072, 131072, 131072, 131072})
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
#define CREATE_HEAP _sm_allocator_create(5, (48 * 1024 * 1024))
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

// ============ hoard ============
// https://github.com/emeryberger/Hoard
extern "C"
{
    extern void hoardInitialize(void);
    extern void hoardFinalize(void);
    extern void hoardThreadInitialize(void);
    extern void hoardThreadFinalize(void);
    extern void* xxmalloc(size_t sz);
    extern void xxfree(void* ptr);
}

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
#include <rpmalloc.h>
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
extern "C"
{
    int dlmalloc_trim(size_t pad);
    void* dlmalloc(size_t);
    void* dlmemalign(size_t, size_t);
    void* dlrealloc(void*, size_t);
    void dlfree(void*);
    size_t dlmalloc_usable_size(void* mem);
}

#include <mutex>
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

    sm_allocator space = _sm_allocator_create(5, (48 * 1024 * 1024));

    UBENCH_DO_BENCHMARK()
    {
        size_t freeIndex = 0;
        size_t allocIndex = wsSize - 1;

        for (size_t i = 0; i < g.randomSequence.size(); i++)
        {
            size_t numBytesToAllocate = g.randomSequence[i];
            void* ptr = _sm_malloc(space, numBytesToAllocate, 16);
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

UBENCH_STATE();
int main(int argc, const char* const argv[])
{
    int res = ubench_main(argc, argv);

    printf("\n");
    printf("name\tnum_threads\tops min\tops max\tops avg\ttime_min\ttime_max\n");
    DoTest_crt();
    DoTest_sm();
    DoTest_hoard();
    DoTest_lt();
    DoTest_rp();
    DoTest_dl_mt();

    if (PerfTestGlobals::kThreadsCount == 1)
    {
        DoTest_sm_tcd();
        DoTest_dl_st();
    }

    return res;
}
