// The MIT License (MIT)
//
// 	Copyright (c) 2017-2023 Sergey Makeev
//
// 	Permission is hereby granted, free of charge, to any person obtaining a copy
// 	of this software and associated documentation files (the "Software"), to deal
// 	in the Software without restriction, including without limitation the rights
// 	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// 	copies of the Software, and to permit persons to whom the Software is
// 	furnished to do so, subject to the following conditions:
//
//      The above copyright notice and this permission notice shall be included in
// 	all copies or substantial portions of the Software.
//
// 	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// 	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// 	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// 	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// 	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// 	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// 	THE SOFTWARE.
#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <stdint.h>

#if __GNUC__ || __INTEL_COMPILER
#define SM_UNLIKELY(expr) __builtin_expect(!!(expr), (0))
#define SM_LIKELY(expr) __builtin_expect(!!(expr), (1))
#else
#define SM_UNLIKELY(expr) (expr)
#define SM_LIKELY(expr) (expr)
#endif

#ifdef _DEBUG

// enable asserts in the debug build
#define SMMALLOC_ENABLE_ASSERTS

// enable stats in the debug build
#define SMMALLOC_STATS_SUPPORT
#endif

#if defined(_M_X64) || _LP64
#define SMMMALLOC_X64
#define SMM_MAX_CACHE_ITEMS_COUNT (7)
#else
#define SMMMALLOC_X86
#define SMM_MAX_CACHE_ITEMS_COUNT (10)
#endif

#ifndef SMM_CACHE_LINE_SIZE
#define SMM_CACHE_LINE_SIZE (64)
#endif

#ifndef SMM_MAX_BUCKET_COUNT
#define SMM_MAX_BUCKET_COUNT (62)
#endif

#if !defined(SMM_LINEAR_PARTITIONING) && !defined(SMM_FLOAT_PARTITIONING) && !defined(SMM_PL_PARTITIONING)

//#define SMM_LINEAR_PARTITIONING
/*
 
  Simple linear partitioning: every bucket size grow by 16 bytes
  Note: fastest but can be wastefull because bucket size distribution is not ideal (works pretty well for a small number of buckets)

  Bucket size table: ( bucket_index -> size_in_bytes )
    0->16, 1->32, 2->48, 3->64, 4->80, 5->96, 6->112, 7->128, 8->144, 9->160, 10->176, 11->192, 12->208,
    13->224, 14->240, 15->256, 16->272, 17->288, 18->304, 19->320, 20->336, 21->352, 22->368, 23->384, 24->400,
    25->416, 26->432, 27->448, 28->464, 29->480, 30->496, 31->512, 32->528, 33->544, 34->560, 35->576, 36->592,
    37->608, 38->624, 39->640, 40->656, 41->672, 42->688, 43->704, 44->720, 45->736, 46->752, 47->768, 48->784,
    49->800, 50->816, 51->832, 52->848, 53->864, 54->880, 55->896, 56->912, 57->928, 58->944, 59->960, 60->976, 61->992
*/

#define SMM_PL_PARTITIONING
/*
  Piecewise linear function partitioning
  Note: as fast as linear partitioning and less wasteful in the case if the number of buckets is big

  Bucket size table: ( bucket_index -> size_in_bytes )
    0->16, 1->32, 2->48, 3->64, 4->80, 5->96, 6->112, 7->128, 8->256, 9->384, 10->512,
    11->640, 12->768, 13->896, 14->1024, 15->1536, 16->2048, 17->2560, 18->3072, 19->3584, 20->4096,
    21->4608, 22->5120, 23->5632, 24->6144, 25->6656, 26->7168, 27->7680, 28->8192,
    29->8704, 30->9216, 31->9728, 32->10240, 33->10752, 34->11264, 35->11776, 36->12288, 37->12800, 38->13312,
    39->13824, 40->14336, 41->14848, 42->15360, 43->15872, 44->16384, 45->16896, 46->17408, 47->17920, 48->18432, 49->18944,
    50->19456, 51->19968, 52->20480, 53->20992, 54->21504, 55->22016, 56->22528, 57->23040, 58->23552, 59->24064, 60->24576, 61->25088
*/

//#define SMM_FLOAT_PARTITIONING
/*
  Use Sebastian Aaltonen's floating point partitioning idea (https://github.com/sebbbi/OffsetAllocator)
  This is significantly SLOWER! than linear partitioning but might be less wasteful in case you need to handle big enough allocations

  Note: smmalloc floating point is different (2-bit mantissa + 6-bit exponent) and biased to better serve common C++ allocation sizes

  Bucket size table: ( bucket_index -> size_in_bytes )
    0->16, 1->20, 2->24, 3->28, 4->32, 5->40, 6->48, 7->56, 8->64, 9->80, 10->96, 11->112, 12->128,
    13->160, 14->192, 15->224, 16->256, 17->320, 18->384, 19->448, 20->512, 21->640, 22->768, 23->896, 24->1024,
    25->1280, 26->1536, 27->1792, 28->2048, 29->2560, 30->3072, 31->3584, 32->4096, 33->5120, 34->6144, 35->7168, 36->8192,
    37->10240, 38->12288, 39->14336, 40->16384, 41->20480, 42->24576, 43->28672, 44->32768, 45->40960, 46->49152, 47->57344, 48->65536,
    49->81920, 50->98304, 51->114688, 52->131072, 53->163840, 54->196608, 55->229376, 56->262144, 57->327680, 58->393216, 59->458752, 60->524288, 61->655360
*/
#endif

#define SMM_MANTISSA_BITS uint32_t(2)
#define SMM_MANTISSA_VALUE uint32_t(1 << SMM_MANTISSA_BITS)
#define SMM_MANTISSA_MASK uint32_t(SMM_MANTISSA_VALUE - 1)

#define SMMALLOC_UNUSED(x) (void)(x)
#define SMMALLOC_USED_IN_ASSERT(x) (void)(x)

#ifdef _MSC_VER
#define SMM_INLINE __forceinline
//#define SMM_INLINE inline
#else
#define SMM_INLINE inline __attribute__((always_inline))
//#define SMM_INLINE inline
#endif

#ifdef _MSC_VER
#define SMM_NOINLINE __declspec(noinline)
#else
#define SMM_NOINLINE __attribute__((__noinline__))
#endif

#ifdef SMMALLOC_ENABLE_ASSERTS
#include <assert.h>
//#define SM_ASSERT(x) assert(x)
#define SM_ASSERT(cond)                                                                                                                    \
    do                                                                                                                                     \
    {                                                                                                                                      \
        if (!(cond))                                                                                                                       \
            __debugbreak();                                                                                                                \
    } while (0)
#else
#define SM_ASSERT(x)
#endif

namespace sm
{
#ifdef SMMALLOC_STATS_SUPPORT
struct GlobalStats
{
    std::atomic<size_t> totalNumAllocationAttempts;
    std::atomic<size_t> totalAllocationsServed;
    std::atomic<size_t> totalAllocationsRoutedToDefaultAllocator;
    std::atomic<size_t> routingReasonBySize;
    std::atomic<size_t> routingReasonSaturation;
    
    GlobalStats()
    {
        totalNumAllocationAttempts.store(0);
        totalAllocationsServed.store(0);
        totalAllocationsRoutedToDefaultAllocator.store(0);
        routingReasonBySize.store(0);
        routingReasonSaturation.store(0);
    }
};

struct BucketStats
{
    std::atomic<size_t> cacheHitCount;
    std::atomic<size_t> hitCount;
    std::atomic<size_t> missCount;
    std::atomic<size_t> freeCount;
    
    BucketStats()
    {
        cacheHitCount.store(0);
        hitCount.store(0);
        missCount.store(0);
        freeCount.store(0);
    }
};
#endif

enum CacheWarmupOptions
{
    CACHE_COLD = 0, // none tls buckets are filled from centralized storage
    CACHE_WARM = 1, // half tls buckets are filled from centralized storage
    CACHE_HOT = 2,  // all tls buckets are filled from centralized storage
};

namespace internal
{
struct TlsPoolBucket;
}

internal::TlsPoolBucket* GetTlsBucket(size_t index);

#ifdef SMM_FLOAT_PARTITIONING

namespace SmallFloat
{

SMM_INLINE uint32_t lzcnt_nonzero(uint32_t v)
{
#ifdef _MSC_VER
    unsigned long retVal;
    _BitScanReverse(&retVal, v);
    return 31 - retVal;
#else
    return __builtin_clz(v);
#endif
}

// Bin sizes follow floating point (exponent + mantissa) distribution (piecewise linear log approx)
// This ensures that for each size class, the average overhead percentage stays the same
SMM_INLINE uint32_t uintToFloatRoundUp(uint32_t size)
{
    uint32_t exp = 0;
    uint32_t mantissa = 0;

    if (size < SMM_MANTISSA_VALUE)
    {
        // Denorm: 0..(MANTISSA_VALUE-1)
        mantissa = size;
    }
    else
    {
        // Normalized: Hidden high bit always 1. Not stored. Just like float.
        uint32_t leadingZeros = lzcnt_nonzero(size);
        uint32_t highestSetBit = 31 - leadingZeros;

        uint32_t mantissaStartBit = highestSetBit - SMM_MANTISSA_BITS;
        exp = mantissaStartBit + 1;
        mantissa = (size >> mantissaStartBit) & SMM_MANTISSA_MASK;

        uint32_t lowBitsMask = (1 << mantissaStartBit) - 1;

        // Round up!
        if ((size & lowBitsMask) != 0)
            mantissa++;
    }

    return (exp << SMM_MANTISSA_BITS) + mantissa; // + allows mantissa->exp overflow for round up
}

SMM_INLINE uint32_t floatToUint(uint32_t floatValue)
{
    uint32_t exponent = floatValue >> SMM_MANTISSA_BITS;
    uint32_t mantissa = floatValue & SMM_MANTISSA_MASK;
    if (exponent == 0)
    {
        // Denorms
        return mantissa;
    }
    else
    {
        return (mantissa | SMM_MANTISSA_VALUE) << (exponent - 1);
    }
}

} // namespace SmallFloat

#endif

SMM_INLINE bool IsAligned(size_t v, size_t alignment)
{
    size_t lowBits = v & (alignment - 1);
    return (lowBits == 0);
}

SMM_INLINE size_t Align(size_t val, size_t alignment)
{
    SM_ASSERT((alignment & (alignment - 1)) == 0 && "Invalid alignment. Must be power of two.");
    size_t r = (val + (alignment - 1)) & ~(alignment - 1);
    SM_ASSERT(IsAligned(r, alignment) && "Alignment failed.");
    return r;
}

SMM_INLINE size_t getBucketIndexBySize(size_t bytesCount)
{
#if defined(SMM_LINEAR_PARTITIONING)
    size_t bucketIndex = ((bytesCount - 1) >> 4);
    return bucketIndex;
#elif defined(SMM_FLOAT_PARTITIONING)
    SM_ASSERT(bytesCount < UINT32_MAX && "Can't handle allocation size >= 4Gb");
    uint32_t _bucketIndex = SmallFloat::uintToFloatRoundUp(uint32_t(bytesCount));
    // remap to a proper range
    uint32_t bucketIndex = (_bucketIndex < 12) ? 0 : (_bucketIndex - 12);
    return size_t(bucketIndex);
#elif defined(SMM_PL_PARTITIONING)
    SM_ASSERT(bytesCount > 0);
    size_t size = (bytesCount - 1);
    size_t p0 = (size >> 4);
    size_t p1 = (7 + (size >> 7));
    size_t p2 = (13 + (size >> 9));
    size_t bucketIndex = (size <= 127) ? p0 : ((size > 1023) ? p2 : p1);
    return bucketIndex;
#else
    #error Unknown partitioning scheme!
#endif
}

SMM_INLINE size_t getBucketSizeInBytesByIndex(size_t bucketIndex)
{
#if defined(SMM_LINEAR_PARTITIONING)
    size_t sizeInBytes = 16 + bucketIndex * 16;
    return sizeInBytes;
#elif defined(SMM_FLOAT_PARTITIONING)
    // remap to a proper range
    uint32_t _bucketIndex = uint32_t(bucketIndex) + 12;
    uint32_t sizeInBytes = SmallFloat::floatToUint(_bucketIndex);
    return size_t(sizeInBytes);
#elif defined(SMM_PL_PARTITIONING)
    size_t p0 = ((bucketIndex + 1) << 4);
    size_t p1 = ((bucketIndex - 6) << 7);
    size_t p2 = ((bucketIndex - 12) << 9);
    size_t sizeInBytes = (bucketIndex <= 7) ? p0 : ((bucketIndex > 14) ? p2 : p1);
    return sizeInBytes;
#else
#error Unknown partitioning scheme!
#endif
}

struct GenericAllocator
{
    typedef void* TInstance;

    static TInstance Invalid();
    static bool IsValid(TInstance instance);

    static TInstance Create();
    static void Destroy(TInstance instance);

    static void* Alloc(TInstance instance, size_t bytesCount, size_t alignment);
    static void Free(TInstance instance, void* p);
    static void* Realloc(TInstance instance, void* p, size_t bytesCount, size_t alignment);
    static size_t GetUsableSpace(TInstance instance, void* p);

    struct Deleter
    {
        explicit Deleter(GenericAllocator::TInstance _instance)
            : instance(_instance)
        {
        }

        SMM_INLINE void operator()(uint8_t* p) { GenericAllocator::Free(instance, p); }
        GenericAllocator::TInstance instance;
    };
};

class Allocator
{
  public:
    static const size_t kMinValidAlignment = 4;
    static const size_t kDefaultTlsCacheAlignment = kMinValidAlignment;

  private:
    static const size_t kMaxValidAlignment = 4096;

    friend struct internal::TlsPoolBucket;

    SMM_INLINE bool IsReadable(void* p) const { return (uintptr_t(p) > kMaxValidAlignment); }

    struct PoolBucket
    {
        union TaggedIndex
        {
            struct
            {
                uint32_t tag;
                uint32_t offset;
            } p;
            uint64_t u;
            static const uint64_t Invalid = UINT64_MAX;
        };

        // 8 bytes
        std::atomic<uint64_t> head;
        // 4/8 bytes
        uint8_t* pData;
        // 4/8 bytes
        uint8_t* pBufferEnd;
        // 4 bytes
        std::atomic<uint32_t> globalTag;

        // 64 bytes
        char padding[SMM_CACHE_LINE_SIZE];

#ifdef SMMALLOC_STATS_SUPPORT
        BucketStats bucketStats;
#endif
        
        PoolBucket()
            : head(TaggedIndex::Invalid)
            , pData(nullptr)
            , pBufferEnd(nullptr)
            , globalTag(0)
        {
        }

        void Create(size_t elementSize);

        SMM_INLINE void* Alloc()
        {
            uint8_t* p = nullptr;
            TaggedIndex headValue;
            headValue.u = head.load();
            while (true)
            {
                // list is empty, exiting
                if (headValue.u == TaggedIndex::Invalid)
                {
                    return nullptr;
                }

                // get head pointer
                p = (pData + headValue.p.offset);
                // read head->next value
                TaggedIndex nextValue = *((TaggedIndex*)(p));

                /*
                    ABA problem here.

                    pop = Alloc
                    push = Free

                    1. Thread 0 begins a pop and sees "A" as the top, followed by "B".
                    2. Thread 1 begins and completes a pop, returning "A".
                    3. Thread 1 begins and completes a push of "D".
                    4. Thread 1 pushes "A" back onto the stack and completes.
                    5. Thread 0 sees that "A" is on top and returns "A", setting the new top to "B".
                    6. Node D is lost.

                    Visual representation here
                    http://15418.courses.cs.cmu.edu/spring2013/lecture/lockfree/slide_016

                    Solive ABA problem using taggged indexes.
                    Each time when thread push to the stack, unique 'tag' increased.
                    And prevent invalid CAS operation.

                */

                // try to swap head to head->next
                if (head.compare_exchange_strong(headValue.u, nextValue.u))
                {
                    // success
                    break;
                }
                // can't swap values, head is changed (now headValue has new head loaded) try again
            }

            // return memory block memory
            return p;
        }

        SMM_INLINE void FreeInterval(void* _pHead, void* _pTail)
        {
            uint8_t* pHead = (uint8_t*)_pHead;
            uint8_t* pTail = (uint8_t*)_pTail;

            // attach already linked list pHead->pTail to lock free list
            // replace head and link tail with old list head

            /*

            Need to use atomic increment here.
            Without atomic increment the operation++ works like this:
            1. uint32_t temp = load(addr);
            2. temp++;
            3. store(addr, temp);

            if one of the thread will superseded before step #3, it will actually roll back the counter (for all other threads) by writing
            the old value into globalTag.
            */
            uint32_t tag = globalTag.fetch_add(1, std::memory_order_relaxed);

            TaggedIndex nodeValue;
            nodeValue.p.offset = (uint32_t)(pHead - pData);
            nodeValue.p.tag = tag;

            TaggedIndex headValue;
            headValue.u = head.load();

            while (true)
            {
                // store current head as node->next
                *((TaggedIndex*)(pTail)) = headValue;

                // try to swap node and head
                if (head.compare_exchange_strong(headValue.u, nodeValue.u))
                {
                    // succes
                    break;
                }
                // can't swap values, head is changed (now headValue has new head loaded) try again
            }
        }

        SMM_INLINE bool IsMyAlloc(void* p) const { return (p >= pData && p < pBufferEnd); }
    };

  public:
    void CreateThreadCache(CacheWarmupOptions warmupOptions, std::initializer_list<uint32_t> options);
    void DestroyThreadCache();

  private:
    size_t bucketsCount;
    size_t bucketSizeInBytes;
    uint8_t* pBufferEnd;
    std::array<uint8_t*, SMM_MAX_BUCKET_COUNT> bucketsDataBegin;
    std::array<PoolBucket, SMM_MAX_BUCKET_COUNT> buckets;
    std::unique_ptr<uint8_t, GenericAllocator::Deleter> pBuffer;
    GenericAllocator::TInstance gAllocator;

#ifdef SMMALLOC_STATS_SUPPORT
    GlobalStats globalStats;
#endif

    SMM_INLINE void* AllocFromCache(internal::TlsPoolBucket* __restrict _self) const;

    template <bool useCacheL0> SMM_INLINE bool ReleaseToCache(internal::TlsPoolBucket* __restrict _self, void* _p);

    SMM_INLINE size_t FindBucket(const void* p) const
    {
        // if p is our pointer it located inside pBuffer and pBufferEnd
        // pBuffer and bucketsDataBegin[0] the same pointers
        uintptr_t index = (uintptr_t)p - (uintptr_t)bucketsDataBegin[0];

        // if p is less than pBuffer we get very huge number due to overflow and this is valid result, since bucketIndex checked before use
        // if p is greater than pBufferEnd we get huge number and this is valid result too, since bucketIndex checked before use

        size_t r = (index / bucketSizeInBytes);
        return r;
    }

    SMM_INLINE PoolBucket* GetBucketByIndex(size_t bucketIndex)
    {
        if (bucketIndex >= bucketsCount)
        {
            return nullptr;
        }
        return &buckets[bucketIndex];
    }

    SMM_INLINE const PoolBucket* GetBucketByIndex(size_t bucketIndex) const
    {
        if (bucketIndex >= bucketsCount)
        {
            return nullptr;
        }
        return &buckets[bucketIndex];
    }

    template <bool enableStatistic> SMM_INLINE void* Allocate(size_t _bytesCount, size_t alignment)
    {
        SM_ASSERT(alignment <= kMaxValidAlignment);

        // http://www.cplusplus.com/reference/cstdlib/malloc/
        // If size is zero, the return value depends on the particular library implementation (it may or may not be a null pointer),
        // but the returned pointer shall not be dereferenced.
        if (SM_UNLIKELY(_bytesCount == 0))
        {
            return (void*)alignment;
        }

#ifdef SMMALLOC_STATS_SUPPORT
        if (enableStatistic)
        {
            globalStats.totalNumAllocationAttempts.fetch_add(1, std::memory_order_relaxed);
        }
#endif

        size_t bytesCount = Align(_bytesCount, alignment);
        size_t bucketIndex = getBucketIndexBySize(bytesCount);

#ifdef SMMALLOC_STATS_SUPPORT
        bool isValidBucket = false;
#endif
        
        if (bucketIndex < bucketsCount)
        {
#ifdef SMMALLOC_STATS_SUPPORT
            isValidBucket = true;
#endif
            // try to handle allocation using local thread cache
            void* pRes = AllocFromCache(GetTlsBucket(bucketIndex));
            if (pRes)
            {
#ifdef SMMALLOC_STATS_SUPPORT
                if (enableStatistic)
                {
                    globalStats.totalAllocationsServed.fetch_add(1, std::memory_order_relaxed);
                    buckets[bucketIndex].bucketStats.cacheHitCount.fetch_add(1, std::memory_order_relaxed);
                }
#endif
                return pRes;
            }
        }

        while (bucketIndex < bucketsCount)
        {
            void* pRes = buckets[bucketIndex].Alloc();
            if (pRes)
            {
#ifdef SMMALLOC_STATS_SUPPORT
                if (enableStatistic)
                {
                    globalStats.totalAllocationsServed.fetch_add(1, std::memory_order_relaxed);
                    buckets[bucketIndex].bucketStats.hitCount.fetch_add(1, std::memory_order_relaxed);
                }
#endif
                return pRes;
            }
            else
            {
#ifdef SMMALLOC_STATS_SUPPORT
                if (enableStatistic)
                {
                    buckets[bucketIndex].bucketStats.missCount.fetch_add(1, std::memory_order_relaxed);
                }
#endif
            }

            // try next find the next bucket (with a proper alignment)
            do
            {
                bucketIndex++;
            } while (!IsAligned(getBucketSizeInBytesByIndex(bucketIndex), alignment));
        }

#ifdef SMMALLOC_STATS_SUPPORT
        if (enableStatistic)
        {
            if (isValidBucket)
            {
                globalStats.routingReasonSaturation.fetch_add(1, std::memory_order_relaxed);
            }
            else
            {
                globalStats.routingReasonBySize.fetch_add(1, std::memory_order_relaxed);
            }
            globalStats.totalAllocationsRoutedToDefaultAllocator.fetch_add(1, std::memory_order_relaxed);
        }
#endif
        // fallback to generic allocator
        return GenericAllocator::Alloc(gAllocator, _bytesCount, alignment);
    }

  public:
    Allocator(GenericAllocator::TInstance allocator);

    void Init(uint32_t bucketsCount, size_t bucketSizeInBytes);

    SMM_INLINE void* Alloc(size_t _bytesCount, size_t alignment) { return Allocate<true>(_bytesCount, alignment); }

    SMM_INLINE void Free(void* p)
    {
        // Assume that p is the pointer that is allocated by passing the zero size.
        if (SM_UNLIKELY(!IsReadable(p)))
        {
            return;
        }

        size_t bucketIndex = FindBucket(p);
        if (bucketIndex < bucketsCount)
        {
#ifdef SMMALLOC_STATS_SUPPORT
            buckets[bucketIndex].bucketStats.freeCount.fetch_add(1, std::memory_order_relaxed);
#endif

            if (ReleaseToCache<true>(GetTlsBucket(bucketIndex), p))
            {
                return;
            }

            PoolBucket* bucket = &buckets[bucketIndex];
            bucket->FreeInterval(p, p);
            return;
        }

        // fallback to generic allocator
        GenericAllocator::Free(gAllocator, (uint8_t*)p);
    }

    SMM_INLINE void* Realloc(void* p, size_t bytesCount, size_t alignment)
    {
        // Assume that p is the pointer that is allocated by passing the zero size. So no real reallocation required.
        if (!IsReadable(p))
        {
            return Alloc(bytesCount, alignment);
        }

        if (bytesCount == 0)
        {
            Free(p);
            return nullptr;
        }

        size_t bucketIndex = FindBucket(p);
        if (bucketIndex < bucketsCount)
        {
            size_t elementSize = getBucketSizeInBytesByIndex(bucketIndex);
            if (bytesCount <= elementSize)
            {
                // reuse existing memory
                return p;
            }

            // alloc new memory block and move memory
            void* p2 = Alloc(bytesCount, alignment);

            // Assume that p is the pointer that is allocated by passing the zero size. No preserve memory conents is requried.
            if (IsReadable(p))
            {
                // move the memory block to the new location
                std::memmove(p2, p, elementSize);
            }

            Free(p);

            return p2;
        }

        if (bytesCount == 0)
        {
            // http://www.cplusplus.com/reference/cstdlib/realloc/
            // If size is zero, the return value depends on the particular library implementation (it may or may not be a null pointer),
            // but the returned pointer shall not be dereferenced.
            if (IsReadable(p))
            {
                GenericAllocator::Free(gAllocator, p);
            }
            return nullptr;
        }

        // check if we need to realloc from generic allocator to smmalloc
        size_t __bytesCount = Align(bytesCount, alignment);
        size_t __bucketIndex = getBucketIndexBySize(__bytesCount);
        if (__bucketIndex < bucketsCount)
        {
            void* p2 = Alloc(bytesCount, alignment);
            // Assume that p is the pointer that is allocated by passing the zero size. No preserve memory conents is requried.
            if (IsReadable(p))
            {
                size_t numBytes = GenericAllocator::GetUsableSpace(gAllocator, p);
                // move the memory block to the new location
                std::memmove(p2, p, std::min(numBytes, bytesCount));
            }

            GenericAllocator::Free(gAllocator, p);
            return p2;
        }

        SM_ASSERT(IsReadable(p));
        return GenericAllocator::Realloc(gAllocator, p, bytesCount, alignment);
    }

    SMM_INLINE size_t GetUsableSize(void* p)
    {
        // Assume that p is the pointer that is allocated by passing the zero size.
        if (!IsReadable(p))
        {
            return 0;
        }

        size_t bucketIndex = FindBucket(p);
        if (bucketIndex < bucketsCount)
        {
            size_t elementSize = getBucketSizeInBytesByIndex(bucketIndex);
            return elementSize;
        }

        return GenericAllocator::GetUsableSpace(gAllocator, p);
    }

    SMM_INLINE int32_t GetBucketIndex(void* _p)
    {
        if (!IsMyAlloc(_p))
        {
            return -1;
        }

        size_t bucketIndex = FindBucket(_p);
        if (bucketIndex >= bucketsCount)
        {
            return -1;
        }

        return (int32_t)bucketIndex;
    }

    SMM_INLINE bool IsMyAlloc(const void* p) const { return (p >= pBuffer.get() && p < pBufferEnd); }

    SMM_INLINE size_t GetBucketsCount() const { return bucketsCount; }

    SMM_INLINE uint32_t GetBucketElementsCount(size_t bucketIndex) const
    {
        if (bucketIndex >= bucketsCount)
        {
            return 0;
        }

        size_t oneElementSize = getBucketSizeInBytesByIndex(bucketIndex);
        return (uint32_t)(bucketSizeInBytes / oneElementSize);
    }

#ifdef SMMALLOC_STATS_SUPPORT

    const GlobalStats& GetGlobalStats() const { return globalStats; }

    const BucketStats* GetBucketStats(size_t bucketIndex) const
    {
        const PoolBucket* bucket = GetBucketByIndex(bucketIndex);
        if (!bucket)
        {
            return nullptr;
        }
        return &bucket->bucketStats;
    }

#endif

    GenericAllocator::TInstance GetGenericAllocatorInstance() { return gAllocator; }
};

namespace internal
{
struct TlsPoolBucket
{
    uint8_t* pBucketData;           // 8 (4)
    uint32_t* pStorageL1;           // 8 (4)
    Allocator::PoolBucket* pBucket; // 8 (4)

    std::array<uint32_t, SMM_MAX_CACHE_ITEMS_COUNT> storageL0; //

    uint32_t maxElementsCount; // 4
    uint32_t numElementsL1;    // 4
    uint8_t numElementsL0;     // 1

    // sizeof(storageL0) + 33 bytes

    SMM_INLINE uint32_t GetElementsCount() const { return numElementsL1 + numElementsL0; }

    void Init(uint32_t* pCacheStack, uint32_t maxElementsNum, CacheWarmupOptions warmupOptions, Allocator* alloc, size_t bucketIndex);
    uint32_t* Destroy();

    SMM_INLINE void ReturnL1CacheToMaster(uint32_t count)
    {
        if (count == 0)
        {
            return;
        }

        SM_ASSERT(pBucket != nullptr);
        if (numElementsL1 == 0)
        {
            return;
        }

        count = std::min(count, numElementsL1);

        uint32_t localTag = 0xFFFFFF;
        uint32_t firstElementToReturn = (numElementsL1 - count);
        uint32_t offset = pStorageL1[firstElementToReturn];
        uint8_t* pHead = pBucketData + offset;
        uint8_t* pPrevBlockMemory = pHead;

        for (uint32_t i = (firstElementToReturn + 1); i < numElementsL1; i++, localTag++)
        {
            offset = pStorageL1[i];

            Allocator::PoolBucket::TaggedIndex* pTag = (Allocator::PoolBucket::TaggedIndex*)pPrevBlockMemory;
            pTag->p.tag = localTag;
            pTag->p.offset = offset;

            uint8_t* pBlockMemory = pBucketData + offset;
            pPrevBlockMemory = pBlockMemory;
        }

        uint8_t* pTail = pPrevBlockMemory;
        pBucket->FreeInterval(pHead, pTail);

        numElementsL1 -= count;
    }
};

static_assert(std::is_pod<TlsPoolBucket>::value == true, "TlsPoolBucket must be POD type, stored in TLS");
static_assert(sizeof(TlsPoolBucket) <= 64, "TlsPoolBucket sizeof must be less than CPU cache line");
} // namespace internal

SMM_INLINE void* Allocator::AllocFromCache(internal::TlsPoolBucket* __restrict _self) const
{
    if (_self->numElementsL0 > 0)
    {
        SM_ASSERT(_self->pBucketData != nullptr);
        _self->numElementsL0--;
        uint32_t offset = _self->storageL0[_self->numElementsL0];
        return _self->pBucketData + offset;
    }

    if (_self->numElementsL1 > 0)
    {
        SM_ASSERT(_self->pBucketData != nullptr);
        SM_ASSERT(_self->numElementsL0 == 0);
        _self->numElementsL1--;
        uint32_t offset = _self->pStorageL1[_self->numElementsL1];
        return _self->pBucketData + offset;
    }
    return nullptr;
}

template <bool useCacheL0> SMM_INLINE bool Allocator::ReleaseToCache(internal::TlsPoolBucket* __restrict _self, void* _p)
{

    if (_self->maxElementsCount == 0)
    {
        // empty cache bucket
        return false;
    }

    SM_ASSERT(_self->pBucket != nullptr);
    SM_ASSERT(_self->pBucketData != nullptr);

    // get offset
    uint8_t* p = (uint8_t*)_p;
    SM_ASSERT(p >= _self->pBucketData && p < _self->pBucket->pBufferEnd);
    uint32_t offset = (uint32_t)(p - _self->pBucketData);

    if (useCacheL0)
    {
        // use L0 storage if available
        if (_self->numElementsL0 < SMM_MAX_CACHE_ITEMS_COUNT)
        {
            _self->storageL0[_self->numElementsL0] = offset;
            _self->numElementsL0++;
            return true;
        }
    }

    if (_self->numElementsL1 < _self->maxElementsCount)
    {
        // use L1 storage if available
        _self->pStorageL1[_self->numElementsL1] = offset;
        _self->numElementsL1++;
        return true;
    }

    //
    // too many elements in cache (return half of the cache to master)
    //    minimizing worst case scenario, cache is full and thread continues to Free a lot of blocks.
    //               and each Free() call leads to an operation with the global lock-free pool

    uint32_t halfOfElements = (_self->numElementsL1 >> 1);
    _self->ReturnL1CacheToMaster(halfOfElements);

    // use L1 storage
    _self->pStorageL1[_self->numElementsL1] = offset;
    _self->numElementsL1++;
    return true;
}

} // namespace sm

#undef SMM_MANTISSA_BITS
#undef SMM_MANTISSA_VALUE
#undef SMM_MANTISSA_MASK

#define SMMALLOC_CSTYLE_FUNCS

#ifdef SMMALLOC_CSTYLE_FUNCS

#define SMMALLOC_DLL

#if defined(_WIN32) && defined(SMMALLOC_DLL)
#define SMMALLOC_API __declspec(dllexport)
#else
#define SMMALLOC_API extern
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    //
    // C style api
    //
    ////////////////////////////////////////////////////////////////////////////////

    typedef sm::Allocator* sm_allocator;

    SMMALLOC_API SMM_INLINE sm_allocator _sm_allocator_create(uint32_t bucketsCount, size_t bucketSizeInBytes)
    {
        sm::GenericAllocator::TInstance instance = sm::GenericAllocator::Create();
        if (!sm::GenericAllocator::IsValid(instance))
        {
            return nullptr;
        }

        // allocate memory for an allocator
        size_t align = __alignof(sm::Allocator);
        align = sm::Align(align, SMM_CACHE_LINE_SIZE);

        void* pBuffer = sm::GenericAllocator::Alloc(instance, sizeof(sm::Allocator), align);

        // placement new
        sm::Allocator* allocator = new (pBuffer) sm::Allocator(instance);

        // initialize
        allocator->Init(bucketsCount, bucketSizeInBytes);

        return allocator;
    }

    SMMALLOC_API SMM_INLINE void _sm_allocator_destroy(sm_allocator allocator)
    {
        if (allocator == nullptr)
        {
            return;
        }

        sm::GenericAllocator::TInstance instance = allocator->GetGenericAllocatorInstance();

        // call dtor
        allocator->~Allocator();

        // free memory
        sm::GenericAllocator::Free(instance, allocator);

        // destroy generic allocator
        sm::GenericAllocator::Destroy(instance);
    }

    SMMALLOC_API SMM_INLINE void _sm_allocator_thread_cache_create(sm_allocator allocator, sm::CacheWarmupOptions warmupOptions,
                                                               std::initializer_list<uint32_t> options)
    {
        if (allocator == nullptr)
        {
            return;
        }

        allocator->CreateThreadCache(warmupOptions, options);
    }

    SMMALLOC_API SMM_INLINE void _sm_allocator_thread_cache_destroy(sm_allocator allocator)
    {
        if (allocator == nullptr)
        {
            return;
        }

        allocator->DestroyThreadCache();
    }

    SMMALLOC_API SMM_INLINE void* _sm_malloc(sm_allocator allocator, size_t bytesCount, size_t alignment)
    {
        return allocator->Alloc(bytesCount, alignment);
    }

    SMMALLOC_API SMM_INLINE void _sm_free(sm_allocator allocator, void* p) { return allocator->Free(p); }

    SMMALLOC_API SMM_INLINE void* _sm_realloc(sm_allocator allocator, void* p, size_t bytesCount, size_t alignment)
    {
        return allocator->Realloc(p, bytesCount, alignment);
    }

    SMMALLOC_API SMM_INLINE size_t _sm_msize(sm_allocator allocator, void* p) { return allocator->GetUsableSize(p); }

    SMMALLOC_API SMM_INLINE int32_t _sm_mbucket(sm_allocator allocator, void* p) { return allocator->GetBucketIndex(p); }

#ifdef __cplusplus
}
#endif

#endif
