cmake_minimum_required(VERSION 3.5)

project(database LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(SOURCES
    cleaner.cpp
    database.cpp
    chunk.cpp
    dynamicdata.cpp
    table.cpp
    column.cpp
    row.cpp
    entry.cpp
    prompt.cpp
    sql/lexer.cpp
    sql/parser.cpp
    sql/select.cpp
    sql/insert.cpp
    sql/createtable.cpp
    sql/createtableifnotexists.cpp
    sql/update.cpp
    sql/delete.cpp
    sql/value.cpp
)

add_library(database ${SOURCES})
add_executable(databaseclt main.cpp ${SOURCES})

install(TARGETS database
    LIBRARY DESTINATION lib)
install(DIRECTORY ${CMAKE_SOURCE_DIR}
    DESTINATION include/database
    FILES_MATCHING PATTERN "*.hpp")
