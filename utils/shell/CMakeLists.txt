cmake_minimum_required(VERSION 3.5)

project(shell LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(SOURCES
    shell.cpp
    command.cpp
    lexer.cpp
    modules/autocomplete.cpp
    modules/builtins.cpp
    modules/alias.cpp
    main.cpp)

add_executable(shell ${SOURCES})
