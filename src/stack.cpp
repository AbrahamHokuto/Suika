#include <suika/stack.hpp>

#include <system_error>

#include <sys/mman.h>
#include <errno.h>

void
suika::stack::alloc()
{
        auto ret = mmap(nullptr, m_stack_size, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_GROWSDOWN | MAP_STACK, 0, 0);

        if (ret == MAP_FAILED)
                throw std::system_error(errno, std::system_category());

        auto sp = reinterpret_cast<std::uint64_t>(ret) + m_stack_size;
        m_sp = reinterpret_cast<void*>(sp);
}

void
suika::stack::dealloc()
{
        auto _ptr = reinterpret_cast<std::uint64_t>(m_sp) - m_stack_size;
        auto ptr = reinterpret_cast<void*>(_ptr);
        munmap(ptr, m_stack_size);
}
