#include <suika/blockpoint.hpp>
#include <suika/fiber.hpp>

#include <iostream>

void
fiber(suika::futex* f)
{
        std::cout << "fiber main" << std::endl;
        f->wait(0);
        std::cout << "fiber main" << std::endl;
}

int
main()
{
        suika::futex f;        
        f.word.store(0, std::memory_order_release);

        std::cout << "main" << std::endl;
        suika::fiber fib(fiber, &f);
        std::cout << "main" << std::endl;
        f.wake(1);
        suika::self::yield();
        std::cout << "main" << std::endl;

        std::string line;
        std::getline(std::cin, line);
}
