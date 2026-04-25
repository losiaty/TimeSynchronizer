#include "TimeSynchronizer.hpp"
#include <cmath>
#include <algorithm>
#include <chrono>
#include <stdexcept>

TimeSynchronizer::TimeSynchronizer() = default;
TimeSynchronizer::~TimeSynchronizer() = default;

void TimeSynchronizer::RegisterDevice(const DeviceConfig& config) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (device_states_.find(config.device_id) != device_states_.end()) {
        return;
    }
    
    DeviceSyncState state;
    state.max_samples = config.calibration_window;
    
    device_states_[config.device_id] = state;
    device_configs_[config.device_id] = config;
}

Timestamp TimeSynchronizer::ConvertToNanoseconds(
    Timestamp ts, 
    DeviceTimestampFormat format, 
    uint64_t frequency_hz) const 
{
    switch (format) {
        case DeviceTimestampFormat::NANOSECONDS_SINCE_EPOCH:
            return ts;
            
        case DeviceTimestampFormat::MICROSECONDS_SINCE_EPOCH:
            return ts * 1000LL;
            
        case DeviceTimestampFormat::MILLISECONDS_SINCE_EPOCH:
            return ts * 1000000LL;
            
        case DeviceTimestampFormat::SECONDS_SINCE_EPOCH:
            return ts * 1000000000LL;
            
        case DeviceTimestampFormat::CUSTOM_FREQUENCY_HZ: {
            if (frequency_hz == 0) {
                throw std::runtime_error("Invalid frequency: 0 Hz");
            }
            
            long double ns_per_tick = 1000000000.0L / static_cast<long double>(frequency_hz);
            long double result_ld = static_cast<long double>(ts) * ns_per_tick;
            
            return static_cast<Timestamp>(std::llround(result_ld));
        }
        
        default:
            throw std::runtime_error("Unknown timestamp format");
    }
}

bool TimeSynchronizer::DetectAnomalies(
    const DeviceSyncState& state,
    Timestamp new_device_ts) 
{
    if (!state.initialized) {
        return false;
    }
    
    if (new_device_ts < state.last_device_ts) {
        return true;
    }
    
    if (state.samples.size() >= 2) {
        if (std::abs(state.drift - 1.0) > state.max_acceptable_drift) {
            return true;
        }
    }
    
    return false;
}

void TimeSynchronizer::EstimateParameters(DeviceSyncState& state) {
    if (state.samples.size() < 2) {
        return;
    }
    
    size_t n = state.samples.size();
    long double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    
    for (const auto& sample : state.samples) {
        long double x = static_cast<long double>(sample.device_ts);
        long double y = static_cast<long double>(sample.server_ts);
        
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_x2 += x * x;
    }
    
    long double n_ld = static_cast<long double>(n);
    long double denominator = n_ld * sum_x2 - sum_x * sum_x;
    
    if (std::abs(denominator) < 1e-9) {
        return;
    }
    
    state.drift = static_cast<double>(
        (n_ld * sum_xy - sum_x * sum_y) / denominator
    );
    
    state.offset_ns = static_cast<double>(
        (sum_y - state.drift * sum_x) / n_ld
    );
}

ProcessResult TimeSynchronizer::Process(
    Timestamp ts_device,
    DeviceID device_id,
    Timestamp& ts_server) 
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    auto it = device_states_.find(device_id);
    if (it == device_states_.end()) {
        return ProcessResult::ERROR;
    }
    
    auto config_it = device_configs_.find(device_id);
    if (config_it == device_configs_.end()) {
        return ProcessResult::ERROR;
    }
    
    DeviceSyncState& state = it->second;
    const DeviceConfig& config = config_it->second;
    
    Timestamp ts_device_ns = ConvertToNanoseconds(
        ts_device,
        config.format,
        config.frequency_hz
    );
    
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    Timestamp server_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    
    if (DetectAnomalies(state, ts_device_ns)) {
        state.samples.clear();
        state.initialized = false;
        state.drift = 1.0;
        state.offset_ns = 0.0;
        
        state.samples.push_back({ts_device_ns, server_time_ns});
        state.last_device_ts = ts_device_ns;
        
        return ProcessResult::TIME_JUMP_DETECTED;
    }
    
    state.samples.push_back({ts_device_ns, server_time_ns});
    if (state.samples.size() > static_cast<size_t>(state.max_samples)) {
        state.samples.pop_front();
    }
    
    state.last_device_ts = ts_device_ns;
    
    if (state.samples.size() >= 3) {
        EstimateParameters(state);
        state.initialized = true;
    }
    
    if (state.initialized && state.samples.size() >= 3) {
        long double ts_device_ld = static_cast<long double>(ts_device_ns);
        long double result_ld = state.drift * ts_device_ld + state.offset_ns;
        ts_server = static_cast<Timestamp>(std::llround(result_ld));
        
        return ProcessResult::SUCCESS;
    } else {
        ts_server = server_time_ns;
        return ProcessResult::CALIBRATING;
    }
}

void TimeSynchronizer::SignalTimeChange(DeviceID device_id) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    auto it = device_states_.find(device_id);
    if (it != device_states_.end()) {
        it->second.samples.clear();
        it->second.initialized = false;
        it->second.drift = 1.0;
        it->second.offset_ns = 0.0;
    }
}

void TimeSynchronizer::GetSyncParameters(
    DeviceID device_id,
    double& out_drift,
    double& out_offset) const 
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    auto it = device_states_.find(device_id);
    if (it != device_states_.end()) {
        out_drift = it->second.drift;
        out_offset = it->second.offset_ns;
    }
}

void TimeSynchronizer::ResetDevice(DeviceID device_id) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    auto it = device_states_.find(device_id);
    if (it != device_states_.end()) {
        it->second = DeviceSyncState();
        it->second.max_samples = device_configs_[device_id].calibration_window;
    }
}
