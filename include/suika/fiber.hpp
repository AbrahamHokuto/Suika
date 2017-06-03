#ifndef SKA_FIBER_HPP
#define SKA_FIBER_HPP

#include "io.hpp"
#include "scheduler.hpp"
#include "stack.hpp"
#include "context.hpp"
#include "blockpoint.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <exception>
#include <stdexcept>

namespace suika {
        using id = std::uint64_t;
        
        class fiber_interruption {};

        class fiber;

        struct fiber_entity: public list_base_hook<fiber_entity> {
                static thread_local fiber_entity* this_fiber;
                
                stack m_stack;
                context m_context;
                id m_id;
                scheduler *m_scheduler = nullptr;

                std::exception_ptr m_eptr = nullptr;

                std::atomic_bool m_interrupted{false};
                std::atomic_bool m_interruption_enabled{true};

                futex m_status_futex{1}; // 0: exited; 1: joinable; 2: detached 3: corpse
                
                constexpr static std::size_t default_stack_size = 4096;                

                fiber_entity();
                fiber_entity(context::entry_t entry, std::size_t stack_size = default_stack_size);

                fiber_entity* switch_to(std::uint64_t, std::uint64_t, fiber_entity* entity);
                void interrupt();

                void clean_up();

                void set_ready();

                void register_timer(scheduler::timer& t);
                bool wait_until(std::chrono::steady_clock::time_point& deadline);
        };
        
        class fiber {
        private:                
                using entry_container_t = std::function<void()>;

                static void entry(entry_container_t*, fiber_entity*);                
                static void entry_wrapper(std::uint64_t, std::uint64_t);
                
        private:
                std::atomic<fiber_entity*> m_entity{nullptr};

                void create_entity(entry_container_t& entry);

        // protected:
        //         virtual void pre_yield_hook();
        //         virtual void post_yield_hook();

        public:
                fiber() = default;
                fiber(const fiber&) = delete;
                fiber(fiber&& rhs) noexcept
                {
                        auto entity = rhs.m_entity.exchange(nullptr, std::memory_order_acq_rel);
                        m_entity.store(entity, std::memory_order_release);
                }

                fiber&
                operator=(fiber&& rhs)
                {
                        if (this->joinable())
                                throw std::runtime_error("fiber object already occupied");

                        auto entity = rhs.m_entity.exchange(nullptr, std::memory_order_acq_rel);
                        m_entity.store(entity, std::memory_order_release);

                        return *this;
                }

                ~fiber();

                void
                swap(fiber& rhs)
                {
                        auto tmp = rhs.m_entity.exchange(nullptr, std::memory_order_acq_rel);
                        rhs.m_entity.exchange(m_entity.exchange(tmp, std::memory_order_acq_rel),
                                              std::memory_order_acq_rel);                        
                }

                template <typename callable, typename... args_t>
                explicit fiber(callable&& f, args_t&&... args)
                {
                        entry_container_t entry([&f, &args...](){ std::invoke(std::forward<callable>(f), std::forward<args_t>(args)...); });
                        create_entity(entry);
                }

                suika::id id();

                bool joinable();
                bool interruption_enabled();
                
                void join();
                void detach();
                void interrupt();
        };

        namespace self {
                suika::id id();

                void resched();
                void yield();

                void interruption_point();
                bool interruption_requested();
                bool interruption_enabled();

                struct disable_interruption {
                        bool m_prev_state;

                        disable_interruption();
                        ~disable_interruption();
                };

                struct restore_interruption {
                        restore_interruption(disable_interruption&);
                        ~restore_interruption();
                };

                void sleep_until(std::chrono::steady_clock::time_point deadline);
                void sleep_for(std::chrono::steady_clock::duration duration);

                template <typename _clock, typename _duration>
                void sleep_until(std::chrono::time_point<_clock, _duration>& deadline)
                {
                        sleep_for(std::chrono::duration_cast<std::chrono::steady_clock::duration>(deadline - _clock::now()));
                }

                template <typename _rep, typename _period>
                void sleep_for(std::chrono::duration<_rep, _period>& duration)
                {
                        sleep_for(std::chrono::duration_cast<std::chrono::steady_clock::duration>(duration));
                }
        };
}

#endif  // SKA_FIBER_HPP
