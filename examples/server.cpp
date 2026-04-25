#include "TimeSynchronizer.hpp"
#include "platform.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <iomanip>

#ifdef _WIN32
    typedef int socklen_t;
#endif

struct NetworkPacket {
    uint32_t device_id;
    uint64_t timestamp;
    uint8_t format;
    uint64_t frequency_hz;
    uint8_t padding[4];
} __attribute__((packed));

class UDPServer {
private:
    int socket_fd_;
    int port_;
    TimeSynchronizer sync_;
    bool running_;

public:
    UDPServer(int port) : port_(port), running_(false) {
        socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_fd_ < 0) {
            throw std::runtime_error("Cannot create socket");
        }
        
        int reuse = 1;
        setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, 
                  (const char*)&reuse, sizeof(reuse));
        
        struct sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);
        
        if (bind(socket_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            throw std::runtime_error("Cannot bind socket");
        }
        
        std::cout << "Server listening on port " << port << std::endl;
    }
    
    ~UDPServer() {
        close_socket(socket_fd_);
    }
    
    void RegisterDevice(const DeviceConfig& config) {
        sync_.RegisterDevice(config);
        std::cout << "  Device " << config.device_id << " registered" << std::endl;
    }
    
    void Run() {
        running_ = true;
        struct sockaddr_in client_addr = {};
        socklen_t client_len = sizeof(client_addr);
        NetworkPacket packet;
        int packet_count = 0;
        
        std::cout << "\nWaiting for packets...\n" << std::endl;
        
        while (running_) {
            int n = recvfrom(socket_fd_, (char*)&packet, sizeof(packet), 0,
                           (struct sockaddr*)&client_addr, &client_len);
            
            if (n < 0) {
                break;
            }
            
            uint32_t device_id = ntohl(packet.device_id);
            uint64_t ts_device = be64toh(packet.timestamp);
            
            Timestamp ts_server;
            ProcessResult result = sync_.Process(ts_device, device_id, ts_server);
            
            double drift, offset;
            sync_.GetSyncParameters(device_id, drift, offset);
            
            packet_count++;
            
            std::cout << "[" << std::setw(5) << packet_count << "] "
                     << "Dev " << std::setw(2) << device_id 
                     << " | ts_device=" << std::setw(15) << ts_device
                     << " | ts_server=" << std::setw(15) << ts_server
                     << " | drift=" << std::fixed << std::setprecision(6) << drift
                     << " | offset=" << std::setw(15) << (int64_t)offset << " ns"
                     << " | status=" << (int)result
                     << std::endl;
        }
    }
    
    void Stop() {
        running_ = false;
    }
};

int main() {
    try {
        WinsockInitializer wsa;
        
        std::cout << "=" << std::string(70, '=') << std::endl;
        std::cout << "  Time Synchronizer - UDP Server" << std::endl;
        std::cout << "=" << std::string(70, '=') << std::endl << std::endl;
        
        UDPServer server(12345);
        
        DeviceConfig dev1;
        dev1.device_id = 1;
        dev1.format = DeviceTimestampFormat::MILLISECONDS_SINCE_EPOCH;
        dev1.calibration_window = 50;
        server.RegisterDevice(dev1);
        
        DeviceConfig dev2;
        dev2.device_id = 2;
        dev2.format = DeviceTimestampFormat::MILLISECONDS_SINCE_EPOCH;
        dev2.calibration_window = 50;
        server.RegisterDevice(dev2);
        
        server.Run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
