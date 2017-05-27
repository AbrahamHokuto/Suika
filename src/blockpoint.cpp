#include <suika/blockpoint.hpp>
#include <suika/fiber.hpp>

using namespace suika;

void
blockpoint::wait()
{
        m_current.exchange(fiber_entity::this_fiber, std::memory_order_acq_rel);
        self::resched();
}

bool
blockpoint::try_wait()
{
        if (m_current.load(std::memory_order_consume))
                return false;

        fiber_entity* expected = nullptr;

        return m_current.compare_exchange_strong(expected, fiber_entity::this_fiber, std::memory_order_acq_rel, std::memory_order_acq_rel);
}

void
blockpoint::wake()
{
        auto value = m_current.exchange(nullptr, std::memory_order_acq_rel);

        if (value)
                value->set_ready();
}

void
blocklist::wait()
{
        std::unique_lock<std::mutex> lk(m_guard);
        m_waiting_list.insert_tail(*fiber_entity::this_fiber);
        lk.unlock();
        
        self::resched();
}

void
blocklist::wake_one()
{
        std::lock_guard<std::mutex> lk(m_guard);
        
        if (m_waiting_list.empty())
                return;
        
        auto& ret = m_waiting_list.head();
        ret.set_ready();
}

void
blocklist::wake_all()
{
        std::lock_guard<std::mutex> lk(m_guard);
        
        while (!m_waiting_list.empty())
                m_waiting_list.head().set_ready();
}

void
futex::wait(futex_word val)
{
        auto current = fiber_entity::this_fiber;
        
        std::unique_lock<std::mutex> lk(m_guard);
        m_waiters.fetch_add(1, std::memory_order_acq_rel);        
        m_waiting_list.insert_tail(*current);

        if (word.load(std::memory_order_acquire) != val) {
                m_waiters.fetch_sub(1, std::memory_order_acq_rel);
                current->unlink();                
        }

        lk.unlock();
        self::resched();
}

void
futex::wake(std::size_t count)
{
        if (!m_waiters.load(std::memory_order_acquire))
                return;
        
        std::lock_guard<std::mutex> lk(m_guard);
        for (std::size_t i = 0; !m_waiting_list.empty() && i < count; ++i)
                m_waiting_list.head().set_ready();
}
