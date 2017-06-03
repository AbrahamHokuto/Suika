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
        f.word.store(1, std::memory_order_release);
        f.wake(1);
        std::cout << "main" << std::endl;

        fib.join();
}
