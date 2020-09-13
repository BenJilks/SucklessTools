cmake_minimum_required(VERSION 3.5)

project(terminal LANGUAGES CXX)
find_package(Freetype REQUIRED)
find_package(X11 REQUIRED)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(SOURCES
    main.cpp
    terminal.cpp
    color.cpp
    decoder.cpp
    buffer.cpp
    attribute.cpp
    output/output.cpp
    output/xlib.cpp
    output/xclipboard.cpp
)

add_executable(terminal ${SOURCES})
target_link_libraries(terminal
    ${FREETYPE_LIBRARIES}
    ${X11_LIBRARIES}
    ${X11_Xft_LIB}
    profile
    util # This is for ARM, where 'openpty' is in libutil
)

target_include_directories(terminal PRIVATE
    ${FREETYPE_INCLUDE_DIRS}
    ${X11_X11_INCLUDE_PATH}
    ${X11_Xft_INCLUDE_DIRS}
)

install(TARGETS terminal
    RUNTIME DESTINATION bin)
