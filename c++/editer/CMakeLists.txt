cmake_minimum_required(VERSION 3.16)
project(editer VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含目录
include_directories(include)

# 源文件
set(SOURCES 
    src/main.cpp
    src/file_tree.cpp
    src/ui/terminal.cpp
    src/syntax/cpp_highlighter.cpp
    src/syntax/python_highlighter.cpp
    src/syntax/markdown_highlighter.cpp
    src/syntax/json_highlighter.cpp
    src/syntax/language_detector.cpp
    src/syntax_renderer.cpp
    src/ai/ai_chat.cpp
    src/ai/api_client.cpp
    src/config/config_interpreter.cpp
)

# 创建可执行文件
add_executable(editer ${SOURCES})

# 查找依赖库
if(WIN32)
    # Windows: 优先使用vcpkg
    find_package(PkgConfig QUIET)
    
    # 查找PDCurses
    find_library(PDCURSES_LIBRARY 
        NAMES pdcurses
        PATHS ${CMAKE_PREFIX_PATH}/lib
        NO_DEFAULT_PATH
    )
    
    if(PDCURSES_LIBRARY)
        target_link_libraries(editer ${PDCURSES_LIBRARY})
        message(STATUS "Found PDCurses: ${PDCURSES_LIBRARY}")
    else()
        message(FATAL_ERROR "PDCurses not found. Please install with: vcpkg install pdcurses:x64-windows")
    endif()
    
    # 查找libcurl
    find_package(CURL REQUIRED)
    target_link_libraries(editer CURL::libcurl)
    
else()
    # Linux: 使用pkg-config
    find_package(PkgConfig REQUIRED)
    
    # 查找ncurses
    pkg_check_modules(NCURSES REQUIRED ncurses)
    target_link_libraries(editer ${NCURSES_LIBRARIES})
    target_include_directories(editer PRIVATE ${NCURSES_INCLUDE_DIRS})
    
    # 查找libcurl
    pkg_check_modules(CURL REQUIRED libcurl)
    target_link_libraries(editer ${CURL_LIBRARIES})
    target_include_directories(editer PRIVATE ${CURL_INCLUDE_DIRS})
endif()

# 查找nlohmann/json (header-only)
find_package(nlohmann_json QUIET)
if(nlohmann_json_FOUND)
    target_link_libraries(editer nlohmann_json::nlohmann_json)
    message(STATUS "Found nlohmann/json")
else()
    message(WARNING "nlohmann/json not found via CMake, assuming header is in system path")
endif()

# 编译选项
target_compile_options(editer PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# 输出信息
message(STATUS "Editer build configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")

# 安装规则 (可选)
install(TARGETS editer DESTINATION bin)
install(FILES README.md INSTALL.md DESTINATION share/doc/editer)
install(FILES editer.conf DESTINATION share/editer)
