#include <suika/context.hpp>
#include <suika/stack.hpp>

#include <iostream>

suika::context main_ctx, test_ctx;

void
test(std::uint64_t, std::uint64_t)
{
        std::cout << "test" << std::endl;
        test_ctx.swap(0, 0, main_ctx);
        std::cout << "test" << std::endl;
        test_ctx.swap(0, 0, main_ctx);
}

int
main()
{
        suika::stack test_stack(4096);
        test_ctx = suika::context(test, reinterpret_cast<std::uint64_t>(test_stack.sp()));
        
        std::cout << "main" << std::endl;
        main_ctx.swap(0, 0, test_ctx);
        std::cout << "main" << std::endl;
        main_ctx.swap(0, 0, test_ctx);
        std::cout << "main" << std::endl;
}
