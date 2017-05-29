#include "allocator.hpp"

#include <cstdlib>
#include <cassert>

#include "utils.hpp"

void* detail::allocate_aligned_memory(size_t align, size_t size)
{
    assert(align >= sizeof(void*));
    assert(is_power_of_two(align));

    if (size == 0) {
        return nullptr;
    }

    void* ptr = nullptr;
    int rc = posix_memalign(&ptr, align, size);

    if (rc != 0) {
        return nullptr;
    }

    return ptr;
}


void detail::deallocate_aligned_memory(void *ptr) noexcept
{
    return free(ptr);
}
