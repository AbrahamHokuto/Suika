#ifndef SKA_SYNC_ADAPTIVE_LOCK
#define SKA_SYNC_ADAPTIVE_LOCK

#include "../fiber.hpp"
#include "../blockpoint.hpp"

#include <atomic>

namespace suika::sync {
        class adaptive_lock {
        private:
                const static unsigned max_spin = 255;

        private:
                std::atomic_bool m_locked{false};
                blocklist m_waiters;

                void lock_slowpath();

        public:
                void lock();
                void unlock();

                bool try_lock();
                
        };
}

#endif  // SKA_SYNC_ADAPTIVE_LOCK
