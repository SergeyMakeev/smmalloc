#include <ubench.h>

extern void compare_allocators();

UBENCH_STATE();
int main(int argc, const char* const argv[])
{
    int res = ubench_main(argc, argv);
    compare_allocators();
    return res;
}
