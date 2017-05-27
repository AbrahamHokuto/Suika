#include <suika/sync/mutex.hpp>
#include <suika/sync/adaptive_lock.hpp>

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
