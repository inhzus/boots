cmake_minimum_required(VERSION 3.12)
project(boots LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

add_subdirectory(src)
add_subdirectory(app/server)

