#include <suika/scheduler.hpp>
#include <suika/fiber.hpp>

using namespace suika;

void
scheduler::ready(fiber_entity& entity)
{
        std::lock_guard<std::mutex> lk(m_guard);
        
        m_ready_list.insert_tail(entity);
}

fiber_entity*
scheduler::pick_next()
{
        std::lock_guard<std::mutex> lk(m_guard);
        if (m_ready_list.empty()) {
                return nullptr;
        } else {
                auto ret = &m_ready_list.head();
                ret->unlink();
                return ret;
        };
}
