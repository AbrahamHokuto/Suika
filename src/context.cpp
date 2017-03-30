#include <suika/context.hpp>

extern "C" std::uint64_t swap_context(std::uint64_t first_arg, std::uint64_t ret_or_second_arg, std::uint64_t target_sp, std::uint64_t* self_sp);
extern "C" void make_context(std::uint64_t *sp, std::uint64_t entry_addr);

using namespace suika;

context::context(entry_t entry_addr, std::uint64_t sp): m_sp(sp)
{
        make_context(&m_sp, reinterpret_cast<std::uint64_t>(entry_addr));
}

uint64_t
context::swap(std::uint64_t first_arg, std::uint64_t ret_or_second_arg, context& target)
{
        return swap_context(first_arg, ret_or_second_arg, target.m_sp, &m_sp);
}
