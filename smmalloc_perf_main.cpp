#include <ubench.h>

extern void compare_allocators();

UBENCH_STATE();
int main(int argc, const char* const argv[])
{
    compare_allocators();
    int res = ubench_main(argc, argv);
    return res;
}
