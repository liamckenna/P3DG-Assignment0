cmake_minimum_required(VERSION 3.20)
project(P3DG-Assignment0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32 AND NOT MSVC)
    message(FATAL_ERROR
        "This project requires the MSVC compiler on Windows.\n"
        "Install Visual Studio 2022 (or Build Tools 2022) with the\n"
        "\"Desktop development with C++\" workload, set it as\n"
        "your Launch/Debug target: \n"
        "ctrl+shft+p CMake: Select a Kit\n"
        " --> Visual Studio Professional 2022 Release - amd64\n"
        "then reconfigure.")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR
        "32-bit build detected. The Vulkan SDK only ships 64-bit libraries.\n"
        "Delete the build directory and reconfigure with:\n"
        "  cmake -B build -G \"Visual Studio 17 2022\" -A x64")
endif()

find_package(Vulkan REQUIRED)

include(FetchContent)
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

add_executable(P3DG-Assignment0 src/main.cpp)
target_link_libraries(P3DG-Assignment0 PRIVATE Vulkan::Vulkan glfw)