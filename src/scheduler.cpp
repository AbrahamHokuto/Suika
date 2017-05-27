#include <suika/scheduler.hpp>
#include <suika/fiber.hpp>
#include <suika/io.hpp>

#include <chrono>
#include <thread>
#include <algorithm>

#include <sys/eventfd.h>

#include <unistd.h>

using namespace suika;

scheduler::scheduler(): m_loop(io::loop::tls_instance()) {}

void
scheduler::_ready(fiber_entity& entity)
{
        m_ready_list.insert_tail(entity);
}

void
scheduler::ready(fiber_entity& entity)
{
        std::lock_guard<std::mutex> lk(m_guard);
        _ready(entity);

        m_loop.interrupt();
}

fiber_entity*
scheduler::pick_next()
{
        std::unique_lock<std::mutex> lk(m_guard, std::defer_lock);
        
        do {
                m_loop.run_once(m_sleep_queue.empty() ? std::chrono::steady_clock::time_point() : m_sleep_queue.min().deadline);
                
                auto now = std::chrono::steady_clock::now();

                lk.lock();
                while (!m_sleep_queue.empty())
                {                        
                        auto& t = m_sleep_queue.min();
                        if (t.deadline > now)
                                break;
                        
                        _ready(*t.fiber);
                        t.remove();
                }
                lk.unlock();               
        } while(m_ready_list.empty());

        auto ret = &m_ready_list.head();
        ret->unlink();
        return ret;
}

void
scheduler::wait(timer& t)
{
        m_sleep_queue.insert(t);
}
