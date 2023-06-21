#include <ubench.h>
#include <vector>
#include <smmalloc.h>
#include <dlmalloc.h>
#include <rpmalloc.h>
#include <mimalloc.h>
#include <hoard.h>


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
#endif

