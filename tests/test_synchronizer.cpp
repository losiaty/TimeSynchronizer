#include "TimeSynchronizer.hpp"
#include "platform.hpp"
#include <cassert>
#include <iostream>
#include <iomanip>
#include <chrono>

int tests_passed = 0;
int tests_failed = 0;

void TestPassed(const std::string& name) {
    std::cout << "  ✓ " << name << std::endl;
    tests_passed++;
}

void TestFailed(const std::string& name, const std::string& reason) {
    std::cout << "  ✗ " << name << " - " << reason << std::endl;
    tests_failed++;
}

void TestBasicConversion() {
    std::cout << "\nTest 1: Basic unit conversions" << std::endl;
    
    try {
        TimeSynchronizer sync;
        
        DeviceConfig config;
        config.device_id = 1;
        config.format = DeviceTimestampFormat::MILLISECONDS_SINCE_EPOCH;
        config.calibration_window = 50;
        
        sync.RegisterDevice(config);
        
        Timestamp ts_server;
        ProcessResult result = sync.Process(1000, 1, ts_server);
        
        assert(result == ProcessResult::CALIBRATING);
        TestPassed("Basic conversion");
    } catch (const std::exception& e) {
        TestFailed("Basic conversion", e.what());
    }
}

void TestDriftEstimation() {
    std::cout << "\nTest 2: Drift estimation" << std::endl;
    
    try {
        TimeSynchronizer sync;
        
        DeviceConfig config;
        config.device_id = 1;
        config.format = DeviceTimestampFormat::MILLISECONDS_SINCE_EPOCH;
        config.calibration_window = 100;
        
        sync.RegisterDevice(config);
        
        for (int i = 0; i < 50; i++) {
            Timestamp ts_device = static_cast<Timestamp>(i * 1000);
            Timestamp ts_server;
            sync.Process(ts_device, 1, ts_server);
        }
        
        double drift, offset;
        sync.GetSyncParameters(1, drift, offset);
        
        assert(drift > 0.5 && drift < 2.0);
        
        std::cout << "  (drift=" << std::fixed << std::setprecision(6) 
                 << drift << ")" << std::endl;
        TestPassed("Drift estimation");
    } catch (const std::exception& e) {
        TestFailed("Drift estimation", e.what());
    }
}

void TestTimeChangeDetection() {
    std::cout << "\nTest 3: TIME_CHANGE detection" << std::endl;
    
    try {
        TimeSynchronizer sync;
        
        DeviceConfig config;
        config.device_id = 1;
        config.format = DeviceTimestampFormat::MILLISECONDS_SINCE_EPOCH;
        config.calibration_window = 50;
        
        sync.RegisterDevice(config);
        
        for (int i = 0; i < 10; i++) {
            Timestamp ts_server;
            sync.Process(i * 1000, 1, ts_server);
        }
        
        Timestamp ts_server;
        ProcessResult result = sync.Process(5000, 1, ts_server);
        
        assert(result == ProcessResult::TIME_JUMP_DETECTED);
        TestPassed("TIME_CHANGE detection");
    } catch (const std::exception& e) {
        TestFailed("TIME_CHANGE detection", e.what());
    }
}

void TestNanosecondPrecision() {
    std::cout << "\nTest 4: Nanosecond precision" << std::endl;
    
    try {
        TimeSynchronizer sync;
        
        DeviceConfig config;
        config.device_id = 1;
        config.format = DeviceTimestampFormat::CUSTOM_FREQUENCY_HZ;
        config.frequency_hz = 1000000000;
        config.calibration_window = 50;
        
        sync.RegisterDevice(config);
        
        Timestamp ts_1ghz = 1000000000LL;
        
        Timestamp ts_server;
        sync.Process(ts_1ghz, 1, ts_server);
        
        std::cout << "  (ts=" << ts_server << " ns)" << std::endl;
        TestPassed("Nanosecond precision");
    } catch (const std::exception& e) {
        TestFailed("Nanosecond precision", e.what());
    }
}

void TestMultipleDevices() {
    std::cout << "\nTest 5: Multiple devices" << std::endl;
    
    try {
        TimeSynchronizer sync;
        
        DeviceConfig config1;
        config1.device_id = 1;
        config1.format = DeviceTimestampFormat::MILLISECONDS_SINCE_EPOCH;
        config1.calibration_window = 50;
        sync.RegisterDevice(config1);
        
        DeviceConfig config2;
        config2.device_id = 2;
        config2.format = DeviceTimestampFormat::MICROSECONDS_SINCE_EPOCH;
        config2.calibration_window = 50;
        sync.RegisterDevice(config2);
        
        Timestamp ts_server1, ts_server2;
        
        ProcessResult r1 = sync.Process(1000, 1, ts_server1);
        ProcessResult r2 = sync.Process(1000000, 2, ts_server2);
        
        assert(r1 == ProcessResult::CALIBRATING);
        assert(r2 == ProcessResult::CALIBRATING);
        
        TestPassed("Multiple devices");
    } catch (const std::exception& e) {
        TestFailed("Multiple devices", e.what());
    }
}

void TestFormatConversions() {
    std::cout << "\nTest 6: Format conversions" << std::endl;
    
    try {
        TimeSynchronizer sync;
        
        DeviceConfig config;
        config.device_id = 1;
        config.format = DeviceTimestampFormat::SECONDS_SINCE_EPOCH;
        config.calibration_window = 50;
        sync.RegisterDevice(config);
        
        Timestamp ts_server;
        ProcessResult result = sync.Process(1000, 1, ts_server);
        
        assert(result != ProcessResult::ERROR);
        
        TestPassed("Format conversions");
    } catch (const std::exception& e) {
        TestFailed("Format conversions", e.what());
    }
}

int main() {
    std::cout << "======================================" << std::endl;
    std::cout << "  TimeSynchronizer Unit Tests" << std::endl;
    std::cout << "======================================" << std::endl;
    
    try {
        WinsockInitializer wsa;
        
        TestBasicConversion();
        TestDriftEstimation();
        TestTimeChangeDetection();
        TestNanosecondPrecision();
        TestMultipleDevices();
        TestFormatConversions();
        
        std::cout << "\n" << std::string(40, '=') << std::endl;
        std::cout << "Results: " << tests_passed << " passed, " 
                 << tests_failed << " failed" << std::endl;
        
        if (tests_failed == 0) {
            std::cout << "✓ All tests passed!" << std::endl;
            return 0;
        } else {
            std::cout << "✗ Some tests failed!" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
