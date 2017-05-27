#ifndef SKA_SYNC_SEMAPHORE
#define SKA_SYNC_SEMAPHORE

#include "../blockpoint.hpp"

#include <atomic>

namespace suika::sync {
        class semaphore {
        private:
                std::atomic<long> m_semaphore;

                blocklist m_queue;

        public:
                semaphore(long init_val = 1): m_semaphore{init_val - 1} {}
                
                void wait();
                void signal(long incr = 1);
        };
};

#endif  // SKA_SYNC_SEMAPHORE
