#ifndef SKA_SCHEDULER_HPP
#define SKA_SCHEDULER_HPP

#include "list.hpp"

#include <mutex>

namespace suika {
        class fiber_entity;
        
        class scheduler {                
        private:
                using fiber_list_t = list<fiber_entity>;
                
                fiber_list_t m_ready_list;
                std::mutex m_guard;

        public:                
                void ready(fiber_entity&);
                fiber_entity* pick_next();
        };
}

#endif  // SKA_SCHEDULER_HPP
