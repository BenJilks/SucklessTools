#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "preproccessor.h"
#include "dumpast.h"
#include "x86.h"

int main()
{
    int data_len;
    const char *data = pre_proccess_file("test.c", &data_len);
    fwrite(data, 1, data_len, stdout);
    return 0;

    lexer_open_memory(data, data_len);

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
