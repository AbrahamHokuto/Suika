#ifndef SKA_FIBER_HPP
#define SKA_FIBER_HPP

#include "stack.hpp"
#include "context.hpp"
#include "blockpoint.hpp"

#include <atomic>
#include <memory>
#include <functional>

namespace suika {
        using id = std::uint64_t;
        
        class fiber_interruption {};

        class scheduler;
        
        class fiber_entity: public list_base_hook<fiber_entity> {
        public:
                static thread_local fiber_entity* this_fiber;
                
                stack m_stack;
                context m_context;
                id m_id;
                scheduler *m_scheduler;

                futex m_running_futex; // 1 for running
                
                std::atomic_bool m_detached{false};
                std::atomic_bool m_interrupted{false};
                std::atomic_bool m_interruption_enabled{true};

                std::atomic<std::size_t> m_joiner_count{0};

                constexpr static std::size_t default_stack_size = 4096;

                fiber_entity();
                fiber_entity(context::entry_t entry, std::size_t stack_size = default_stack_size);

                fiber_entity* switch_to(std::uint64_t, std::uint64_t, fiber_entity* entity);
                void interrupt();

                void clean_up();

                void set_ready();
        };
        
        class fiber {
        private:
                using entry_container_t = std::function<void()>;

                static void entry(entry_container_t*, fiber*);                
                static void entry_wrapper(std::uint64_t, std::uint64_t);
                
        private:
                std::atomic<fiber_entity*> m_entity{nullptr};

                void create_entity(entry_container_t& entry);
                fiber_entity* safe_entity_access();

        // protected:
        //         virtual void pre_yield_hook();
        //         virtual void post_yield_hook();

        public:
                fiber() = default;
                fiber(const fiber&) = delete;
                fiber(fiber&& rhs) noexcept
                {
                        fiber_entity* tmp = nullptr;
                        rhs.m_entity.exchange(tmp, std::memory_order_acq_rel);
                        m_entity.store(tmp, std::memory_order_release);
                }

                fiber&
                operator=(fiber&& rhs)
                {
                        if (this->joinable()) {
                                this->interrupt();
                                this->join();
                        }

                        fiber_entity* tmp = nullptr;
                        rhs.m_entity.exchange(tmp, std::memory_order_acq_rel);
                        m_entity.store(tmp, std::memory_order_release);

                        return *this;
                }

                ~fiber();

                template <typename callable, typename... args_t>
                explicit fiber(callable&& f, args_t&&... args)
                {
                        entry_container_t entry(std::bind(f, std::forward<args_t>(args)...));
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
        };
}

#endif  // SKA_FIBER_HPP
