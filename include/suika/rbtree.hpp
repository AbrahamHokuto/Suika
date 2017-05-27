#ifndef SKA_RBTREE_HPP
#define SKA_RBTREE_HPP

#include <algorithm>
#include <functional>

namespace suika {
        // template <typename _value_type, typename comparator = std::less<_value_type>> rbtree;

        template <typename _value_type>
        struct rbtree_base_hook {
                using value_type = _value_type;
                
                rbtree_base_hook* parent = nullptr;
                rbtree_base_hook* left = nullptr;
                rbtree_base_hook* right = nullptr;

                bool is_red = true;
                
                rbtree_base_hook() = default;
                ~rbtree_base_hook() { if (!isolated()) remove(); }

                value_type& container() { return *static_cast<value_type*>(this); }

                bool isolated() { return !parent; }

                static rbtree_base_hook*
                fin_max(rbtree_base_hook* n)
                {
                        while (n->right) n = n->right;
                        return n;
                }

                static rbtree_base_hook*
                find_min(rbtree_base_hook* n)
                {
                        while (n->left) n = n->left;
                        return n;
                }

                static void
                rotate_left(rbtree_base_hook* p)
                {
                        auto n = p->right;
                        auto g = p->parent;
                        auto c = n->left;

                        p->right = c;
                        n->left = p;
                        
                        n->parent = g;                        
                        p->parent = n;

                        if (c)
                                c->parent = p;

                        if (p == g->left)
                                g->left = n;
                        else
                                g->right = n;
                }

                static void
                rotate_right(rbtree_base_hook* p)
                {
                        auto n = p->left;
                        auto g = p->parent;
                        auto c = n->right;

                        p->left = c;
                        n->right = p;
                        
                        n->parent = g;
                        p->parent = n;
                        
                        if (c)
                                c->parent = p;

                        if (p == g->left)
                                g->left = n;
                        else
                                g->right = n;
                }

                static void
                do_insert(rbtree_base_hook* n)
                {
                        while (true)
                        {
                                auto p = n->parent;
                                auto g = p->parent;
                                auto u = g ? (p == g->left ? g->right : g->left) : nullptr;

                                // Case 1: n is root
                                if (!g) {
                                        n->is_red = false;
                                        break;
                                }

                                // Case 2: p is black
                                if (!p->is_red) break;

                                // Case 3: p is red and u is red
                                if (u && u->is_red) {
                                        p->is_red = false;
                                        u->is_red = false;

                                        g->is_red = true;
                                        n = g;
                                        continue;
                                }

                                // Case 4: p is red and u is black, n == p->right and p == g->left
                                if (n == p->right && p == g->left) {
                                        rotate_left(p);
                                        
                                        n = n->left;
                                        p = n->parent;
                                        g = p->parent;
                                } else if (n == p->left && p == g->right) { // or the mirror of above case
                                        rotate_right(p);
                                        
                                        n = n->right;
                                        p = n->parent;
                                        g = p->parent;
                                }

                                // Case 5: p is red and u is black, n == p->left and p == g->left or the mirror
                                p->is_red = false;
                                g->is_red = true;

                                if (n == p->left)
                                        rotate_right(g);
                                else
                                        rotate_left(g);
                        }
                }

                static void
                remove_rebalance(rbtree_base_hook* n)
                {
                        auto p = n->parent;
                        auto s = (n == p->left) ? p->right : p->left;
                        
                        while (true) {                                
                                // if n is the new root: we are done
                                if (!p->parent)
                                        return;                               
                                
                                // if s is red: reverse the colors of p and s, then rotate left at p (assuming n is the left child of p)
                                if (s && s->is_red) {
                                        std::swap(p->is_red, s->is_red);
                                        
                                        if (n == p->left)
                                                rotate_left(p);
                                        else                                       
                                                rotate_right(p);

                                        p = n->parent;
                                        s = (n == p->left) ? p->right : p->left;
                                }

                                // if p, s, and s's children are black: repaint s red, and relabance on p
                                if (!p->is_red && !s->is_red && !(s->left && s->left->is_red) && !(s->right && s->right->is_red)) {
                                        s->is_red = true;
                                        n = p;
                                        p = n->parent;
                                        s = (n == p->left) ? p->right : p->left;
                                        continue;                                        
                                }

                                break;
                        }

                        // if s and s's children are black, but p is red: swap the colors of s and p
                        if (p->is_red && !s->is_red && !(s->left && s->left->is_red) && !(s->right && s->right->is_red)) {
                                p->is_red = false;
                                s->is_red = true;
                        }

                        // if n is the left child, s is black, s's left children is red, s's right children is black: rotate right at s, then exchange the colors of s and it's new parent
                        if ((s->left && s->left->is_red) && n == p->left) {
                                rotate_right(s);
                                std::swap(s->is_red, s->parent->is_red);                                
                        } else if ((s->right && s->right->is_red) && n == p->right)  { // or the mirror of above case
                                rotate_left(s);
                                std::swap(s->is_red, s->parent->is_red);
                        }

                        p = n->parent;
                        s = (n == p->left) ? p->right : p->left;

                        // if n is the left child, s is black, s's right children is red: rotate left at p, then exchange the colors of s and p, and make the right child of s black
                        if ((s->right && s->right->is_red) && n == p->left) {
                                rotate_left(p);
                                std::swap(p->is_red, s->is_red);
                                s->right->is_red = false;
                        } else if ((s->left && s->left->is_red) && n == p->right) { // or the mirror of above case
                                rotate_right(p);
                                std::swap(p->is_red, s->is_red);
                                s->left->is_red = false;
                        }
                }

                static void
                do_remove(rbtree_base_hook* m)
                {
                        auto p = m->parent;

                        // if m have two non-leaf child: replace m with min element in its right subtree
                        if (m->left && m->right) {
                                auto c = find_min(m->right);

                                std::swap(m->is_red, c->is_red);

                                if (c != m->left) {
                                        std::swap(m->left, c->left);
                                } else {
                                        m->left = c->left;
                                        c->left = m;
                                }

                                if (c != m->right) {
                                        std::swap(m->right, c->right);
                                } else {
                                        m->right = c->right;
                                        c->right = m;
                                }

                                if (c->parent != m) {
                                        std::swap(m->parent, c->parent);
                                } else {
                                        c->parent = m->parent;
                                        m->parent = c;
                                }

                                if (m == p->left)
                                        p->left = c;
                                else
                                        p->right = c;
                                
                                if (m->parent != c && c == m->parent->left)
                                        m->parent->left = m;
                                else if (m->parent != c)
                                        m->parent->right = m;

                                if (m->left)
                                        m->left->parent = m;
                                if (m->right)
                                        m->right->parent = m;

                                c->left->parent = c;
                                c->right->parent = c;

                                p = m->parent;
                        }
                        
                        auto c = m->left ? m->left : m->right;
                        
                        // if m is red or c is not a null leaf: replace c into m
                        if (m -> is_red || c) {
                                if (c) c->parent = p;
                                if (p->left == m)
                                        p->left = c;
                                else                                
                                        p->right = c;

                                if (m->is_red)
                                        return;

                                if (c->is_red) {
                                        c->is_red = false;
                                        return;
                                }
                        }

                        if (c) {
                                remove_rebalance(c);
                        } else {
                                remove_rebalance(m);

                                if (m == m->parent->left)
                                        m->parent->left = nullptr;
                                else
                                        m->parent->right = nullptr;
                        }

                        m->parent = m->left = m->right = nullptr;
                        m->is_red = true;                        
                }

                void
                remove()
                {
                        do_remove(this);
                }
        };

        template <typename _value_type, typename comparator = std::less<_value_type>>
        class rbtree {
        public:
                using value_type = _value_type;
                using node = rbtree_base_hook<value_type>;

        private:
                node head;

        public:
                bool empty() { return !head.left; }

                value_type& max() { return node::find_max(head.left)->container(); }
                value_type& min() { return node::find_min(head.left)->container(); }

                value_type& root() { return head.left->container(); }

                void
                insert(value_type& v) {                        
                        auto n = static_cast<node*>(&v);

                        if (empty()) {
                                // n is the new root
                                head.left = n;
                                n->parent = &head;
                        } else {
                                auto p = head.left;
                                comparator cmp{};
                                
                                // find parent for new node
                                while (true) {
                                        if (cmp(p->container(), v)) {
                                                if (p->right) {
                                                        p = p->right;
                                                }else {
                                                        p->right = n;
                                                        break;
                                                }
                                        } else {
                                                if (p->left) {
                                                        p = p->left;
                                                } else {
                                                        p->left = n;
                                                        break;
                                                }
                                        }                                
                                }
                                
                                n->parent = p;
                        }
                        
                        node::do_insert(n);
                }
        };
}

#endif  // SKA_RBTREE_HPP
