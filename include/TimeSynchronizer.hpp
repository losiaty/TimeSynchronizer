#pragma once

#include <cstdint>
#include <map>
#include <vector>
#include <mutex>
#include <deque>
#include <memory>

using Timestamp = int64_t;
using DeviceID = uint32_t;

enum class DeviceTimestampFormat {
    NANOSECONDS_SINCE_EPOCH,
    MICROSECONDS_SINCE_EPOCH,
    MILLISECONDS_SINCE_EPOCH,
    SECONDS_SINCE_EPOCH,
    CUSTOM_FREQUENCY_HZ
};

struct DeviceConfig {
    DeviceID device_id;
    DeviceTimestampFormat format;
    uint64_t frequency_hz;
    int calibration_window;
};

enum class ProcessResult {
    SUCCESS = 0,
    CALIBRATING = 1,
    TIME_JUMP_DETECTED = 2,
    ERROR = -1
};

struct DeviceSyncState {
    double drift;
    double offset_ns;
    
    struct Sample {
        Timestamp device_ts;
        Timestamp server_ts;
    };
    
    std::deque<Sample> samples;
    int max_samples;
    Timestamp last_device_ts;
    bool initialized;
    double max_acceptable_drift;
    
    DeviceSyncState() 
        : drift(1.0), offset_ns(0.0), max_samples(50), 
          last_device_ts(0), initialized(false), 
          max_acceptable_drift(0.1)
    {}
};

class TimeSynchronizer {
public:
    TimeSynchronizer();
    ~TimeSynchronizer();

    void RegisterDevice(const DeviceConfig& config);
    ProcessResult Process(Timestamp ts_device, DeviceID device_id, Timestamp& ts_server);
    void SignalTimeChange(DeviceID device_id);
    void GetSyncParameters(DeviceID device_id, double& out_drift, double& out_offset) const;
    void ResetDevice(DeviceID device_id);

private:
    Timestamp ConvertToNanoseconds(Timestamp ts, DeviceTimestampFormat format, uint64_t frequency_hz) const;
    void EstimateParameters(DeviceSyncState& state);
    bool DetectAnomalies(const DeviceSyncState& state, Timestamp new_device_ts);

    mutable std::mutex state_mutex_;
    std::map<DeviceID, DeviceSyncState> device_states_;
    std::map<DeviceID, DeviceConfig> device_configs_;
};
