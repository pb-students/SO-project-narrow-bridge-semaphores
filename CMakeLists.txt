cmake_minimum_required(VERSION 3.17)
project(narrow_bridge C)

set(CMAKE_C_STANDARD 11)

add_executable(narrow_bridge main.c queue.c urandom.c car.c car.h)
target_link_libraries(narrow_bridge PUBLIC pthread)
