cmake_minimum_required(VERSION 3.20)

project(immutable_string VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

# force shared CRT
if(MSVC)
    cmake_policy(SET CMP0091 NEW)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

if(NOT MSVC)
    #add_compile_options(-fsanitize=address -g3)
    #add_link_options(-fsanitize=address)
endif()

# Windows-specific settings
if(ER_WINDOWS)
    add_definitions(-D_WIN32_WINNT=0x0601)
endif()

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(submodules/googletest)
    add_subdirectory(tests)
endif()
    