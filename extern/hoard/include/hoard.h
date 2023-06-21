
extern "C"
{
    extern void hoardInitialize(void);
    extern void hoardFinalize(void);
    extern void hoardThreadInitialize(void);
    extern void hoardThreadFinalize(void);
    extern void* xxmalloc(size_t sz);
    extern void xxfree(void* ptr);
}
