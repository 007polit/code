@echo off
setlocal EnableDelayedExpansion

echo ========================================
echo     Editer MSVC Build Script
echo ========================================
echo.

echo [1/4] Setting up MSVC environment...
call "E:\exe\visual studio\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Cannot find vcvars64.bat
    echo Please check Visual Studio installation path:
    echo E:\exe\visual studio\VC\Auxiliary\Build\vcvars64.bat
    pause
    exit /b 1
)
echo MSVC environment ready

echo [2/4] Checking compiler...
cl >nul 2>&1
if errorlevel 1 (
    echo [ERROR] cl.exe not found in PATH
    pause
    exit /b 1
)
echo Compiler found

echo [3/4] Checking vcpkg...
set "VCPKG_ROOT=F:\tool\vcpkg"
if not exist "%VCPKG_ROOT%\installed\x64-windows\include" (
    echo [ERROR] vcpkg libraries not found!
    echo Expected path: %VCPKG_ROOT%\installed\x64-windows\include
    echo Please run:
    echo   vcpkg install pdcurses:x64-windows curl:x64-windows nlohmann-json:x64-windows
    pause
    exit /b 1
)
echo vcpkg libraries ready

echo [4/4] Compiling editer.exe ...
echo.

rem Check if icon resource exists
if exist "assets\icon.ico" (
    echo Found icon resource: assets\icon.ico
) else (
    echo [WARNING] Icon not found. Run assets\generate_icon.bat to create it.
)

rem 
pushd "%~dp0"

rem Auto-fix wrong include if still present
if exist "src\main.cpp" (
    findstr /c:"#include \"core/buffer_fixed.hpp\"" "src\main.cpp" >nul 2>&1
    if !errorlevel! equ 0 (
        echo Fixing incorrect include: buffer_fixed.hpp -^> buffer.hpp
        powershell -Command "(Get-Content 'src\main.cpp') -replace '#include\s+\"core/buffer_fixed\.hpp\"', '#include \"core/buffer.hpp\"' | Set-Content 'src\main.cpp' -Encoding ASCII" >nul 2>&1
    )
)

rem Compile resource file if icon exists
set "RC_FILE="
if exist "assets\icon.ico" (
    if exist "assets\editer.rc" (
        echo Compiling icon resource...
        rc /nologo /fo assets\editer.res assets\editer.rc >nul 2>&1
        if !errorlevel! equ 0 (
            set "RC_FILE=assets\editer.res"
            echo Icon resource compiled successfully
        )
    )
)

rem 
cl /std:c++20 /EHsc /MD /DNDEBUG ^
   /Iinclude ^
   /I"%VCPKG_ROOT%\installed\x64-windows\include" ^
   src\main.cpp src\file_tree.cpp ^
   src\ui\terminal.cpp ^
   src\ai\ai_chat.cpp ^
   src\ai\api_client.cpp ^
   src\config\config_interpreter.cpp ^
   src\syntax\cpp_highlighter.cpp ^
   src\syntax\python_highlighter.cpp ^
   src\syntax\markdown_highlighter.cpp ^
   src\syntax\json_highlighter.cpp ^
   src\syntax\cfg_highlighter.cpp ^
   src\syntax\language_detector.cpp ^
   src\syntax_renderer.cpp ^
   %RC_FILE% ^
   /link ^
   /LIBPATH:"%VCPKG_ROOT%\installed\x64-windows\lib" ^
   pdcurses.lib libcurl.lib libcrypto.lib libssl.lib ^
   ws2_32.lib crypt32.lib wldap32.lib ^
   /OUT:editer.exe

set "compile_result=%errorlevel%"
popd

if %compile_result% neq 0 (
    echo.
    echo [ERROR] Compilation failed! See error messages above.
    pause
    exit /b 1
)

echo.
echo ========================================
echo         BUILD SUCCESSFUL!
echo ========================================
echo.
echo editer.exe has been created in the project root directory.
echo.
echo You can now run:
echo   editer.exe
echo   or
echo   editer.exe examples\test.cpp
echo.
echo Enjoy your AI-powered editor!
echo.
pause