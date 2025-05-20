cmake_minimum_required(VERSION 3.10)
project(chess)

set(CMAKE_CXX_STANDARD 17)

# Find SFML
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

add_executable(sfml-chess main.cpp)

target_link_libraries(sfml-chess sfml-graphics sfml-window sfml-system)
