#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "preproccessor.h"
#include "dumpast.h"
#include "x86.h"

int main()
{
    Stream input_stream = stream_create_input_file("test.c");
    Stream output_stream = stream_create_output_memory();
    pre_proccess_file(&input_stream, &output_stream);
    //fwrite(output_stream.memory, 1, output_stream.memory_length, stdout);

    stream_close(&input_stream);
    stream_close(&output_stream);
    return 0;
    //lexer_open_memory(data, data_len);

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
    return 0;
}
