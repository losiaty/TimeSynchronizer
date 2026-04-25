cmake_minimum_required(VERSION 3.10)
project(TimeSynchronizer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Library
add_library(time_synchronizer STATIC
    src/TimeSynchronizer.cpp
)

target_include_directories(time_synchronizer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(WIN32)
    target_link_libraries(time_synchronizer PUBLIC ws2_32 mswsock advapi32)
endif()

# Server
add_executable(time_sync_server examples/server.cpp)
target_link_libraries(time_sync_server time_synchronizer)

# Client
add_executable(time_sync_client examples/client.cpp)
target_link_libraries(time_sync_client)

if(WIN32)
    target_link_libraries(time_sync_client ws2_32 mswsock advapi32)
endif()

# Tests
enable_testing()
add_executable(test_synchronizer tests/test_synchronizer.cpp)
target_link_libraries(test_synchronizer time_synchronizer)
add_test(NAME TimeSynchronizer COMMAND test_synchronizer)
