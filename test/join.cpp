#include <suika/fiber.hpp>

#include <iostream>

void
fiber()
{
        std::cout << "fiber main" << std::endl;
        suika::self::yield();
        std::cout << "fiber main" << std::endl;        
}

int
main()
{
        std::cout << "main" << std::endl;
        suika::fiber fib(fiber);
        std::cout << "main" << std::endl;
        fib.join();
        std::cout << "main" << std::endl;
}
