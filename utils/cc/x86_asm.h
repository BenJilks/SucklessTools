#ifndef X86_ASM_H
#define X86_ASM_H

#include <sys/types.h>

#define ENUMERATE_X86_REG \
    __REG(ESP, esp) \
    __REG(EBP, ebp) \
    __REG(EAX, eax) \
    __REG(EBX, ebx)

enum X86Reg
{
#define __REG(x, ...) X86_REG_##x,
    ENUMERATE_X86_REG
#undef __REG
};

#define ENUMERATE_X86_ARGUMENT_TYPE \
    __TYPE(NONE) \
    __TYPE(REG) \
    __TYPE(IMM8)

enum X86ArgumentType
{
#define __TYPE(x) X86_ARGUMENT_TYPE_##x,
    ENUMERATE_X86_ARGUMENT_TYPE
#undef __TYPE
};

typedef struct X86Argument
{
    enum X86ArgumentType type;
    union
    {
        enum X86Reg reg;
        uint8_t imm8;
    };
} X86Argument;

#define ENUMERATE_X86_OP_CODES \
    __OP_CODE(MOV_REG_REG,      REG,    REG,    mov) \
    __OP_CODE(SUB_REG_IMM8,     REG,    IMM8,   sub) \
    __OP_CODE(PUSH_REG,         REG,    NONE,   push) \
    __OP_CODE(POP_REG,          REG,    NONE,   pop) \
    __OP_CODE(RET,              NONE,   NONE,   ret)

enum X86OpCode
{
#define __OP_CODE(x, ...) X86_OP_CODE_##x,
    ENUMERATE_X86_OP_CODES
#undef __OP_CODE
};

typedef struct X86Instruction
{
    enum X86OpCode op_code;
    X86Argument arg1;
    X86Argument arg2;
} X86Instruction;

#define ENUMERATE_X86_LINE_TYPE \
    __TYPE(INSTRUCTION)

enum X86LineType
{
#define __TYPE(x) X86_LINE_TYPE_##x,
    ENUMERATE_X86_LINE_TYPE
#undef __TYPE
};

typedef struct X86Line
{
    enum X86LineType type;
    X86Instruction instruction;
} X86Line;

typedef struct X86Code
{
    X86Line *lines;
    int line_count;
    int line_buffer;
} X86Code;

X86Code x86_code_new();
void x86_code_add_instruction(X86Code*, X86Instruction);
void free_x86_code(X86Code*);

X86Instruction x86(enum X86OpCode op_code, ...);
void x86_dump(X86Code *code);

#endif // X86_ASM_H
