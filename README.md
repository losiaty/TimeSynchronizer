# TimeSynchronizer

Cross-platform C++ library for synchronizing timestamps from distributed devices with clock drift and offset compensation.

## Features

✅ **Nanosecond precision** - All calculations use `long double` for maximum accuracy  
✅ **Automatic calibration** - Linear regression on last 50 samples to estimate drift and offset  
✅ **TIME_CHANGE detection** - Detects backward time jumps and anomalies  
✅ **Thread-safe** - Mutex-protected state management  
✅ **5 timestamp formats** - ns, μs, ms, s, custom frequency  
✅ **Cross-platform** - Windows (MSVC) and Linux (GCC/Clang) support  

## Building

### Windows (Visual Studio 2022)
```bash
mkdir build && cd build
cmake -G "Visual Studio 17 2022" ..
cmake --build . --config Release
```

### Linux (GCC)
```bash
mkdir build && cd build
cmake ..
make
```

## Quick Start

### Terminal 1: Server
```bash
./time_sync_server          # Linux
Release\time_sync_server.exe # Windows
```

### Terminal 2: Client
```bash
./time_sync_client          # Linux
Release\time_sync_client.exe # Windows
```

## Usage

```cpp
#include "TimeSynchronizer.hpp"

TimeSynchronizer sync;

// Register device
DeviceConfig config;
config.device_id = 1;
config.format = DeviceTimestampFormat::MILLISECONDS_SINCE_EPOCH;
config.calibration_window = 50;
sync.RegisterDevice(config);

// Process timestamp
Timestamp ts_device = 1714067234523;
Timestamp ts_server;
ProcessResult result = sync.Process(ts_device, 1, ts_server);

if (result == ProcessResult::SUCCESS) {
    printf("Synchronized: %ld nanoseconds\n", ts_server);
}

// Get calibration parameters
double drift, offset;
sync.GetSyncParameters(1, drift, offset);
// drift = 1.03 (clock +3% faster)
// offset = -500000000 (shifted -500ms)
```

## Algorithm

The synchronizer estimates:
```
ts_server ≈ drift × ts_device + offset
```

Using linear regression on a sliding window of timestamp pairs.

## Supported Timestamp Formats

- `NANOSECONDS_SINCE_EPOCH` - Direct (no conversion)
- `MICROSECONDS_SINCE_EPOCH` - ×1,000
- `MILLISECONDS_SINCE_EPOCH` - ×1,000,000
- `SECONDS_SINCE_EPOCH` - ×1,000,000,000
- `CUSTOM_FREQUENCY_HZ` - Configurable frequency

## Testing

```bash
ctest --verbose
```

Runs 6 unit tests covering:
- Unit conversions
- Drift estimation
- TIME_CHANGE detection
- Nanosecond precision
- Multiple devices
- Format conversions

## Architecture

- **platform.hpp** - Cross-platform socket abstraction (Winsock2 / POSIX)
- **TimeSynchronizer.hpp** - Public API
- **TimeSynchronizer.cpp** - Core algorithm implementation
- **examples/server.cpp** - UDP server demonstrating usage
- **examples/client.cpp** - Device simulator with configurable drift/offset

## Demo Output

```
[    1] Dev  1 | ts_device=    1714067234523 | ts_server=    1714067234023 | 
      drift=1.000000 | offset=     -500000000 ns | status=1

[   50] Dev  1 | ts_device=    1714067235523 | ts_server=    1714067235023 | 
      drift=1.030000 | offset=     -500000000 ns | status=0
```

## License

MIT
