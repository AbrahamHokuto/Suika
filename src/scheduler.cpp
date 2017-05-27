#include <suika/scheduler.hpp>
#include <suika/fiber.hpp>

#include <chrono>
#include <thread>
#include <algorithm>

#include <sys/eventfd.h>

#include <unistd.h>

using namespace suika;

constexpr static std::size_t epoll_maxevents = 256;

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

        std::uint64_t buf = 1;
        ::write(m_eventfd, &buf, sizeof(buf));
}

scheduler::scheduler()
{
        m_eventfd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (m_eventfd < 0)
                throw std::system_error(errno, std::system_category());

        m_epfd = ::epoll_create1(EPOLL_CLOEXEC);
        if (m_epfd < 0)
                throw std::system_error(errno, std::system_category());

        epoll_event ev{ EPOLLIN, { .fd = m_eventfd } };        
        if (::epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_eventfd, &ev) < 0)
                throw std::system_error(errno, std::system_category());
}

scheduler::~scheduler()
{
        close(m_eventfd);
        close(m_epfd);
}

fiber_entity*
scheduler::pick_next()
{
        std::unique_lock<std::mutex> lk(m_guard, std::defer_lock);

        do {
                auto now = std::chrono::steady_clock::now();

                epoll_event events[epoll_maxevents];

                auto timeout = (!m_ready_list.empty()) ? 0
                        : (m_sleep_queue.empty() ? -1
                           : std::max(0L, std::chrono::duration_cast<std::chrono::milliseconds>(m_sleep_queue.min().deadline - std::chrono::steady_clock::now()).count()));
                
                auto ret = ::epoll_wait(m_epfd, events, epoll_maxevents, timeout);                        
                
                if (ret < 0) {
                        if (errno != EINTR) throw std::system_error(errno, std::system_category());
                        continue;
                }
                
                lk.lock();
                for (int i = 0; i < ret; ++i) {
                        if (events[i].data.fd == m_eventfd) {
                                std::uint64_t buf;
                                while (::read(m_eventfd, &buf, sizeof(buf)) > 0);                                
                        } else {
                                auto entity = static_cast<fiber_entity*>(events[i].data.ptr);
                                _ready(*entity);                                
                        }
                }
                
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

int
scheduler::epoll_ctl(int op, int fd, epoll_event* event)
{
        return ::epoll_ctl(m_epfd, op, fd, event);        
}
