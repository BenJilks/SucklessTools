#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <getopt.h>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/unit.h"
#include "preprocessor/preprocessor.h"
#include "x86/x86.h"
#include "dumpast.h"

static struct option cmd_options[] =
{
	{ "dumpast", no_argument, 0, 'a' },
};

int main(int argc, char *argv[])
{
    int dump_ast = 0;
	for (;;)
	{
		int option_index;
		int c = getopt_long(argc, argv, "a",
			cmd_options, &option_index);

		if (c == -1)
			break;

		switch (c)
		{
			case 'a':
                dump_ast = 1;
                break;
        }
    }

    if (optind >= argc)
    {
        fprintf(stderr, "No input files\n");
        return 1;
    }
    
    // TODO: Allow multiple source files
    const char *file_path = argv[optind];

    Stream input_stream = stream_create_input_file(file_path);
    Stream output_stream = stream_create_output_memory();
    SourceMap source_map = pre_proccess_file(&input_stream, &output_stream);

    stream_close(&input_stream);
    lexer_open_memory(output_stream.memory, output_stream.memory_length, &source_map);

    // Parse
    unit_create();
    parse();
    if (dump_ast)
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

