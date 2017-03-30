#ifndef SKA_CONTEXT_HPP
#define SKA_CONTEXT_HPP

#include <cstdint>

namespace suika {
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
