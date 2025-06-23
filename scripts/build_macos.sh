#!/bin/bash

# macOS build script for ParellelStone Minecraft Server
# Targets arm64 architecture only

set -e  # Exit on any error

# Detect architecture
ARCH=$(uname -m)
echo "Building ParellelStone for macOS ${ARCH}..."

# Validate architecture (macOS arm64 only)
if [[ "${ARCH}" != "arm64" ]]; then
    echo "Warning: Only arm64 is officially supported on macOS"
    echo "Current architecture: ${ARCH}"
    echo "Consider using an arm64 Mac for optimal performance"
fi

VCPKG_TRIPLET="arm64-osx"

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
if [ -f "../vcpkg_installed/vcpkg/scripts/buildsystems/vcpkg.cmake" ]; then
    # Use vcpkg toolchain if available
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=../vcpkg_installed/vcpkg/scripts/buildsystems/vcpkg.cmake \
        -DVCPKG_TARGET_TRIPLET=${VCPKG_TRIPLET}
else
    # Fallback to direct paths
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH=../vcpkg_installed/${VCPKG_TRIPLET} \
        -DBOOST_ROOT=../vcpkg_installed/${VCPKG_TRIPLET}
fi

# Build
echo "Building..."
make -j$(sysctl -n hw.ncpu)

echo "Build completed successfully!"
echo "Executable: $(pwd)/bin/ParellelStone"
echo "Architecture: ${ARCH}"