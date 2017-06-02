#include <suika/fiber.hpp>

void
test()
{       
        try {
                throw std::runtime_error("test");
        } catch (const std::exception& e) {                
        }
}

int
main()
{
        suika::fiber f(test);
        f.join();
        throw std::runtime_error("test");
}
