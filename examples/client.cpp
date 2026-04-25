#include "platform.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <cstring>
#include <iomanip>

struct NetworkPacket {
    uint32_t device_id;
    uint64_t timestamp;
    uint8_t format;
    uint64_t frequency_hz;
    uint8_t padding[4];
} __attribute__((packed));

class SimulatedDevice {
private:
    uint32_t device_id_;
    int socket_fd_;
    std::string server_ip_;
    int server_port_;
    
    std::chrono::steady_clock::time_point start_device_;
    std::chrono::system_clock::time_point start_system_;
    double clock_drift_;
    int64_t clock_offset_ms_;
    bool running_;

public:
    SimulatedDevice(uint32_t id, const std::string& server_ip, int server_port,
                   double drift, int64_t offset_ms)
        : device_id_(id), server_ip_(server_ip), server_port_(server_port),
          clock_drift_(drift), clock_offset_ms_(offset_ms), running_(false)
    {
        socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_fd_ < 0) {
            throw std::runtime_error("Cannot create socket");
        }
        
        start_device_ = std::chrono::steady_clock::now();
        start_system_ = std::chrono::system_clock::now();
        
        std::cout << "Device " << std::setw(2) << id 
                 << " created (drift=" << std::fixed << std::setprecision(2) 
                 << drift << ", offset=" << std::setw(4) << offset_ms << "ms)"
                 << std::endl;
    }
    
    ~SimulatedDevice() {
        close_socket(socket_fd_);
    }
    
    uint64_t GetDeviceTime() {
        auto now_steady = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now_steady - start_device_
        ).count();
        
        auto elapsed_with_drift = static_cast<int64_t>(elapsed * clock_drift_);
        
        auto system_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            start_system_.time_since_epoch()
        ).count();
        
        return system_ms + elapsed_with_drift + clock_offset_ms_;
    }
    
    void SendSample() {
        NetworkPacket packet = {};
        packet.device_id = htonl(device_id_);
        packet.timestamp = htobe64(GetDeviceTime());
        packet.format = 2;
        packet.frequency_hz = 0;
        
        struct sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(server_port_);
        
#ifdef _WIN32
        inet_pton(AF_INET, server_ip_.c_str(), &addr.sin_addr.s_addr);
#else
        inet_aton(server_ip_.c_str(), &addr.sin_addr);
#endif
        
        sendto(socket_fd_, (const char*)&packet, sizeof(packet), 0,
               (struct sockaddr*)&addr, sizeof(addr));
    }
    
    void Run(int duration_sec) {
        running_ = true;
        auto start = std::chrono::system_clock::now();
        int sample_count = 0;
        
        std::cout << "  Device " << device_id_ << " sending for " 
                 << duration_sec << " seconds..." << std::endl;
        
        while (running_) {
            auto now = std::chrono::system_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
            
            if (elapsed >= duration_sec) {
                break;
            }
            
            SendSample();
            sample_count++;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        std::cout << "  Device " << device_id_ << " sent " << sample_count 
                 << " samples" << std::endl;
    }
};

int main() {
    try {
        WinsockInitializer wsa;
        
        std::cout << "=" << std::string(50, '=') << std::endl;
        std::cout << "  Time Synchronizer - Client Simulator" << std::endl;
        std::cout << "=" << std::string(50, '=') << std::endl;
        std::cout << "\nStarting " << 2 << " simulated devices..." << std::endl << std::endl;
        
        std::vector<std::shared_ptr<SimulatedDevice>> devices;
        std::vector<std::thread> threads;
        
        devices.push_back(std::make_shared<SimulatedDevice>(
            1, "127.0.0.1", 12345, 1.03, -500
        ));
        
        devices.push_back(std::make_shared<SimulatedDevice>(
            2, "127.0.0.1", 12345, 0.98, 200
        ));
        
        std::cout << "\nStarting transmission threads...\n" << std::endl;
        
        for (auto& dev : devices) {
            threads.emplace_back([dev]() { dev->Run(60); });
        }
        
        std::cout << "Waiting for completion..." << std::endl;
        
        for (auto& t : threads) {
            t.join();
        }
        
        std::cout << "\n" << std::string(50, '=') << std::endl;
        std::cout << "Test completed successfully!" << std::endl;
        std::cout << std::string(50, '=') << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
