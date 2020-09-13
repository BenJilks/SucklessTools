cmake_minimum_required(VERSION 3.5)

project(profile LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(SOURCES
	profile.cpp
)

add_library(profile ${SOURCES})

install(TARGETS profile
    LIBRARY DESTINATION lib)
install(DIRECTORY ${CMAKE_SOURCE_DIR}
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp")
