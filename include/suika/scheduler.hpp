#ifndef SKA_SCHEDULER_HPP
#define SKA_SCHEDULER_HPP

#include "list.hpp"
#include "rbtree.hpp"
#include "io.hpp"

#include <list>
#include <mutex>
#include <queue>
#include <chrono>

#include <sys/epoll.h>

namespace suika {
        class fiber_entity;
        
        class scheduler {
        public:
                using time_point = std::chrono::steady_clock::time_point;

                struct timer: public rbtree_base_hook<timer> {
                        time_point deadline;
                        fiber_entity* fiber;                        
                };

        private:
                using fiber_list_t = list<fiber_entity>;               

                struct timer_less
                {
                        bool
                        operator()(const timer& lhs, const timer& rhs) const noexcept
                        {
                                return lhs.deadline < rhs.deadline;
                        }
                };

                using timer_queue_t = rbtree<timer, timer_less>;                
                
        private:
                std::mutex m_guard;
                fiber_list_t m_ready_list;

                timer_queue_t m_sleep_queue;

                io::loop& m_loop;

                int m_epfd = -1, m_eventfd = -1;

                void _ready(fiber_entity&);
                
        public:
                scheduler();
                
                void ready(fiber_entity&);
                fiber_entity* pick_next();
                void wait(timer& t);

                // TODO: separate impl from scheduler
                int epoll_ctl(int op, int fd, epoll_event* event);
        };
}

#endif  // SKA_SCHEDULER_HPP
