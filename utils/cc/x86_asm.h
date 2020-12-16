#ifndef X86_ASM_H
#define X86_ASM_H

#include <sys/types.h>
#include <inttypes.h>

#define ENUMERATE_X86_REG \
    __REG(ESP, esp) \
    __REG(EBP, ebp) \
    __REG(EAX, eax) \
    __REG(EBX, ebx) \
    __REG(ECX, ecx) \
    __REG(AL, al) \
    __REG(BL, bl) \
    __REG(ST0, st0) \
    __REG(ST1, st1)

enum X86Reg
{
#define __REG(x, ...) X86_REG_##x,
    ENUMERATE_X86_REG
#undef __REG
};

#define ENUMERATE_X86_ARGUMENT_TYPE \
    __TYPE(NONE) \
    __TYPE(REG) \
    __TYPE(IMM8) \
    __TYPE(IMM32) \
    __TYPE(OFF64) \
    __TYPE(OFF32) \
    __TYPE(OFF8) \
    __TYPE(LABEL)

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
        int8_t off;
        uint8_t imm8;
        uint32_t imm32;
        const char *label;
    };
} X86Argument;

#define ENUMERATE_X86_OP_CODES                                               \
    __OP_CODE(MOV_REG_REG,                    REG,    REG,    NONE,   mov)   \
    __OP_CODE(MOV_REG_IMM32,                  REG,    IMM32,  NONE,   mov)   \
    __OP_CODE(MOV_REG_IMM8,                   REG,    IMM8,   NONE,   mov)   \
    __OP_CODE(MOV_REG_MEM32_REG_OFF,          REG,    REG,    OFF32,  mov)   \
    __OP_CODE(MOV_REG_MEM8_REG_OFF,           REG,    REG,    OFF8,   mov)   \
    __OP_CODE(MOV_MEM32_REG_OFF_REG,          REG,    OFF32,  REG,    mov)   \
    __OP_CODE(MOV_MEM32_REG_OFF_IMM32,        REG,    OFF32,  IMM32,  mov)   \
    __OP_CODE(MOV_MEM8_REG_OFF_REG,           REG,    OFF8,   REG,    mov)   \
    __OP_CODE(LOAD_AH_F,                      NONE,   NONE,   NONE,   lahf)  \
    __OP_CODE(SET_REG_IF_ZERO,                REG,    NONE,   NONE,   sete)  \
    __OP_CODE(SET_REG_IF_NOT_ZERO,            REG,    NONE,   NONE,   setne) \
    __OP_CODE(SET_REG_IF_LESS,                REG,    NONE,   NONE,   setl)  \
    __OP_CODE(SET_REG_IF_LESS_UNSIGNED,       REG,    NONE,   NONE,   setb)  \
    __OP_CODE(SET_REG_IF_GREATER,             REG,    NONE,   NONE,   setg)  \
    __OP_CODE(SET_REG_IF_GREATER_UNSIGNED,    REG,    NONE,   NONE,   seta)  \
    __OP_CODE(SUB_REG_IMM8,                   REG,    IMM8,   NONE,   sub)   \
    __OP_CODE(SUB_REG_REG,                    REG,    REG,    NONE,   sub)   \
    __OP_CODE(ADD_REG_REG,                    REG,    REG,    NONE,   add)   \
    __OP_CODE(ADD_REG_IMM8,                   REG,    IMM8,   NONE,   add)   \
    __OP_CODE(MUL_REG_REG,                    REG,    REG,    NONE,   imul)  \
    __OP_CODE(MUL_REG_IMM8,                   REG,    IMM8,   NONE,   imul)  \
    __OP_CODE(DIV_REG,                        REG,    NONE,   NONE,   idiv)  \
    __OP_CODE(CMP_REG_IMM32,                  REG,    IMM32,  NONE,   cmp)   \
    __OP_CODE(CMP_REG_REG,                    REG,    REG,    NONE,   cmp)   \
    __OP_CODE(AND_REG_IMM32,                  REG,    IMM32,  NONE,   and)   \
    __OP_CODE(PUSH_REG,                       REG,    NONE,   NONE,   push)  \
    __OP_CODE(PUSH_IMM32,                     IMM32,  NONE,   NONE,   push)  \
    __OP_CODE(PUSH_MEM32_REG_OFF,             REG,    OFF32,  NONE,   push)  \
    __OP_CODE(PUSH_LABEL,                     LABEL,  NONE,   NONE,   push)  \
    __OP_CODE(POP_REG,                        REG,    NONE,   NONE,   pop)   \
    __OP_CODE(CALL_LABEL,                     LABEL,  NONE,   NONE,   call)  \
    __OP_CODE(RET,                            NONE,   NONE,   NONE,   ret)   \
    __OP_CODE(JUMP_LABEL,                     LABEL,  NONE,   NONE,   jmp)   \
    __OP_CODE(JUMP_LABEL_IF_ZERO,             LABEL,  NONE,   NONE,   jz)    \
    __OP_CODE(JUMP_LABEL_IF_NOT_ZERO,         LABEL,  NONE,   NONE,   jnz)   \
    __OP_CODE(FLOAD_FLOAT_MEM32_REG_OFF,      REG,    OFF32,  NONE,   fld)   \
    __OP_CODE(FLOAD_INT_MEM32_REG_OFF,        REG,    OFF32,  NONE,   fild)  \
    __OP_CODE(FSTORE_FLOAT_POP_MEM32_REG_OFF, REG,    OFF32,  NONE,   fstp)  \
    __OP_CODE(FSTORE_FLOAT_POP_MEM64_REG_OFF, REG,    OFF64,  NONE,   fstp)  \
    __OP_CODE(FADD_POP_REG_REG,               REG,    REG,    NONE,   faddp) \
    __OP_CODE(DOUBLE_TO_QUADWORD,             NONE,   NONE,   NONE,   cdq)

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
    X86Argument arg3;
} X86Instruction;

#define ENUMERATE_X86_LINE_TYPE \
    __TYPE(INSTRUCTION) \
    __TYPE(LABEL) \
    __TYPE(COMMENT) \
    __TYPE(BLANK)

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
    char label[80];
} X86Line;

typedef struct X86Code
{
    char **label_mem;
    int label_count;

    char **string_data;
    int string_data_count;

    char *externals;
    int external_count;
    int external_buffer;

    X86Line *lines;
    int line_count;
    int line_buffer;
} X86Code;

#define COMMENT_CODE(code, ...) \
    { \
        char buffer[80]; \
        sprintf(buffer, __VA_ARGS__); \
        x86_code_add_comment(code, buffer); \
    }

X86Code x86_code_new();
void x86_code_add_instruction(X86Code*, X86Instruction);
void x86_code_add_label(X86Code*, const char *label);
void x86_code_add_comment(X86Code*, const char *comment);
void x86_code_add_blank(X86Code*);
void x86_code_add_external(X86Code*, const char *external);
int x86_code_add_string_data(X86Code*, const char *str);
void free_x86_code(X86Code*);

X86Instruction x86(X86Code *code, enum X86OpCode op_code, ...);
void x86_dump(X86Code *code);

#endif // X86_ASM_H
