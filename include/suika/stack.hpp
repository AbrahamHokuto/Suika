#ifndef SKA_STACK_HPP
#define SKA_STACK_HPP

#include <cstddef>
#include <algorithm>

namespace suika {
        struct stack_info {
                void* vp;
                void* sp;                
        };
        
        class stack_allocator {
        public:
                stack_allocator() = default;

                stack_info alloc(std::size_t);
                void dealloc(const stack_info& info);
        };
}

#endif  // SKA_STACK_HPP
