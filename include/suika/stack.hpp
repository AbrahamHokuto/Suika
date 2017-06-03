#ifndef SKA_STACK_HPP
#define SKA_STACK_HPP

#include <cstddef>
#include <algorithm>

namespace suika {
        class stack {
        private:
                std::size_t m_stack_size;
                void* m_sp = nullptr;
                void* m_vp = nullptr;

                int m_stack_id;

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
                        std::swap(m_vp, rhs.m_vp);
                };

                stack&
                operator=(stack&& rhs)
                {
                        if (m_sp) dealloc();

                        m_sp = nullptr;
                        m_vp = nullptr;
                        std::swap(m_sp, rhs.m_sp);
                        std::swap(m_vp, rhs.m_vp);
                        
                        return *this;
                }

                ~stack() { if (m_vp) dealloc(); }
                
                void* sp() const noexcept { return m_sp; }
                void* vp() const noexcept { return m_vp; }
        };
}

#endif  // SKA_STACK_HPP
