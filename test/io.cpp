#include <list>
#include <iostream>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <suika/fiber.hpp>

struct socket_t {
        int fd = -1;

        socket_t(int fd): fd(fd) {}
        socket_t(socket_t&& rhs): fd(rhs.fd) { rhs.fd = -1; };
        ~socket_t() { ::shutdown(fd, SHUT_RDWR); close(fd); }
};

void
fiber_main(int _sock, sockaddr_in addr)
{
        socket_t sock(_sock);
        using namespace std::literals::chrono_literals;
        char buf[4096];

        try {                
                while (true) {
                        ssize_t bytes_to_send = 0;
                
                        while (true) {                        
                                if (!suika::self::wait_oneshot(sock.fd, suika::io::masks::read, 5000ms))
                                        throw std::runtime_error("recv timeout"); 
                                
                
                                auto ret = recv(sock.fd, buf, 4096, MSG_NOSIGNAL);
                        
                                if (ret == 0) {
                                        throw std::runtime_error("client closed");
                                }


                                if (ret < 0) {
                                        if (errno != EAGAIN)
                                                throw std::system_error(errno, std::system_category());
                                        else
                                                continue;
                                }

                                bytes_to_send = ret;
                                break;
                        }
                
                        while (bytes_to_send) {
                                if (!suika::self::wait_oneshot(sock.fd, suika::io::masks::write, 5000ms))
                                        throw std::runtime_error("send timeout");

                                auto ret = send(sock.fd, buf, bytes_to_send, MSG_NOSIGNAL);

                                if (ret == 0)
                                        throw std::runtime_error("client closed");

                                if (ret < 0) {
                                        if (errno != EAGAIN)
                                                throw std::system_error(errno, std::system_category());
                                        else
                                                continue;
                                }

                                bytes_to_send -= ret;
                        }
                }
        } catch (const std::exception& e) {                
                std::cout << "exiting: " << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << " :" << e.what() << std::endl;
        }
}

int
main()
{
        auto lsock = ::socket(AF_INET, SOCK_STREAM, 0);
        if (lsock < 0)
                throw std::system_error(errno, std::system_category());

        int val = 1;
        if (::setsockopt(lsock, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val)) < 0)
                throw std::system_error(errno, std::system_category());        

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(1919);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(lsock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
                throw std::system_error(errno, std::system_category());

        if (listen(lsock, 128) < 0)
                throw std::system_error(errno, std::system_category());

        std::cout << "listening" << std::endl;        
        
        while (true) {
                suika::self::wait_oneshot(lsock, suika::io::masks::read);

                struct sockaddr_in c_addr;
                socklen_t addrlen;

                auto ret = accept4(lsock, reinterpret_cast<sockaddr*>(&c_addr), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);             

                if (ret < 0)
                        throw std::system_error(errno, std::system_category());

                std::cout << "accepted: " << ::inet_ntoa(c_addr.sin_addr) << ":" << ntohs(c_addr.sin_port) << std::endl;
                
                suika::fiber f(fiber_main, ret, c_addr);
                f.detach();
        }
}
