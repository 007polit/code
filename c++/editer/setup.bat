@echo off
echo ========================================
echo   Editer Setup Script for New Users
echo ========================================
echo.
echo This script will help you set up the development environment.
echo.

REM Check if Visual Studio is installed
echo [1/4] Checking Visual Studio installation...
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    echo Found Visual Studio 2022 Community
    set VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat
    goto :vs_found
)

if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    echo Found Visual Studio 2022 Professional
    set VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat
    goto :vs_found
)

if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    echo Found Visual Studio 2022 Build Tools
    set VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat
    goto :vs_found
)

echo Visual Studio 2022 not found in standard locations.
echo Please install Visual Studio 2022 Community or Build Tools:
echo https://visualstudio.microsoft.com/downloads/
echo.
echo Make sure to install "Desktop development with C++" workload
pause
exit /b 1

:vs_found
echo Visual Studio found!

REM Check vcpkg
echo [2/4] Checking vcpkg installation...
where vcpkg >nul 2>&1
if errorlevel 1 (
    echo vcpkg not found in PATH.
    echo Please install vcpkg:
    echo 1. git clone https://github.com/Microsoft/vcpkg.git
    echo 2. cd vcpkg
    echo 3. .\bootstrap-vcpkg.bat
    echo 4. .\vcpkg integrate install
    echo 5. Add vcpkg directory to PATH
    echo.
    pause
    exit /b 1
)
echo vcpkg found!

REM Install required packages
echo [3/4] Installing required packages...
echo This may take several minutes...
vcpkg install pdcurses:x64-windows
vcpkg install curl:x64-windows
vcpkg install nlohmann-json:x64-windows

if errorlevel 1 (
    echo Package installation failed
    echo Please run the commands manually:
    echo vcpkg install pdcurses:x64-windows curl:x64-windows nlohmann-json:x64-windows
    pause
    exit /b 1
)

REM Create custom build script
echo [4/4] Creating custom build script...
(
echo @echo off
echo call "%VS_PATH%" ^>nul 2^>^&1
echo cl /std:c++20 /EHsc /Iinclude test_buffer.cpp /OUT:test_buffer.exe
echo if %%errorlevel%% equ 0 ^(
echo     echo Buffer test successful!
echo     cl /std:c++20 /EHsc /Iinclude /I"%%VCPKG_ROOT%%\installed\x64-windows\include" src\main.cpp /link /LIBPATH:"%%VCPKG_ROOT%%\installed\x64-windows\lib" pdcurses.lib libcurl.lib /OUT:editer.exe
echo ^)
echo pause
) > build_custom.bat

echo.
echo ========================================
echo   Setup Complete!
echo ========================================
echo.
echo You can now build the project using:
echo   build_custom.bat  (your custom script)
echo   build_msvc.bat    (if paths match the original developer)
echo.
echo To test: run test_buffer.exe first
echo Then run: editer.exe [filename]
echo.
pause
