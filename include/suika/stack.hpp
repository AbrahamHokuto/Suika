#ifndef SKA_STACK_HPP
#define SKA_STACK_HPP

#include <cstddef>
#include <algorithm>

namespace suika {        
        class stack {
        private:
                std::size_t m_stack_size;
                void* m_sp = nullptr;

                void alloc();
                void dealloc();

        public:
                stack() = default;
                stack(std::size_t stack_size):
                        m_stack_size(stack_size)
                {
                        alloc();
                }

                stack(const stack&) = delete;
                stack(stack&& rhs) {
                        m_stack_size = rhs.m_stack_size;
                        std::swap(m_sp, rhs.m_sp);
                };

                stack&
                operator=(stack&& rhs)
                {
                        if (m_sp) dealloc();

                        m_sp = nullptr;
                        std::swap(m_sp, rhs.m_sp);
                        
                        return *this;
                }

                ~stack() { if (m_sp) dealloc(); }
                
                void* sp() const noexcept { return m_sp; }
        };
}

#endif  // SKA_STACK_HPP
