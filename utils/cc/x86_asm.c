#include "x86_asm.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#define EXTERNAL_MAX_LENGTH 80

X86Code x86_code_new()
{
    X86Code code;
    code.label_mem = NULL;
    code.label_count = 0;

    code.string_data = NULL;
    code.string_data_count = 0;

    code.externals = malloc(80 * EXTERNAL_MAX_LENGTH);
    code.external_buffer = 80;
    code.external_count = 0;

    code.lines = malloc(80 * sizeof(X86Line));
    code.line_buffer = 80;
    code.line_count = 0;
    return code;
}

void free_x86_code(X86Code *code)
{
    if (code->label_mem)
    {
        for (int i = 0; i < code->label_count; i++)
            free(code->label_mem[i]);
        free(code->label_mem);
    }

    if (code->externals)
        free(code->externals);

    if (code->lines)
        free(code->lines);
}

static void add_line(X86Code *code, X86Line line)
{
    // Check we have enough space
    if (code->line_count >= code->line_buffer)
    {
        code->line_buffer += 80;
        code->lines = realloc(code->lines, code->line_buffer * sizeof(X86Line));
    }

    // Add it to the code
    code->lines[code->line_count] = line;
    code->line_count += 1;
}

void x86_code_add_instruction(X86Code *code, X86Instruction instruction)
{
    X86Line line;
    line.type = X86_LINE_TYPE_INSTRUCTION;
    line.instruction = instruction;
    add_line(code, line);
}

void x86_code_add_label(X86Code *code, const char *label)
{
    X86Line line;
    line.type = X86_LINE_TYPE_LABEL;
    strcpy(line.label, label);
    add_line(code, line);
}
void x86_code_add_external(X86Code *code, const char *external)
{
    if (code->external_count >= code->external_buffer)
    {
        code->external_buffer += 80;
        code->externals = realloc(code->externals,
            code->external_buffer * EXTERNAL_MAX_LENGTH);
    }

    strcpy(code->externals + code->external_count * EXTERNAL_MAX_LENGTH, external);
    code->external_count += 1;
}

void x86_code_add_blank(X86Code *code)
{
    X86Line line;
    line.type = X86_LINE_TYPE_BLANK;
    add_line(code, line);
}

int x86_code_add_string_data(X86Code *code, const char *str)
{
    if (!code->string_data)
        code->string_data = malloc(sizeof(char*));
    else
        code->string_data = realloc(code->string_data, (code->string_data_count + 1) * sizeof(char*));

    char *mem = malloc(strlen(str) + 1);
    strcpy(mem, str);
    code->string_data[code->string_data_count++] = mem;
    return code->string_data_count - 1;
}

static X86Argument reg(enum X86Reg reg, X86Code *code)
{
    (void)code;
    X86Argument arg;
    arg.type = X86_ARGUMENT_TYPE_REG;
    arg.reg = reg;
    return arg;
}
static X86Argument imm8(uint8_t imm8, X86Code *code)
{
    (void)code;
    X86Argument arg;
    arg.type = X86_ARGUMENT_TYPE_IMM8;
    arg.imm8 = imm8;
    return arg;
}
static X86Argument imm32(uint32_t imm32, X86Code *code)
{
    (void)code;
    X86Argument arg;
    arg.type = X86_ARGUMENT_TYPE_IMM32;
    arg.imm32 = imm32;
    return arg;
}
static X86Argument off(uint8_t off, X86Code *code)
{
    (void)code;
    X86Argument arg;
    arg.type = X86_ARGUMENT_TYPE_OFF;
    arg.off = off;
    return arg;
}
static X86Argument label(const char *label, X86Code *code)
{
    if (!code->label_mem)
        code->label_mem = malloc(sizeof(char*));
    else
        code->label_mem = realloc(code->label_mem, (code->line_count + 1) * sizeof(char*));

    char *mem = malloc(strlen(label) + 1);
    strcpy(mem, label);
    code->label_mem[code->label_count++] = mem;

    X86Argument arg;
    arg.type = X86_ARGUMENT_TYPE_LABEL;
    arg.label = mem;
    return arg;
}

X86Instruction x86(X86Code *code, enum X86OpCode op_code, ...)
{
    X86Instruction instruction;
    instruction.op_code = op_code;

    X86Argument none;
    none.type = X86_ARGUMENT_TYPE_NONE;

    va_list argp;
    va_start(argp, op_code);
    switch (op_code)
    {
#define ARG_FUNC_REG reg
#define ARG_TYPE_REG enum X86Reg
#define ARG_FUNC_IMM8 imm8
#define ARG_TYPE_IMM8 int
#define ARG_FUNC_IMM32 imm32
#define ARG_TYPE_IMM32 int
#define ARG_FUNC_OFF off
#define ARG_TYPE_OFF int
#define ARG_FUNC_LABEL label
#define ARG_TYPE_LABEL const char*
#define ARG_FUNC_NONE(...) none

#define __OP_CODE(x, a1, a2, a3, ...) \
        case X86_OP_CODE_##x: \
            instruction.arg1 = ARG_FUNC_##a1(va_arg(argp, ARG_TYPE_##a1), code); \
            instruction.arg2 = ARG_FUNC_##a2(va_arg(argp, ARG_TYPE_##a2), code); \
            instruction.arg3 = ARG_FUNC_##a3(va_arg(argp, ARG_TYPE_##a3), code); \
            break;

        ENUMERATE_X86_OP_CODES
#undef __OP_CODE

        default:
            assert (0);
    }

    va_end(argp);
    return instruction;
}

static void dump_reg(enum X86Reg reg)
{
    switch (reg)
    {
#define __REG(x, name) case X86_REG_##x: printf(#name); break;
        ENUMERATE_X86_REG
#undef __REG
    }
}

static void dump_argument(X86Argument *argument, int is_first)
{
    switch (argument->type)
    {
        case X86_ARGUMENT_TYPE_NONE:
            break;
        case X86_ARGUMENT_TYPE_REG:
            printf("%s", is_first ? "" : ", ");
            dump_reg(argument->reg);
            break;
        case X86_ARGUMENT_TYPE_IMM8:
            printf("%sbyte %i", is_first ? "" : ", ", argument->imm8);
            break;
        case X86_ARGUMENT_TYPE_IMM32:
            printf("%sdword %i", is_first ? "" : ", ", argument->imm32);
            break;
        case X86_ARGUMENT_TYPE_LABEL:
            printf("%s%s", is_first ? "" : ", ", argument->label);
            break;
        default:
            assert (0);
    }
}

static void dump_instruction(X86Instruction *instruction)
{
    switch (instruction->op_code)
    {
#define __OP_CODE(x, a1, a2, a3, name) \
        case X86_OP_CODE_##x: printf("  " #name " "); break;
        ENUMERATE_X86_OP_CODES
#undef __OP_CODE
    }

    if (instruction->arg2.type == X86_ARGUMENT_TYPE_OFF)
    {
        printf("dword [");
        dump_argument(&instruction->arg1, 1);
        if (instruction->arg2.off >= 0)
            printf("+%i]", instruction->arg2.off);
        else
            printf("-%i]", -instruction->arg2.off);
        dump_argument(&instruction->arg3, 0);
    }
    else if (instruction->arg3.type == X86_ARGUMENT_TYPE_OFF)
    {
        dump_argument(&instruction->arg1, 1);
        printf("dword [");
        dump_argument(&instruction->arg2, 1);
        if (instruction->arg3.off >= 0)
            printf("+%i]", instruction->arg3.off);
        else
            printf("-%i]", -instruction->arg3.off);
    }
    else
    {
        dump_argument(&instruction->arg1, 1);
        dump_argument(&instruction->arg2, 0);
        dump_argument(&instruction->arg3, 0);
    }
    printf("\n");
}

static void dump_externals(X86Code *code)
{
    for (int i = 0; i < code->external_count; i++)
        printf("  extern %s\n", code->externals + i * EXTERNAL_MAX_LENGTH);
}

static void dump_string_data(X86Code *code)
{
    for (int i = 0; i < code->string_data_count; i++)
        printf("  str%i: db \"%s\"\n", i, code->string_data[i]);
}

void x86_dump(X86Code *code)
{
    printf("  global main\n");
    dump_externals(code);

    printf("segment .data\n");
    dump_string_data(code);

    printf("\nsegment .text\n");
    for (int i = 0; i < code->line_count; i++)
    {
        X86Line *line = &code->lines[i];
        switch (line->type)
        {
            case X86_LINE_TYPE_INSTRUCTION:
                dump_instruction(&line->instruction);
                break;
            case X86_LINE_TYPE_LABEL:
                printf("%s:\n", line->label);
                break;
            case X86_LINE_TYPE_BLANK:
                printf("\n");
                break;
        }
    }
}
