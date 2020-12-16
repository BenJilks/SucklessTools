#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "unit.h"
#include "preproccessor.h"
#include "dumpast.h"
#include "x86.h"
#include <ctype.h>

int main(int argc, char *argv[])
{
    // TODO: Actuall parse command line arguments
    const char *file_path = "test.c";
    if (argc >= 2)
        file_path = argv[1];

    Stream input_stream = stream_create_input_file(file_path);
    Stream output_stream = stream_create_output_memory();
    SourceMap source_map = pre_proccess_file(&input_stream, &output_stream);

    stream_close(&input_stream);
    lexer_open_memory(output_stream.memory, output_stream.memory_length, &source_map);

    // Parse
    unit_create();
    parse();
    dump_unit();

    // Compile
    X86Code code = x86_compile();
    x86_dump(&code);

    // Clean up
    unit_destroy();
    lexer_destroy();
    free_x86_code(&code);
    free_source_map(&source_map);
    return lexer_has_error();
}

