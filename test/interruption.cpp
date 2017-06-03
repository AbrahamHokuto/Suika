#include <suika/fiber.hpp>

#include <iostream>

void
fiber()
{
        suika::self::disable_interruption _di{};
        suika::self::yield();
        
        try {
                suika::self::restore_interruption _ri(_di);
                suika::self::interruption_point();
        } catch (const suika::fiber_interruption&) {
                std::cout << "interrupted" << std::endl;
        }

        try {
                suika::self::interruption_point();
        } catch (const suika::fiber_interruption&) {
                std::cout << "ambiguous interruption" << std::endl;
        }
}

int
main()
{
        std::cout << "main" << std::endl;
        suika::fiber fib(fiber);
        fib.interrupt();
        std::cout << "main" << std::endl;
        fib.join();        
}
