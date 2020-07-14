cmake_minimum_required (VERSION 2.8.11)
project(docgen)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(SOURCES
    main.cpp
    lexer.cpp
)

add_executable(docgen ${SOURCES})
