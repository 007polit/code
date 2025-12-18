@echo off
setlocal enabledelayedexpansion

echo ========================================
echo     Editer Tests Build Script
echo ========================================
echo.

echo [1/3] Setting up MSVC environment...
call "E:\exe\visual studio\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Cannot find vcvars64.bat
    echo Please check Visual Studio installation path
    pause
    exit /b 1
)
echo MSVC environment ready

echo [2/3] Configuring CMake...
set "BUILD_DIR=build"
set "VCPKG_ROOT=F:\tool\vcpkg"

if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

echo Configuring with:
echo   Source: current directory
echo   Build:  %BUILD_DIR%
echo.

cmake -S . -B "%BUILD_DIR%" -G "Visual Studio 17 2022" "-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
if errorlevel 1 (
    echo [ERROR] CMake configure failed.
    pause
    exit /b 1
)
echo CMake configuration successful

echo [3/3] Building tests...
cmake --build "%BUILD_DIR%" --config Debug
if errorlevel 1 (
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

echo.
echo ========================================
echo     Running Tests...
echo ========================================
echo.

ctest --test-dir "%BUILD_DIR%" -C Debug --output-on-failure --verbose

if errorlevel 1 (
    echo [WARNING] Some tests failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo     ALL TESTS PASSED!
echo ========================================
echo.
pause
exit /b 0