// The MIT License (MIT)
//
// 	Copyright (c) 2017-2021 Sergey Makeev
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
#include "smmalloc.h"
#include <malloc.h>

struct Header
{
    void* p;
    size_t size;
};

sm::GenericAllocator::TInstance sm::GenericAllocator::Invalid() { return nullptr; }

bool sm::GenericAllocator::IsValid(TInstance instance)
{
    SMMALLOC_UNUSED(instance);
    return true;
}

sm::GenericAllocator::TInstance sm::GenericAllocator::Create() { return nullptr; }

void sm::GenericAllocator::Destroy(sm::GenericAllocator::TInstance instance) { SMMALLOC_UNUSED(instance); }

void* sm::GenericAllocator::Alloc(sm::GenericAllocator::TInstance instance, size_t bytesCount, size_t alignment)
{
    SMMALLOC_UNUSED(instance);
    if (alignment < 16)
    {
        alignment = 16;
    }
    void* p;
    void** p2;
    size_t offset = alignment - 1 + sizeof(Header);
    if ((p = (void*)std::malloc(bytesCount + offset)) == NULL)
    {
        return NULL;
    }
    p2 = (void**)(((size_t)(p) + offset) & ~(alignment - 1));

    Header* h = reinterpret_cast<Header*>(reinterpret_cast<char*>(p2) - sizeof(Header));
    h->p = p;
    h->size = bytesCount;
    return p2;
}

void sm::GenericAllocator::Free(sm::GenericAllocator::TInstance instance, void* p)
{
    SMMALLOC_UNUSED(instance);
    if (!p)
    {
        return;
    }
    Header* h = reinterpret_cast<Header*>(reinterpret_cast<char*>(p) - sizeof(Header));
    std::free(h->p);
}

void* sm::GenericAllocator::Realloc(sm::GenericAllocator::TInstance instance, void* p, size_t bytesCount, size_t alignment)
{
    SMMALLOC_UNUSED(instance);

    void* p2 = Alloc(instance, bytesCount, alignment);
    if (!p2)
    {
        // https://en.cppreference.com/w/c/memory/realloc
        // If there is not enough memory, the old memory block is not freed and null pointer is returned.
        return nullptr;
    }

    if (p)
    {
        size_t oldBlockSize = GetUsableSpace(instance, p);
        std::memmove(p2, p, std::min(oldBlockSize, bytesCount));
    }

    Free(instance, p);
    return p2;
}

size_t sm::GenericAllocator::GetUsableSpace(sm::GenericAllocator::TInstance instance, void* p)
{
    SMMALLOC_UNUSED(instance);

    if (!p)
    {
        return 0;
    }

    Header* h = reinterpret_cast<Header*>(reinterpret_cast<char*>(p) - sizeof(Header));
    return h->size;
}
