cmake_minimum_required(VERSION 3.5)

project(debtorsbook LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(/usr/local/include)
link_directories(/usr/local/lib)
link_libraries(database)
add_executable(debtorsbook main.cpp)
install(TARGETS debtorsbook
    RUNTIME DESTINATION bin)
