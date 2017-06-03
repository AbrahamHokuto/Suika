#include <suika/io.hpp>
#include <suika/scheduler.hpp>
#include <suika/fiber.hpp>

#include <unistd.h>

#include <sys/eventfd.h>
#include <sys/epoll.h>

using namespace suika;

io::watcher::watcher(int fd, masks_t masks, loop& l)
{
        start(fd, masks, l);
}

void
io::watcher::start(int fd, masks_t masks, loop& l)
{
        m_fd = fd;
        m_loop = &l;

        m_loop->add(*this, masks);        
}

void
io::watcher::stop()
{
        m_loop->del(*this);
        
        m_fd = -1;
        m_loop = nullptr;        
}

void
io::watcher::set_masks(masks_t masks)
{
        m_loop->mod(*this, masks);
}

void
io::watcher::wait()
{
        m_bp.wait();
}

bool
io::watcher::wait(std::chrono::milliseconds timeout)
{
        scheduler::timer t;
        t.deadline = std::chrono::steady_clock::now() + timeout;

        fiber_entity::this_fiber->register_timer(t);
        
        m_bp.wait();

        return std::chrono::steady_clock::now() <= t.deadline;
}

void
io::watcher::wake()
{
        m_bp.wake();
}

io::loop::loop()
{
        auto epfd = ::epoll_create1(EPOLL_CLOEXEC);
        if (epfd < 0)
                throw std::system_error(errno, std::system_category());

        m_epfd = epfd;
        
        auto eventfd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (eventfd < 0) {
                ::close(m_epfd);
                throw std::system_error(errno, std::system_category());
        }

        m_eventfd = eventfd;

        ::epoll_event ev{EPOLLIN, { .fd = m_eventfd }};
        
        if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_eventfd, &ev) < 0) {
                ::close(m_epfd);
                ::close(m_eventfd);

                throw std::system_error(errno, std::system_category());
        }
}

io::loop::~loop()
{
        ::close(m_epfd);
        ::close(m_eventfd);
}

void
io::loop::run_once(std::chrono::milliseconds timeout)
{
        constexpr static std::size_t max_events = 256;
        
        ::epoll_event evs[max_events];

        while (true) {                
                auto ret = ::epoll_wait(m_epfd, evs, max_events, timeout == std::chrono::milliseconds::max() ? -1 : std::max(0L, timeout.count()));
                if (ret < 0) {
                        if (errno != EINTR)
                                throw std::system_error(errno, std::system_category());

                        continue;
                }

                for (int i = 0; i < ret; ++i) {
                        if (evs[i].data.fd == m_eventfd) {
                                uint64_t buf = 0;
                                while (read(m_eventfd, &buf, sizeof(buf)) > 0);

                                if (errno != EAGAIN)
                                        throw std::system_error(errno, std::system_category());

                        } else {
                                auto w = static_cast<watcher*>(evs[i].data.ptr);
                                w->wake();
                        }
                }

                break;
        }
        m_interrupted.store(false, std::memory_order_release);
}

void
io::loop::interrupt()
{
        if (m_interrupted.load(std::memory_order_consume))
                return;

        m_interrupted.store(false, std::memory_order_release);
        
        std::uint64_t buf = 1;
        if (::write(m_eventfd, &buf, sizeof(buf)) < 0)
                throw std::system_error(errno, std::system_category());
}

void
io::loop::add(watcher& entity, masks_t masks)
{
        ::epoll_event ev{masks, { .ptr = &entity }};
        if (::epoll_ctl(m_epfd, EPOLL_CTL_ADD, entity.fd(), &ev) < 0) {
                if (errno != EEXIST || !(masks & EPOLLONESHOT))
                        throw std::system_error(errno, std::system_category());

                if (::epoll_ctl(m_epfd, EPOLL_CTL_MOD, entity.fd(), &ev) < 0)
                        throw std::system_error(errno, std::system_category());                
        }        
}

void
io::loop::del(watcher& entity)
{
        ::epoll_event ev;
        if (::epoll_ctl(m_epfd, EPOLL_CTL_DEL, entity.fd(), &ev) < 0)
                throw std::system_error(errno, std::system_category());        
}

void
io::loop::mod(watcher& entity, masks_t masks)
{
        ::epoll_event ev{masks, { .ptr = &entity }};
        if (::epoll_ctl(m_epfd, EPOLL_CTL_MOD, entity.fd(), &ev) < 0)
                throw std::system_error(errno, std::system_category());        
}

io::loop&
io::loop::tls_instance()
{
        static thread_local io::loop instance;
        return instance;
}
