#include <suika/sync/mutex.hpp>
#include <suika/sync/adaptive_lock.hpp>
#include <suika/sync/semaphore.hpp>

using namespace suika;

void
sync::mutex::lock()
{
        while (m_locked.exchange(true, std::memory_order_acq_rel))
                m_waiters.wait();        
}

void
sync::mutex::unlock()
{
        m_locked.exchange(false, std::memory_order_acq_rel);        
}

bool
sync::mutex::try_lock()
{
        if (m_locked.load(std::memory_order_consume))
                return false;
        
        return !m_locked.exchange(true, std::memory_order_acq_rel);        
}

void
sync::adaptive_lock::lock_slowpath()
{
        while (m_locked.exchange(true, std::memory_order_acq_rel))
                m_waiters.wait();
}

void
sync::adaptive_lock::lock()
{
        unsigned spincount = 0;

        while (m_locked.exchange(true, std::memory_order_acq_rel) && spincount++ != max_spin)
                __builtin_ia32_pause();

        lock_slowpath();
}

void
sync::adaptive_lock::unlock()
{
        m_locked.exchange(false, std::memory_order_acq_rel);
}

bool
sync::adaptive_lock::try_lock()
{
        if (m_locked.load(std::memory_order_consume))
                return false;

        return !m_locked.exchange(true, std::memory_order_acq_rel);        
}

void
sync::semaphore::wait()
{
        auto val = m_semaphore.fetch_sub(1, std::memory_order_acq_rel);
        if (val < 1)        
                m_queue.wait();
}

void
sync::semaphore::signal(long incr)
{
        auto val = m_semaphore.fetch_add(incr);

        if (val < 0)
                m_queue.wake(incr);
}
