#include "x86_asm.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

X86Code x86_code_new()
{
    X86Code code;
    code.lines = malloc(80 * sizeof(X86Line));
    code.line_buffer = 80;
    code.line_count = 0;
    return code;
}

void free_x86_code(X86Code *code)
{
    if (code->lines)
        free(code->lines);
}

void x86_code_add_instruction(X86Code *code, X86Instruction instruction)
{
    // Check we have enough space
    if (code->line_count >= code->line_buffer)
    {
        code->line_buffer += 80;
        code->lines = realloc(code->lines, code->line_buffer * sizeof(X86Line));
    }

    // Create line
    X86Line line;
    line.type = X86_LINE_TYPE_INSTRUCTION;
    line.instruction = instruction;

    // Add it to the code
    code->lines[code->line_count] = line;
    code->line_count += 1;
}

static X86Argument reg(enum X86Reg reg)
{
    X86Argument arg;
    arg.type = X86_ARGUMENT_TYPE_REG;
    arg.reg = reg;
    return arg;
}
static X86Argument imm8(uint8_t imm8)
{
    X86Argument arg;
    arg.type = X86_ARGUMENT_TYPE_IMM8;
    arg.imm8 = imm8;
    return arg;
}

X86Instruction x86(enum X86OpCode op_code, ...)
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
#define ARG_FUNC_NONE(...) none
#define __OP_CODE(x, a1, a2, ...) \
        case X86_OP_CODE_##x: \
            instruction.arg1 = ARG_FUNC_##a1(va_arg(argp, ARG_TYPE_##a1)); \
            instruction.arg2 = ARG_FUNC_##a2(va_arg(argp, ARG_TYPE_##a2)); \
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
#define __REG(x, name) case X86_REG_##x: printf(#name " "); break;
        ENUMERATE_X86_REG
#undef __REG
    }
}

static void dump_argument(X86Argument *argument)
{
    switch (argument->type)
    {
        case X86_ARGUMENT_TYPE_NONE:
            break;
        case X86_ARGUMENT_TYPE_REG:
            dump_reg(argument->reg);
            break;
        case X86_ARGUMENT_TYPE_IMM8:
            printf("#%i ", argument->imm8);
            break;
    }
}

static void dump_instruction(X86Instruction *instruction)
{
    switch (instruction->op_code)
    {
#define __OP_CODE(x, a1, a2, name) \
        case X86_OP_CODE_##x: printf(#name " "); break;
        ENUMERATE_X86_OP_CODES
#undef __OP_CODE
    }

    dump_argument(&instruction->arg1);
    dump_argument(&instruction->arg2);
    printf("\n");
}

void x86_dump(X86Code *code)
{
    for (int i = 0; i < code->line_count; i++)
    {
        X86Line *line = &code->lines[i];
        switch (line->type)
        {
            case X86_LINE_TYPE_INSTRUCTION:
                dump_instruction(&line->instruction);
                break;
        }
    }
}
