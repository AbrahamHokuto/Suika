#ifndef SKA_IO_HPP
#define SKA_IO_HPP

#include <sys/epoll.h>

#include <chrono>

namespace suika::io {
        using masks_t = std::uint32_t;
        
        namespace masks {
                constexpr static masks_t read = EPOLLIN;
                constexpr static masks_t write = EPOLLOUT;
        }
};

#endif  // SKA_IO_HPP
