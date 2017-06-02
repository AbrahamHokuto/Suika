#include <suika/fiber.hpp>

#include <iostream>

void
fiber()
{
        std::cout << "fiber main" << std::endl;
}

int
main()
{
        std::cout << "main" << std::endl;
        suika::fiber f(fiber);
        std::cout << "main" << std::endl;
        f.join();
}
