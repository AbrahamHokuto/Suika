#ifndef SKA_IO_HPP
#define SKA_IO_HPP

#include "blockpoint.hpp"

#include <sys/epoll.h>

#include <chrono>
#include <memory>
#include <atomic>

namespace suika::io {
        using masks_t = std::uint32_t;
        
        namespace masks {
                constexpr static masks_t read = EPOLLIN;
                constexpr static masks_t write = EPOLLOUT;

                constexpr static masks_t edge_triggered = EPOLLET;
                constexpr static masks_t level_triggered = 0;

                constexpr static masks_t oneshot = EPOLLONESHOT;
        }

        class watcher;
        
        class loop {
        private:
                int m_epfd = -1;
                int m_eventfd = -1;

                std::atomic_bool m_interrupted{false};

        public:
                loop();
                ~loop();

                void run_once(std::chrono::steady_clock::time_point deadline);

                void interrupt();

                void add(watcher& entity, masks_t masks);
                void del(watcher& entity);
                void mod(watcher& entity, masks_t masks);

        public:
                static loop& tls_instance();
        };

        class watcher {
        private:
                int m_fd = -1;
                loop* m_loop = nullptr;

                blockpoint m_bp;

        public:
                watcher() = default;
                watcher(int fd, masks_t masks, loop& l = loop::tls_instance());
                watcher(const watcher&) = delete;

                int fd() { return m_fd; }

                void start(int fd, masks_t masks, loop& l = loop::tls_instance());
                void stop();

                bool running() { return m_loop; }
                
                void set_masks(masks_t masks);
                void wait();
                bool wait(std::chrono::milliseconds timeout);
                void wake();
        };        
};

#endif  // SKA_IO_HPP
