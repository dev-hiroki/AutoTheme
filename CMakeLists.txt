cmake_minimum_required(VERSION 4.0)
project(AutoTheme)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_WIN32_EXECUTABLE TRUE)

include("CPM.cmake")

CPMAddPackage("gh:gabime/spdlog@1.16.0")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    remove_definitions(-D_DEBUG)
    add_definitions(-DDEBUG)
endif()

add_executable(${PROJECT_NAME} PCH.cpp Main.cpp Resources/Resource.rc)
target_link_libraries(${PROJECT_NAME} PRIVATE WindowsApp.lib spdlog)
target_precompile_headers(${PROJECT_NAME} PRIVATE "PCH.h")