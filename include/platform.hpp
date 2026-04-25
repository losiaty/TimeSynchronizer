#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
    #pragma comment(lib, "Mswsock.lib")
    #pragma comment(lib, "AdvApi32.lib")
    
    #define close_socket closesocket
    #define SOCKET_ERROR_VAL SOCKET_ERROR
    #define INVALID_SOCKET_VAL INVALID_SOCKET
    
    inline uint64_t htobe64(uint64_t x) {
        return ((uint64_t)htonl((uint32_t)(x >> 32)) << 32) | htonl((uint32_t)x);
    }
    
    inline uint64_t be64toh(uint64_t x) {
        return ((uint64_t)ntohl((uint32_t)(x >> 32)) << 32) | ntohl((uint32_t)x);
    }
    
    class WinsockInitializer {
    public:
        WinsockInitializer() {
            WSADATA wsa_data;
            int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
            if (result != 0) {
                throw std::runtime_error("WSAStartup failed");
            }
        }
        
        ~WinsockInitializer() {
            WSACleanup();
        }
    };
    
#else  // Linux
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <endian.h>
    
    #define close_socket close
    #define SOCKET_ERROR_VAL -1
    #define INVALID_SOCKET_VAL -1
    
    class WinsockInitializer {
    public:
        WinsockInitializer() { }
        ~WinsockInitializer() { }
    };
#endif

#endif // PLATFORM_HPP
