#include "x86.h"
#include "dumpast.h"
#include <assert.h>
#include <stdio.h>

#define INST(...) x86_code_add_instruction(code, x86(code, __VA_ARGS__))

typedef struct X86Value
{
    DataType data_type;
} X86Value;

static void compile_string(X86Code *code, Value *value)
{
    int string_id = x86_code_add_string_data(code, lexer_printable_token_data(&value->s));
    char label[80];
    sprintf(label, "str%i", string_id);
    INST(X86_OP_CODE_PUSH_LABEL, label);
}

static int get_variable_location(Symbol *variable)
{
    int location;
    if (variable->flags & SYMBOL_LOCAL)
        location = -4 - variable->location;
    else if (variable->flags & SYMBOL_ARGUMENT)
        location = 8 + variable->location;
    else
        assert (0);

    return location;
}

static X86Value compile_variable(X86Code *code, Symbol *variable)
{
    int location = get_variable_location(variable);
    INST(X86_OP_CODE_PUSH_MEM32_REG_OFF, X86_REG_EBP, location);
    return (X86Value) { variable->data_type };
}

static X86Value compile_variable_pointer(X86Code *code, Symbol *variable)
{
    int location = get_variable_location(variable);
    INST(X86_OP_CODE_MOV_REG_REG, X86_REG_EAX, X86_REG_EBP);
    if (location > 0)
        INST(X86_OP_CODE_ADD_REG_IMM8, X86_REG_EAX, location);
    else if (location < 0)
        INST(X86_OP_CODE_SUB_REG_IMM8, X86_REG_EAX, -location);
    INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);

    DataType type = variable->data_type;
    type.pointer_count += 1;
    return (X86Value) { type };
}

static X86Value compile_value(X86Code *code, Value *value)
{
    switch (value->type)
    {
        case VALUE_TYPE_INT:
            INST(X86_OP_CODE_PUSH_IMM32, value->i);
            return (X86Value) { dt_int() };
        case VALUE_TYPE_FLOAT:
            INST(X86_OP_CODE_PUSH_IMM32, value->f);
            return (X86Value) { dt_float() };
        case VALUE_TYPE_STRING:
            compile_string(code, value);
            return (X86Value) { dt_const_char_pointer() };
        case VALUE_TYPE_VARIABLE:
            if (!value->v)
                break;

            return compile_variable(code, value->v);
    }

    return (X86Value) { dt_void() };
}

X86Value compile_cast(X86Code *code, X86Value *value, DataType *data_type)
{
    assert (value->data_type.flags & DATA_TYPE_PRIMITIVE);
    assert (data_type->flags & DATA_TYPE_PRIMITIVE);

    switch (value->data_type.primitive)
    {
        case PRIMITIVE_INT:
            switch (data_type->primitive)
            {
                case PRIMITIVE_INT:
                    return *value;
                case PRIMITIVE_CHAR:
                    COMMENT_CODE(code, "Cast int to char");
                    INST(X86_OP_CODE_ADD_REG_IMM8, X86_REG_ESP, 3);
                    return (X86Value) { dt_char() };
                default:
                    break;
            }
            break;
        case PRIMITIVE_FLOAT:
            switch (data_type->primitive)
            {
                case PRIMITIVE_FLOAT:
                    return *value;
                default:
                    break;
            }
            break;
        case PRIMITIVE_DOUBLE:
            switch (data_type->primitive)
            {
                case PRIMITIVE_DOUBLE:
                    return *value;
                default:
                    break;
            }
            break;
        case PRIMITIVE_CHAR:
            switch (data_type->primitive)
            {
                case PRIMITIVE_CHAR:
                    return *value;
                case PRIMITIVE_INT:
                    COMMENT_CODE(code, "Cast char to int");
                    INST(X86_OP_CODE_MOV_REG_MEM8_REG_OFF, X86_REG_AL, X86_REG_ESP, 0);
                    INST(X86_OP_CODE_MOV_MEM32_REG_OFF_IMM32, X86_REG_ESP, 0, 0);
                    INST(X86_OP_CODE_SUB_REG_IMM8, X86_REG_ESP, 3);
                    return (X86Value) { dt_int() };
                default:
                    break;
            }
            break;
        default:
            break;
    }

    printf("Error: Connot cast type '%s'", printable_data_type(&value->data_type));
    printf(" to '%s'\n", printable_data_type(data_type));
    return *value;
}

static X86Value compile_expression(X86Code *code, Expression *expression);
static X86Value compile_fuction_call(X86Code *code, Expression *expression)
{
    Symbol *func_symbol = expression->left->value.v;
    Token func_name = func_symbol->name;
    COMMENT_CODE(code, "Function call %s", lexer_printable_token_data(&func_name));
    for (int i = expression->argument_length - 1; i >= 0; i--)
    {
        COMMENT_CODE(code, "Argument %i", i);
        DataType argument_type = dt_int();
        if (i < func_symbol->param_count)
            argument_type = func_symbol->params[i].data_type;

        X86Value value = compile_expression(code, expression->arguments[i]);
        compile_cast(code, &value, &argument_type);
    }

    COMMENT_CODE(code, "Do call");
    INST(X86_OP_CODE_CALL_LABEL, lexer_printable_token_data(&func_name));
    INST(X86_OP_CODE_ADD_REG_IMM8, X86_REG_ESP, expression->argument_length * 4);
    INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);
    return (X86Value) { func_symbol->data_type };
}

static X86Value compile_expression(X86Code *code, Expression *expression)
{
    switch (expression->type)
    {
        case EXPRESSION_TYPE_VALUE:
            return compile_value(code, &expression->value);
        case EXPRESSION_TYPE_ADD:
        {
            COMMENT_CODE(code, "Compile add");
            COMMENT_CODE(code, "Left hand side");
            X86Value lhs = compile_expression(code, expression->left);
            compile_cast(code, &lhs, &expression->data_type);
            COMMENT_CODE(code, "Right hand side");
            X86Value rhs = compile_expression(code, expression->right);
            compile_cast(code, &rhs, &expression->data_type);

            COMMENT_CODE(code, "Do add operation");
            INST(X86_OP_CODE_POP_REG, X86_REG_EBX);
            INST(X86_OP_CODE_POP_REG, X86_REG_EAX);
            INST(X86_OP_CODE_ADD_REG_REG, X86_REG_EAX, X86_REG_EBX);
            INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);
            return (X86Value) { expression->data_type };
        }
        case EXPRESSION_TYPE_FUNCTION_CALL:
            return compile_fuction_call(code, expression);
        case EXPRESSION_TYPE_REF:
            assert(expression->left->type == EXPRESSION_TYPE_VALUE);
            if (expression->left->value.type != VALUE_TYPE_VARIABLE)
                lexer_error("Can only take pointer of non literals");
            else
                return compile_variable_pointer(code, expression->left->value.v);
            break;
        default:
            break;
    }

    return (X86Value) { expression->data_type };
}

static void compile_assign_variable(X86Code *code, Symbol *variable, X86Value *value)
{
    COMMENT_CODE(code, "Assign vairable %s", lexer_printable_token_data(&variable->name));
    compile_cast(code, value, &variable->data_type);
    INST(X86_OP_CODE_POP_REG, X86_REG_EAX);

    int location = get_variable_location(variable);
    switch (variable->data_type.size)
    {
        case 1:
            INST(X86_OP_CODE_MOV_MEM8_REG_OFF_REG, X86_REG_EBP, location, X86_REG_AL);
            break;
        case 4:
            INST(X86_OP_CODE_MOV_MEM32_REG_OFF_REG, X86_REG_EBP, location, X86_REG_EAX);
            break;
        default:
            printf("Error: Unknown size %i\n", variable->data_type.size);
            assert (0);
    }
}

static void compile_function_return(X86Code *code)
{
    INST(X86_OP_CODE_MOV_REG_REG, X86_REG_ESP, X86_REG_EBP);
    INST(X86_OP_CODE_POP_REG, X86_REG_EBP);
    INST(X86_OP_CODE_RET);
}

static void compile_decleration(X86Code *code, Statement *statement)
{
    if (statement->expression)
    {
        X86Value value = compile_expression(code, statement->expression);
        compile_assign_variable(code, &statement->symbol, &value);
    }

    if (statement->next)
        compile_decleration(code, statement->next);
}

static void compile_scope(X86Code *code, Scope *scope)
{
    for (int i = 0; i < scope->statement_count; i++)
    {
        Statement *statement = &scope->statements[i];
        switch (statement->type)
        {
            case STATEMENT_TYPE_DECLARATION:
                compile_decleration(code, statement);
                break;
            case STATEMENT_TYPE_EXPRESSION:
                compile_expression(code, statement->expression);
                INST(X86_OP_CODE_POP_REG, X86_REG_EAX);
                break;
            case STATEMENT_TYPE_RETURN:
                compile_expression(code, statement->expression);
                INST(X86_OP_CODE_POP_REG, X86_REG_EAX);
                compile_function_return(code);
                break;
            default:
                break;
        }
    }
}

static void compile_function(X86Code *code, Function *function)
{
    if (!function->body)
    {
        x86_code_add_external(code, lexer_printable_token_data(&function->func_symbol.name));
        return;
    }

    x86_code_add_label(code, lexer_printable_token_data(&function->func_symbol.name));
    INST(X86_OP_CODE_PUSH_REG, X86_REG_EBP);
    INST(X86_OP_CODE_MOV_REG_REG, X86_REG_EBP, X86_REG_ESP);
    INST(X86_OP_CODE_SUB_REG_IMM8, X86_REG_ESP, function->stack_size);
    x86_code_add_blank(code);
    compile_scope(code, function->body);

    x86_code_add_blank(code);
    compile_function_return(code);
    x86_code_add_blank(code);
}

X86Code x86_compile_unit(Unit *unit)
{
    X86Code code = x86_code_new();
    for (int i = 0; i < unit->function_count; i++)
        compile_function(&code, &unit->functions[i]);

    return code;
}
