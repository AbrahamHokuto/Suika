#include <suika/fiber.hpp>

#include <iostream>

void
fiber(std::unique_ptr<int> obj)
{
        std::cout << *obj << std::endl;;
}

int
main()
{
        suika::fiber fib(fiber, std::make_unique<int>(0));
        fib.join();
}
