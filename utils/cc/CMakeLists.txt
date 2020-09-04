project(cc LANGUAGES C)

set(CMAKE_BUILD_TYPE Debug)
set(SOURCES
	main.c
	lexer.c
        parser.c
)

add_executable(cc ${SOURCES})
