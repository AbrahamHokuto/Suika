#ifndef SKA_BLOCKPOINT_HPP
#define SKA_BLOCKPOINT_HPP

#include "list.hpp"

#include <atomic>
#include <mutex>

namespace suika {
        class fiber_entity;

        class blockpoint {
        private:
                std::atomic<fiber_entity*> m_current{nullptr};

        public:
                bool occupied();
                
                void wait();
                void wake();

                bool try_wait();
        };
        
        class blocklist {
        private:
                using fiber_list_t = list<fiber_entity>;
                
                fiber_list_t m_waiting_list;
                std::mutex m_guard;

        public:
                void wait();
                
                void wake_one();
                void wake_all();
        };
       
        using futex_word = std::int32_t;

        class futex {
        private:
                using fiber_list_t = list<fiber_entity>;
                
                std::atomic<std::size_t> m_waiters{0};
                
                fiber_list_t m_waiting_list;
                std::mutex m_guard;

        public:
                std::atomic<futex_word> word;

                futex() = default;
                futex(futex_word init_val): word(init_val) {}
                
                void wait(futex_word val);
                void wake(std::size_t count);
        };
}

#endif  // SKA_BLOCKPOINT_HPP
