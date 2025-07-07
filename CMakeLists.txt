cmake_minimum_required(VERSION 3.10)
project(chess)

# Always enable debug symbols unless overridden
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 17)

# Find SFML
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# Add executable after build type is set
add_executable(chess main.cpp)

# Link SFML
target_link_libraries(chess sfml-graphics sfml-window sfml-system)
