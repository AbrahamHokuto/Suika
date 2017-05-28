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
        
        if (prev->m_running_futex.word.load(std::memory_order_acquire)) {
                return prev;
        } else {
                prev->clean_up();
                return nullptr;
        }
}

void
fiber_entity::interrupt()
{
        m_interrupted.store(true, std::memory_order::memory_order_release);
}

void
fiber_entity::clean_up()
{
        if (m_detached.load(std::memory_order::memory_order_acquire))
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
        sched.ready(*fiber_entity::this_fiber);
        entity->m_running_futex.word.store(1, std::memory_order_release);
        fiber_entity::this_fiber->switch_to(reinterpret_cast<std::uint64_t>(&entry_container), reinterpret_cast<std::uint64_t>(this), entity);        
        // control flow won't return until child finishing its initialization        
        this->m_entity.store(entity, std::memory_order_release);
}

void
fiber::entry_wrapper(std::uint64_t first, std::uint64_t second)
{
        // this function does not return, so do not create any local object that is not trivially destructible.
        auto entry_container_ptr = reinterpret_cast<entry_container_t*>(first);
        auto this_ptr = reinterpret_cast<fiber*>(second);

        entry(entry_container_ptr, this_ptr);
        self::resched();

        std::terminate();
}

void
fiber::entry(entry_container_t* entry_container_ptr, fiber*)
{
        auto entry_container = std::move(*entry_container_ptr);

        try {
                entry_container();
        } catch (const fiber_interruption&) {
        } catch (const _fiber_exit_unwind&) {
        } catch (const std::exception& e) {
                // TODO: uncaught exception msg
        } catch (...) {
                // TODO: uncaught exception msg
        }

        fiber_entity::this_fiber->m_running_futex.word.store(0, std::memory_order::memory_order_release);
        fiber_entity::this_fiber->m_running_futex.wake(std::numeric_limits<std::size_t>::max());
}

fiber::~fiber()
{
        if (this->joinable()) {
                this->interrupt();
                this->join();
        }
}

fiber_entity*
fiber::safe_entity_access()
{
        auto entity_ptr = m_entity.load(std::memory_order::memory_order_acquire);
        if (!entity_ptr)
                return nullptr;

        if (entity_ptr->m_detached.load(std::memory_order::memory_order_acquire))
                return nullptr;

        return entity_ptr;
}

suika::id
fiber::id()
{
        return safe_entity_access()->m_id;
}

bool
fiber::joinable()
{
        return safe_entity_access() != nullptr;
}

bool
fiber::interruption_enabled()
{
        return safe_entity_access()->m_interruption_enabled.load(std::memory_order_acquire);        
}

void
fiber::join()
{
        auto entity = safe_entity_access();
        if (!entity)
                return;

        // TODO: fiber synchronozation mechanics

        entity->m_joiner_count.fetch_add(1, std::memory_order_acq_rel);
        entity->m_running_futex.wait(1);

        entity = safe_entity_access();
        if (!entity)
                return;

        if (entity->m_joiner_count.fetch_sub(1, std::memory_order_acq_rel) > 1)
                return;

        m_entity.store(nullptr, std::memory_order_release);
        delete entity;
}

void
fiber::detach()
{
        auto entity = safe_entity_access();
        if (!entity)
                return;

        entity->m_detached.store(true, std::memory_order_release);
        m_entity.store(nullptr, std::memory_order_release);

        if (entity->m_joiner_count.load(std::memory_order_acquire))
                entity->m_running_futex.wake(std::numeric_limits<std::size_t>::max());
        else
                delete entity;
}

void
fiber::interrupt()
{
        auto entity = safe_entity_access();
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
        auto next = sched.pick_next();
        
        sched.ready(*fiber_entity::this_fiber);

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

