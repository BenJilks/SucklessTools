cmake_minimum_required(VERSION 3.5)

project(open LANGUAGES CXX)
find_package(Freetype REQUIRED)
find_package(X11 REQUIRED)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(SOURCES
    main.cpp
    item.cpp
    xlib.cpp
)

add_executable(open ${SOURCES})
target_link_libraries(open
    ${FREETYPE_LIBRARIES}
    ${X11_LIBRARIES}
    ${X11_Xft_LIB}
)

target_include_directories(open PRIVATE
    ${FREETYPE_INCLUDE_DIRS}
    ${X11_X11_INCLUDE_PATH}
    ${X11_Xft_INCLUDE_DIRS}
)

install(TARGETS open
    RUNTIME DESTINATION bin)
