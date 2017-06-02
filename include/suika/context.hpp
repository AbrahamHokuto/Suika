#ifndef SKA_CONTEXT_HPP
#define SKA_CONTEXT_HPP

#include <cstdint>

namespace suika {
        using fcontext_t = void*;

        struct transfer_t {
                fcontext_t fctx;
                void* data;
        };

        extern "C" transfer_t jump_fcontext(fcontext_t const to, void* vp);
        extern "C" fcontext_t make_fcontext(void* sp, std::size_t size, void (*fn)(transfer_t));
        
        class context {
        private:
                std::uint64_t m_sp;

                static thread_local context main;

        public:
                typedef void (*entry_t)(std::uint64_t, std::uint64_t);

                context() {}
                context(entry_t entry, std::uint64_t sp);
                context(const context&) = delete;
                context(context&&) = default;

                context& operator= (context&& rhs) { m_sp = rhs.m_sp; return *this; }

                uint64_t swap(std::uint64_t, std::uint64_t, context& target);
        };
}

#endif  // SKA_CONTEXT_HPP
