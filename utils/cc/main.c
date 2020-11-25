#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "preproccessor.h"
#include "dumpast.h"
#include "x86.h"
#include <ctype.h>

int main()
{
    Stream input_stream = stream_create_input_file("test.c");
    Stream output_stream = stream_create_output_memory();
    SourceMap source_map = pre_proccess_file(&input_stream, &output_stream);

    stream_close(&input_stream);
    lexer_open_memory(output_stream.memory, output_stream.memory_length, &source_map);

    // Parse
    Unit *unit = parse();
    dump_unit(unit);

    // Compile
    X86Code code = x86_compile_unit(unit);
    x86_dump(&code);

    // Clean up
    free_x86_code(&code);
    free_unit(unit);
    free(unit);

    lexer_close();
    free_source_map(&source_map);
    return 0;
}

