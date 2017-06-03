#include <suika/fiber.hpp>
#include <iostream>

void
test()
{       
        throw std::runtime_error("test");
}

int
main()
{
        suika::fiber f(test);

        try {
                f.join();
        } catch (const std::exception& e) {
                std::cout << "exception from child fiber: " << e.what() << std::endl;
        }
}
