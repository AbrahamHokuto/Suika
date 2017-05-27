#include <suika/rbtree.hpp>

#include <random>
#include <vector>
#include <iostream>
#include <cstdlib>

struct entry: public suika::rbtree_base_hook<entry> {
        uint32_t data;

        bool operator<(const entry& rhs) const noexcept { return this->data < rhs.data; }

        entry(uint32_t v): data(v) {}
};

const char*
color_seq(bool is_red) noexcept
{
        return is_red ? "\033[31;1m" : "";
}

const char* color_reset = "\033[0m";

void
indent(std::size_t height)
{
        for (std::size_t i = 0; i < height; ++i)
                std::cout << "  "; // two spaces
}

void
print_tree(entry* e, std::size_t height = 1, std::size_t black_count = 0, entry* p = nullptr)
{
        if (!e) {
                std::cout << "L " << black_count + 1;
                return;
        }
        
        if (p && e->parent != p) {
                std::printf("invalid parent pointer: p = %p, *p = %u, e->parent = %p, *e = %u\n", p, p->data, e->parent, e->data);
                // std::terminate();
        }

        std::cout << color_seq(e->is_red) << e->data << color_reset << " ";

        print_tree(&e->left->container(), height + 1, e->is_red ? black_count : black_count + 1, e);

        std::cout << std::endl;
        indent(height);
        print_tree(&e->right->container(), height + 1, e->is_red ? black_count : black_count + 1, e);
}

int
main()
{
        suika::rbtree<entry> tree;
        
        std::vector<entry> container;

        std::size_t count = 9;

        for (std::size_t i = 0; i < count; ++i)
                container.emplace_back(i + 1);

        std::random_shuffle(container.begin(), container.end());

        for (auto& i: container) {
                std::cout << "inserting " << i.data << std::endl;
                tree.insert(i);
                print_tree(&tree.root());
                std::cout << std::endl << std::endl;
        }

        for (auto& i: container) {
                std::cout << "removing " << i.data << std::endl;
                i.remove();
                print_tree(&tree.root());
                std::cout << std::endl << std::endl;
        }
}
