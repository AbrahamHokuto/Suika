#include <suika/stack.hpp>

#include <system_error>

#include <sys/mman.h>
#include <errno.h>


suika::stack_info
suika::stack_allocator::alloc(std::size_t stack_size)
{
        auto ret = mmap(nullptr, stack_size, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, 0, 0);

        if (ret == MAP_FAILED)
                throw std::system_error(errno, std::system_category());

        auto vp = reinterpret_cast<std::uint64_t>(ret);
        auto sp = vp + stack_size;
        return { reinterpret_cast<void*>(vp), reinterpret_cast<void*>(sp) };
}

void
suika::stack_allocator::dealloc(const stack_info& info)
{
        auto stack_size = reinterpret_cast<std::uint64_t>(info.sp) - reinterpret_cast<std::uint64_t>(info.vp);
        if (munmap(info.vp, stack_size) < 0)
                throw std::system_error(errno, std::system_category());
}
