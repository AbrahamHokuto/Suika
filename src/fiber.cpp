#include <suika/fiber.hpp>
#include <suika/scheduler.hpp>
#include <suika/exceptions.hpp>

using namespace suika;

static std::atomic<suika::id> next_id{0};
static thread_local scheduler sched;
thread_local std::unique_ptr<fiber_entity> main_fiber = std::make_unique<fiber_entity>();
thread_local fiber_entity* fiber_entity::this_fiber = main_fiber.get();

class _fiber_exit_unwind {};

fiber_entity::fiber_entity(): m_id(next_id.fetch_add(1)), m_scheduler(&sched)
{
        this_fiber = this;
}

fiber_entity::fiber_entity(context::entry_t entry, std::size_t stack_size):
        m_stack(stack_size),
        m_context(entry, reinterpret_cast<std::uint64_t>(m_stack.sp())),
        m_id(next_id.fetch_add(1)),
        m_scheduler(&sched)
{
}

fiber_entity*
fiber_entity::switch_to(std::uint64_t first, std::uint64_t second_or_ret, fiber_entity* entity)
{
        if (!entity || entity == this_fiber)
                return nullptr;
        
        this_fiber = entity;
        
        auto _prev = m_context.swap(first, second_or_ret, entity->m_context);        
        auto prev = reinterpret_cast<fiber_entity*>(_prev);

        this_fiber = this;

        
        if (prev->m_status_futex.word.load(std::memory_order::memory_order_acquire) > 1) {
                switch (prev->m_status_futex.word.exchange(4, std::memory_order_release)) {
                case 2:
                        prev = nullptr;
                        break;
                case 3:
                        prev->clean_up();
                        prev = nullptr;
                        break;
                default:
                        // we shouldn't be there
                        std::terminate();
                }
        }

        return prev;
}

void
fiber_entity::interrupt()
{
        m_interrupted.store(true, std::memory_order::memory_order_release);
}

void
fiber_entity::clean_up()
{
        delete this;
}

void
fiber_entity::set_ready()
{
        m_scheduler->ready(*this);
}

void
fiber_entity::register_timer(scheduler::timer& t)
{
        t.fiber = this_fiber;
        sched.wait(t);
}

bool
fiber_entity::wait_until(std::chrono::steady_clock::time_point& deadline)
{
        scheduler::timer t;
        t.deadline = deadline;

        register_timer(t);
        
        self::resched();
        
        return deadline <= std::chrono::steady_clock::now();
}

void
fiber::create_entity(entry_container_t& entry_container)
{
        auto entity = new fiber_entity(entry_wrapper);
        fiber_entity::this_fiber->switch_to(reinterpret_cast<std::uint64_t>(&entry_container), reinterpret_cast<std::uint64_t>(fiber_entity::this_fiber), entity);

        // parent will be resumed only after child have finished its initialization
        
        this->m_entity.store(entity, std::memory_order_release);
}

void
fiber::entry_wrapper(std::uint64_t first, std::uint64_t second)
{
        // NORETURN: non trivially-destructable objects not allowed
        auto entry_container_ptr = reinterpret_cast<entry_container_t*>(first);
        auto parent_ptr = reinterpret_cast<fiber_entity*>(second);

        entry(entry_container_ptr, parent_ptr);
        self::resched();

        std::terminate();
}

void
fiber::entry(entry_container_t* entry_container_ptr, fiber_entity* parent_ptr)
{        
        auto entry_container = std::move(*entry_container_ptr);        
        // initialization finished: mark myself ready and switch back to parent

        fiber_entity::this_fiber->set_ready();
        fiber_entity::this_fiber->switch_to(0, reinterpret_cast<std::uint64_t>(fiber_entity::this_fiber), parent_ptr);

        // actually start running
        
        try {
                entry_container();
        } catch (const fiber_interruption&) {
        } catch (const _fiber_exit_unwind&) {
        } catch (...) {
                fiber_entity::this_fiber->m_eptr = std::current_exception();
        }

        if (fiber_entity::this_fiber->m_status_futex.word.fetch_add(2, std::memory_order::memory_order_acq_rel) == 0)
                fiber_entity::this_fiber->m_status_futex.wake(1);
        
}

fiber::~fiber()
{
        if (this->joinable()) {
                this->interrupt();
                this->join();
        }
}

suika::id
fiber::id()
{
        return m_entity.load(std::memory_order_acquire)->m_id;
}

bool
fiber::joinable()
{
        return m_entity.load(std::memory_order_acquire) != nullptr;
}

bool
fiber::interruption_enabled()
{
        return m_entity.load(std::memory_order_acquire)->m_interruption_enabled.load(std::memory_order_acquire);        
}

void
fiber::join()
{
        auto entity = m_entity.load(std::memory_order_acquire);
        if (!entity)
                throw std::invalid_argument{"fiber is not joinable"};
        
        entity->m_status_futex.wait(0);

        m_entity.store(nullptr, std::memory_order_release);

        if (entity->m_status_futex.word.exchange(3, std::memory_order_acquire) == 4)
                delete entity;       
}

void
fiber::detach()
{
        auto entity = m_entity.exchange(nullptr, std::memory_order_acq_rel);
        
        if (entity->m_status_futex.word.exchange(1, std::memory_order_acq_rel) == 4)
                delete entity;            
}

void
fiber::interrupt()
{
        auto entity = m_entity.load(std::memory_order_acquire);
        if (!entity)
                throw invalid_handle();

        entity->m_interrupted.store(true, std::memory_order_release);
        
        self::yield();
}

suika::id
self::id()
{
        return fiber_entity::this_fiber->m_id;
}

void
self::resched()
{
        auto next = sched.pick_next();
        fiber_entity::this_fiber->switch_to(0, reinterpret_cast<std::uint64_t>(fiber_entity::this_fiber), next);

        interruption_point();
}

void
self::yield()
{
        auto self = fiber_entity::this_fiber;
        sched.ready(*self);
        
        auto next = sched.pick_next();

        if (next != self)
                fiber_entity::this_fiber->switch_to(0, reinterpret_cast<std::uint64_t>(fiber_entity::this_fiber), next);

        interruption_point();
}

void
self::interruption_point()
{
        if (!fiber_entity::this_fiber->m_interruption_enabled.load(std::memory_order_acquire)) return;
        
        auto interrupted = false;
        interrupted = fiber_entity::this_fiber->m_interrupted.exchange(interrupted, std::memory_order_acq_rel);
        if (interrupted)
                throw fiber_interruption();
}

bool
self::interruption_requested()
{
        return fiber_entity::this_fiber->m_interrupted.load(std::memory_order_acquire);
}

bool
self::interruption_enabled()
{
        return fiber_entity::this_fiber->m_interruption_enabled.load(std::memory_order_acquire);
}

self::disable_interruption::disable_interruption():
        m_prev_state(fiber_entity::this_fiber->m_interruption_enabled.exchange(false, std::memory_order_acq_rel))
{
}

self::disable_interruption::~disable_interruption()
{
        fiber_entity::this_fiber->m_interruption_enabled.store(m_prev_state, std::memory_order_release);
}

self::restore_interruption::restore_interruption(disable_interruption& di)
{
        fiber_entity::this_fiber->m_interruption_enabled.store(di.m_prev_state, std::memory_order_release);
}

self::restore_interruption::~restore_interruption()
{
        fiber_entity::this_fiber->m_interruption_enabled.store(false, std::memory_order_release);
}

void
self::sleep_until(std::chrono::steady_clock::time_point deadline)
{
        fiber_entity::this_fiber->wait_until(deadline);
}

void
self::sleep_for(std::chrono::steady_clock::duration duration)
{
        sleep_until(duration + std::chrono::steady_clock::now());
}

