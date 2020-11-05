project(cc LANGUAGES C)

set(CMAKE_BUILD_TYPE Debug)
set(SOURCES
	main.c
	lexer.c
        parser.c
        expression.c
        datatype.c
        symbol.c
        preproccessor.c
        dumpast.c
        stream.c
        macro_condition.c
        x86.c
        x86_expression.c
        x86_asm.c
)

add_executable(cc ${SOURCES})
