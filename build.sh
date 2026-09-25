#!/bin/bash
# LinuxGuard Build and Test Script
# Run this on a Linux system with C++17 compiler and CMake

set -e

echo "=== LinuxGuard Build Script ==="
echo ""

# Check dependencies
echo "Checking dependencies..."
command -v g++ >/dev/null 2>&1 || { echo "ERROR: g++ not found"; exit 1; }
command -v cmake >/dev/null 2>&1 || { echo "ERROR: cmake not found"; exit 1; }
command -v make >/dev/null 2>&1 || { echo "ERROR: make not found"; exit 1; }

echo "✓ g++ $(g++ --version | head -1)"
echo "✓ cmake $(cmake --version | head -1)"
echo "✓ make $(make --version | head -1)"
echo ""

# Clean and build
echo "Building LinuxGuard..."
rm -rf build
mkdir build
cd build

cmake ..
make -j$(nproc)

echo ""
echo "✓ Build complete"
echo ""

# Run tests
echo "Running tests..."
ctest --output-on-failure

echo ""
echo "✓ Tests passed"
echo ""

# Show binaries
echo "Built binaries:"
ls -lh linuxguard linuxguard_tests

echo ""
echo "=== Build Successful ==="
echo ""
echo "To run the daemon:"
echo "  ./linuxguard --daemon ../configs/demo.service"
echo ""
echo "To run CLI commands (in another terminal):"
echo "  ./linuxguard list"
echo "  ./linuxguard status <service_name>"
echo "  ./linuxguard start <service_name>"
echo "  ./linuxguard stop <service_name>"
echo "  ./linuxguard shutdown"
