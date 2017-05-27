#include <suika/fiber.hpp>
#include <iostream>

void
fiber_main(std::chrono::steady_clock::duration d, int n)
{
        std::cout << "fiber #" << n << " started" << std::endl;
        suika::self::sleep_for(d);
        std::cout << "fiber #" << n << " resumed" << std::endl;
}

int
main()
{
        using namespace std::literals::chrono_literals;        
        suika::fiber f1(fiber_main, 5s, 1);
        suika::fiber f2(fiber_main, 10s, 2);

        f1.join();
        f2.join();
}
