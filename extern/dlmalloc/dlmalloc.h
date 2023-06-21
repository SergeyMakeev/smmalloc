
extern "C"
{
    int dlmalloc_trim(size_t pad);
    void* dlmalloc(size_t);
    void* dlmemalign(size_t, size_t);
    void* dlrealloc(void*, size_t);
    void dlfree(void*);
    size_t dlmalloc_usable_size(void* mem);
}
