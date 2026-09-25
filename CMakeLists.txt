cmake_minimum_required(VERSION 3.14)
project(LinuxGuard VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
add_compile_options(-Wall -Wextra -Wpedantic)

# Main executable
add_executable(linuxguard
    src/main.cpp
    src/daemon/daemon.cpp
    src/process/process_manager.cpp
    src/config/config_parser.cpp
    src/monitor/resource_monitor.cpp
    src/dependency/dependency_graph.cpp
    src/ipc/unix_socket.cpp
    src/logging/logger.cpp
    src/cli/cli.cpp
)

target_include_directories(linuxguard PRIVATE src)
target_link_libraries(linuxguard PRIVATE pthread)

# Tests
enable_testing()

add_executable(linuxguard_tests
    tests/process_tests.cpp
    tests/config_tests.cpp
    tests/dependency_tests.cpp
    src/process/process_manager.cpp
    src/config/config_parser.cpp
    src/monitor/resource_monitor.cpp
    src/dependency/dependency_graph.cpp
    src/ipc/unix_socket.cpp
    src/logging/logger.cpp
)

target_include_directories(linuxguard_tests PRIVATE src)
target_link_libraries(linuxguard_tests PRIVATE pthread)

add_test(NAME unit_tests COMMAND linuxguard_tests)

# Install
install(TARGETS linuxguard RUNTIME DESTINATION bin)
install(FILES configs/demo.service DESTINATION etc/linuxguard)
