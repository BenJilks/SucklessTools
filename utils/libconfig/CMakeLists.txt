cmake_minimum_required(VERSION 3.5)

project(libconfig LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(libconfig INTERFACE)
set_target_properties(libconfig PROPERTIES PUBLIC_HEADER "libconfig.hpp")
install(TARGETS libconfig
    PUBLIC_HEADER DESTINATION include)
