# Build Instructions

## Windows (Visual Studio 2022)

```bash
# Clone repository
git clone https://github.com/losiaty/TimeSynchronizer.git
cd TimeSynchronizer

# Create build directory
mkdir build
cd build

# Generate Visual Studio project
cmake -G "Visual Studio 17 2022" ..

# Build
cmake --build . --config Release

# Run tests
ctest --config Release --verbose
```

## Windows (MSVC command line)

```bash
mkdir build
cd build
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ..
nmake
```

## Linux (GCC)

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Run tests
ctest --verbose
```

## Linux (Clang)

```bash
mkdir build
cd build
CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

## macOS

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)
```

## Run Demo

### Terminal 1: Start server
```bash
cd build
./time_sync_server        # Linux/macOS
.\Release\time_sync_server.exe  # Windows
```

### Terminal 2: Run client simulator
```bash
cd build
./time_sync_client        # Linux/macOS
.\Release\time_sync_client.exe  # Windows
```

## Output Files

After build, the following are generated:

```
build/
├── time_sync_server[.exe]
├── time_sync_client[.exe]
├── test_synchronizer[.exe]
├── libtime_synchronizer.a    (static library)
└── CMakeFiles/
```

## Troubleshooting

### "Cannot open include file"
- Ensure CMake is in PATH: `cmake --version`
- On Windows, use full path: `C:\Program Files\CMake\bin\cmake.exe`

### "Socket permission denied" on Linux
- Default port 12345 requires no special permissions
- If port in use, modify port in server.cpp/client.cpp

### "git not found" warning during build
- This is harmless, git is optional for this project

## IDE Integration

### Visual Studio
```bash
cmake -G "Visual Studio 17 2022" ..
# Open generated .sln file in Visual Studio
```

### VS Code with CMake Tools
1. Install CMake extension
2. Open folder in VS Code
3. CMake should auto-detect configuration

### CLion
1. Open project folder
2. CLion auto-detects CMakeLists.txt
3. Use built-in CMake integration

## Compiler Requirements

- **Windows**: MSVC 2019+ (or compatible)
- **Linux**: GCC 7+ (or Clang 5+)
- **macOS**: Apple Clang
- **CMake**: 3.10 or later

## Dependencies

- None! Only standard C++17 library and platform sockets (built-in)

## Building as Shared Library

### Linux
```bash
cmake -DBUILD_SHARED=ON ..
make
# Produces: libtime_synchronizer.so
```

### Windows (DLL)
```bash
cmake -G "Visual Studio 17 2022" -DBUILD_SHARED=ON ..
cmake --build . --config Release
# Produces: time_synchronizer.dll
```
