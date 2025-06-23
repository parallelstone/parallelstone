#!/bin/bash

# Linux build script for ParellelStone Minecraft Server
# Supports both x86_64 and arm64 architectures

set -e  # Exit on any error

# Detect architecture
ARCH=$(uname -m)
echo "Building ParellelStone for Linux ${ARCH}..."

# Validate supported architecture
case "${ARCH}" in
    x86_64|amd64)
        VCPKG_TRIPLET="x64-linux"
        echo "Target: Linux x86_64"
        ;;
    aarch64|arm64)
        VCPKG_TRIPLET="arm64-linux"
        echo "Target: Linux arm64"
        ;;
    *)
        echo "Error: Unsupported architecture ${ARCH}"
        echo "Supported: x86_64, arm64"
        exit 1
        ;;
esac

# Install dependencies
echo "Installing dependencies..."
sudo apt-get update
sudo apt-get install -y cmake build-essential pkg-config

# Check if vcpkg dependencies are installed
if [ ! -d "vcpkg_installed/${VCPKG_TRIPLET}" ]; then
    echo "Installing vcpkg dependencies for ${VCPKG_TRIPLET}..."
    if [ ! -d "vcpkg" ]; then
        echo "Please install vcpkg and run: vcpkg install --triplet=${VCPKG_TRIPLET}"
        exit 1
    fi
    vcpkg install --triplet=${VCPKG_TRIPLET}
fi

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=${VCPKG_TRIPLET}

# Build
echo "Building..."
make -j$(nproc)

echo "Build completed successfully!"
echo "Executable: $(pwd)/bin/ParellelStone"
echo "Architecture: ${ARCH}"