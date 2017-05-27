#ifndef SKA_SYNC_MUTEX_HPP
#define SKA_SYNC_MUTEX_HPP

#include "../fiber.hpp"
#include "../blockpoint.hpp"

#include <atomic>

namespace suika::sync {
        class mutex {
        private:
                blocklist m_waiters;                
                std::atomic_bool m_locked{false};
                
        public:
                void lock();
                void unlock();
                bool try_lock();       
        };
};

#endif  // SKA_SYNC_MUTEX_HPP
