#ifndef SKA_LIST_HPP
#define SKA_LIST_HPP

#include <cstdint>

namespace suika {
        template <typename _elem_t> class list;
        
        template <typename _elem_t>
        class list_base_hook {
        private:
                list_base_hook *prev = this;
                list_base_hook *next = this;           

                friend list<_elem_t>;
                
        public:
                bool isolated() const noexcept { return prev == this && next == this; }

                void link(list_base_hook& before) { unlink(); prev = &before; next = before.next; next->prev = this; before.next = this; }                
                void unlink() { prev->next = next; next->prev = prev; prev = next = this; }
                
                ~list_base_hook() { unlink(); }
        };

        template <typename _elem_t>
        class list {
        private:
                using ctl_t = list_base_hook<_elem_t>;
                ctl_t head_node;
                
        public:
                bool empty() { return head_node.isolated(); }
                
                void insert_head(ctl_t& e) { e.link(head_node); }
                void insert_tail(ctl_t& e) { e.link(*head_node.prev); }

                _elem_t& head() { return *static_cast<_elem_t*>(head_node.next); }
                _elem_t& tail() { return *static_cast<_elem_t*>(head_node.prev); }

                std::size_t size()
                {
                        std::size_t ret = 0;

                        for (ctl_t* i = head_node.next; i != &head_node; i = i->next)
                                ++ret;

                        return ret;
                }
        };
}

#endif  // SKA_LIST_HPP
