@echo off
REM Windows build script for ParellelStone Minecraft Server
REM Targets x86_64 architecture only

echo Building ParellelStone for Windows x86_64...

REM Validate architecture
if "%PROCESSOR_ARCHITECTURE%"=="x86" (
    echo Error: Only x86_64 architecture is supported on Windows
    echo Current architecture: x86
    exit /b 1
)

set VCPKG_TRIPLET=x64-windows
echo Target: Windows x86_64

REM Check if vcpkg dependencies are installed
if not exist vcpkg_installed\%VCPKG_TRIPLET%\ (
    echo Installing vcpkg dependencies for %VCPKG_TRIPLET%...
    if not exist vcpkg\ (
        echo Please install vcpkg and run: vcpkg install --triplet=%VCPKG_TRIPLET%
        exit /b 1
    )
    vcpkg\vcpkg install --triplet=%VCPKG_TRIPLET%
)

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake
echo Configuring with CMake...
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake ^
    -DVCPKG_TARGET_TRIPLET=%VCPKG_TRIPLET%

REM Build
echo Building...
cmake --build . --config Release --parallel

echo Build completed successfully!
echo Executable: %cd%\bin\Release\ParellelStone.exe
echo Architecture: x86_64

pause