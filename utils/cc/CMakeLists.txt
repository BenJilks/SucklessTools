project(cc LANGUAGES C)

set(SOURCES
	main.c
	lexer.c
        unit.c
)

add_executable(cc ${SOURCES})

