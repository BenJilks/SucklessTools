project(cc LANGUAGES C)

set(CMAKE_BUILD_TYPE Debug)
set(SOURCES
    main.c
    dumpast.c
    lexer/lexer.c
    preprocessor/stream.c
    preprocessor/source_map.c
    preprocessor/preprocessor.c
    preprocessor/macro_condition.c
    parser/parser.c
    parser/expression.c
    parser/datatype.c
    parser/symbol.c
    parser/unit.c
    x86/x86.c
    x86/x86_expression.c
    x86/x86_asm.c
)

add_executable(cc ${SOURCES})

