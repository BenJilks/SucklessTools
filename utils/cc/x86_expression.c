#include "x86_expression.h"
#include "dumpast.h"
#include <stdio.h>
#include <assert.h>

X86Value compile_cast(X86Code *code, X86Value *value, DataType *data_type)
{
    if (!(value->data_type.flags & DATA_TYPE_PRIMITIVE) || !(data_type->flags & DATA_TYPE_PRIMITIVE))
    {
        if (data_type_equals(&value->data_type, data_type))
            return *value;

        fprintf(stderr, "Error: Connot cast type '%s'", printable_data_type(&value->data_type));
        fprintf(stderr, " to '%s'\n", printable_data_type(data_type));
        return *value;
    }

    switch (value->data_type.primitive)
    {
        case PRIMITIVE_INT:
            switch (data_type->primitive)
            {
                case PRIMITIVE_INT:
                    return *value;
                case PRIMITIVE_CHAR:
                    COMMENT_CODE(code, "Cast int to char");
                    INST(X86_OP_CODE_SUB_REG_IMM8, X86_REG_ESP, 3);
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
                    INST(X86_OP_CODE_SUB_REG_IMM8, X86_REG_ESP, 3);
                    INST(X86_OP_CODE_MOV_MEM32_REG_OFF_IMM32, X86_REG_ESP, 0, 0);
                    INST(X86_OP_CODE_MOV_MEM8_REG_OFF_REG, X86_REG_ESP, 0, X86_REG_AL);
                    return (X86Value) { dt_int() };
                default:
                    break;
            }
            break;
        default:
            break;
    }

    fprintf(stderr, "Error: Connot cast type '%s'", printable_data_type(&value->data_type));
    fprintf(stderr, " to '%s'\n", printable_data_type(data_type));
    return *value;
}

static int get_variable_location(Symbol *variable)
{
    int location;
    if (variable->flags & SYMBOL_LOCAL)
        location = -symbol_size(variable) - variable->location;
    else if (variable->flags & SYMBOL_ARGUMENT)
        location = 8 + variable->location;
    else if (variable->flags & SYMBOL_MEMBER)
        location = variable->location;
    else
        assert (0);

    return location;
}

static void get_value_from_address(X86Code *code, DataType *type)
{
    switch (data_type_size(type))
    {
        case 1:
            INST(X86_OP_CODE_MOV_REG_MEM8_REG_OFF, X86_REG_BL, X86_REG_EAX, 0);
            INST(X86_OP_CODE_MOV_MEM8_REG_OFF_REG, X86_REG_ESP, -1, X86_REG_BL);
            INST(X86_OP_CODE_SUB_REG_IMM8, X86_REG_ESP, 1);
            break;
        case 4:
            INST(X86_OP_CODE_MOV_REG_MEM32_REG_OFF, X86_REG_EBX, X86_REG_EAX, 0);
            INST(X86_OP_CODE_MOV_MEM32_REG_OFF_REG, X86_REG_ESP, -4, X86_REG_EBX);
            INST(X86_OP_CODE_SUB_REG_IMM8, X86_REG_ESP, 4);
            break;
        default:
            assert (0);
    }
}

static X86Value compile_variable_pointer(X86Code *code, Symbol *variable)
{
    int location = get_variable_location(variable);
    if (variable->flags & SYMBOL_MEMBER)
    {
        INST(X86_OP_CODE_PUSH_IMM32, location);
    }
    else
    {
        INST(X86_OP_CODE_MOV_REG_REG, X86_REG_EAX, X86_REG_EBP);
        if (location > 0)
            INST(X86_OP_CODE_ADD_REG_IMM8, X86_REG_EAX, location);
        else if (location < 0)
            INST(X86_OP_CODE_SUB_REG_IMM8, X86_REG_EAX, -location);
        INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);
    }

    DataType type = variable->data_type;
    type.pointer_count += 1;
    return (X86Value) { type };
}

static X86Value compile_variable(X86Code *code, Symbol *variable)
{
    int location = get_variable_location(variable);
    int type_size = data_type_size(&variable->data_type);

    // NOTE: Arrays should return their pointer
    if (variable->flags & SYMBOL_ARRAY)
        return compile_variable_pointer(code, variable);

    switch (type_size)
    {
        case 1:
            INST(X86_OP_CODE_MOV_REG_MEM8_REG_OFF, X86_REG_AL, X86_REG_EBP, location);
            INST(X86_OP_CODE_MOV_MEM8_REG_OFF_REG, X86_REG_ESP, -1, X86_REG_AL);
            INST(X86_OP_CODE_SUB_REG_IMM8, X86_REG_ESP, 1);
            break;
        case 4:
            INST(X86_OP_CODE_PUSH_MEM32_REG_OFF, X86_REG_EBP, location);
            break;
        default:
            for (int i = 0; i < type_size; i++)
            {
                INST(X86_OP_CODE_MOV_REG_MEM8_REG_OFF, X86_REG_AL, X86_REG_EBP, location + i);
                INST(X86_OP_CODE_MOV_MEM8_REG_OFF_REG, X86_REG_ESP, -type_size + i, X86_REG_AL);
            }
            INST(X86_OP_CODE_SUB_REG_IMM8, X86_REG_ESP, type_size);
            break;
    }

    return (X86Value) { variable->data_type };
}

static void compile_string(X86Code *code, Value *value)
{
    int string_id = x86_code_add_string_data(code, lexer_printable_token_data(&value->s));
    char label[80];
    sprintf(label, "str%i", string_id);
    INST(X86_OP_CODE_PUSH_LABEL, label);
}

static X86Value compile_value(X86Code *code, Value *value, enum ExpressionMode mode)
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
            if (mode == EXPRESSION_MODE_LHS)
                return compile_variable_pointer(code, value->v);
            return compile_variable(code, value->v);
    }

    return (X86Value) { dt_void() };
}

static X86Value compile_fuction_call(X86Code *code, Expression *expression)
{
    Symbol *func_symbol = expression->left->value.v;
    Token func_name = func_symbol->name;
    int argument_size = 0;
    COMMENT_CODE(code, "Function call %s", lexer_printable_token_data(&func_name));
    for (int i = expression->argument_length - 1; i >= 0; i--)
    {
        COMMENT_CODE(code, "Argument %i", i);
        DataType argument_type = dt_int();
        if (i < func_symbol->param_count)
            argument_type = func_symbol->params[i].data_type;

        X86Value value = compile_expression(code, expression->arguments[i], EXPRESSION_MODE_RHS);
        compile_cast(code, &value, &argument_type);
        argument_size += data_type_size(&argument_type);
    }

    COMMENT_CODE(code, "Do call");
    INST(X86_OP_CODE_CALL_LABEL, lexer_printable_token_data(&func_name));
    INST(X86_OP_CODE_ADD_REG_IMM8, X86_REG_ESP, argument_size);
    if (data_type_size(&func_symbol->data_type) == 4)
        INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);
    else if (data_type_size(&func_symbol->data_type) != 0)
        assert (0);
    return (X86Value) { func_symbol->data_type };
}

static void compile_rhs_eax_lhs_ebx(X86Code *code, Expression *expression, enum ExpressionMode lhs_mode)
{
    COMMENT_CODE(code, "Left hand side");
    X86Value lhs = compile_expression(code, expression->left, lhs_mode);
    if (lhs_mode == EXPRESSION_MODE_RHS)
        compile_cast(code, &lhs, &expression->data_type);
    COMMENT_CODE(code, "Right hand side");
    X86Value rhs = compile_expression(code, expression->right, EXPRESSION_MODE_RHS);
    compile_cast(code, &rhs, &expression->data_type);

    INST(X86_OP_CODE_POP_REG, X86_REG_EBX);
    INST(X86_OP_CODE_POP_REG, X86_REG_EAX);
}

void compile_assign_variable(X86Code *code, Symbol *variable, X86Value *value)
{
    COMMENT_CODE(code, "Assign vairable %s", lexer_printable_token_data(&variable->name));
    compile_cast(code, value, &variable->data_type);

    int location = get_variable_location(variable);
    switch (data_type_size(&variable->data_type))
    {
        case 1:
            // FIXME: This doesn't look right to me
            INST(X86_OP_CODE_POP_REG, X86_REG_EAX);
            INST(X86_OP_CODE_MOV_MEM8_REG_OFF_REG, X86_REG_EBP, location, X86_REG_AL);
            break;
        case 4:
            INST(X86_OP_CODE_POP_REG, X86_REG_EAX);
            INST(X86_OP_CODE_MOV_MEM32_REG_OFF_REG, X86_REG_EBP, location, X86_REG_EAX);
            break;
        default:
            for (int i = 0; i < data_type_size(&variable->data_type); i++)
            {
                INST(X86_OP_CODE_MOV_REG_MEM8_REG_OFF, X86_REG_AL, X86_REG_ESP, i);
                INST(X86_OP_CODE_MOV_MEM8_REG_OFF_REG, X86_REG_EBP, location + i, X86_REG_AL);
            }
            INST(X86_OP_CODE_ADD_REG_IMM8, X86_REG_ESP, data_type_size(&variable->data_type));
            break;
    }
}

static X86Value compile_assign_expression(X86Code *code, Expression *expression)
{
    COMMENT_CODE(code, "Compile assign");
    COMMENT_CODE(code, "Compile value");
    compile_expression(code, expression->right, EXPRESSION_MODE_RHS);
    COMMENT_CODE(code, "Compile pointer");
    compile_expression(code, expression->left, EXPRESSION_MODE_LHS);
    COMMENT_CODE(code, "Pop pointer into eax");
    INST(X86_OP_CODE_POP_REG, X86_REG_EAX);

    COMMENT_CODE(code, "Do assign operation");
    switch (data_type_size(&expression->data_type))
    {
        case 1:
            INST(X86_OP_CODE_MOV_REG_MEM8_REG_OFF, X86_REG_AL, X86_REG_ESP, 0);
            INST(X86_OP_CODE_MOV_MEM8_REG_OFF_REG, X86_REG_EAX, 0, X86_REG_AL);
            break;
        case 4:
            INST(X86_OP_CODE_MOV_REG_MEM32_REG_OFF, X86_REG_EBX, X86_REG_ESP, 0);
            INST(X86_OP_CODE_MOV_MEM32_REG_OFF_REG, X86_REG_EAX, 0, X86_REG_EBX);
            break;
        default:
            for (int i = 0; i < data_type_size(&expression->data_type); i++)
            {
                INST(X86_OP_CODE_MOV_REG_MEM8_REG_OFF, X86_REG_BL, X86_REG_ESP, i);
                INST(X86_OP_CODE_MOV_MEM8_REG_OFF_REG, X86_REG_EAX, i, X86_REG_BL);
            }
            break;
    }

    return (X86Value) { expression->data_type };
}

static X86Value compile_index_expression(X86Code *code, Expression *expression, enum ExpressionMode mode)
{
    DataType offset_type = dt_int();

    COMMENT_CODE(code, "Compile index");
    COMMENT_CODE(code, "Compile value");
    compile_expression(code, expression->left, EXPRESSION_MODE_LHS);
    COMMENT_CODE(code, "Compile offset");
    X86Value offset = compile_expression(code, expression->right, EXPRESSION_MODE_RHS);
    compile_cast(code, &offset, &offset_type);

    COMMENT_CODE(code, "Add pointer and offset");
    INST(X86_OP_CODE_POP_REG, X86_REG_EBX);
    INST(X86_OP_CODE_POP_REG, X86_REG_EAX);
    INST(X86_OP_CODE_MUL_REG_IMM8, X86_REG_EBX, data_type_size(&expression->data_type));
    INST(X86_OP_CODE_ADD_REG_REG, X86_REG_EAX, X86_REG_EBX);

    if (mode == EXPRESSION_MODE_LHS)
        INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);
    else
        get_value_from_address(code, &expression->data_type);

    return (X86Value) { expression->data_type };
}

X86Value compile_expression(X86Code *code, Expression *expression, enum ExpressionMode mode)
{
    switch (expression->type)
    {
        case EXPRESSION_TYPE_VALUE:
            return compile_value(code, &expression->value, mode);
        case EXPRESSION_TYPE_ASSIGN:
            return compile_assign_expression(code, expression);
        case EXPRESSION_TYPE_ADD:
        {
            COMMENT_CODE(code, "Compile add");
            compile_rhs_eax_lhs_ebx(code, expression, EXPRESSION_MODE_RHS);

            COMMENT_CODE(code, "Do add operation");
            INST(X86_OP_CODE_ADD_REG_REG, X86_REG_EAX, X86_REG_EBX);
            INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);
            return (X86Value) { expression->data_type };
        }
        case EXPRESSION_TYPE_SUB:
        {
            COMMENT_CODE(code, "Compile sub");
            compile_rhs_eax_lhs_ebx(code, expression, EXPRESSION_MODE_RHS);

            COMMENT_CODE(code, "Do sub operation");
            INST(X86_OP_CODE_SUB_REG_REG, X86_REG_EAX, X86_REG_EBX);
            INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);
            return (X86Value) { expression->data_type };
        }
        case EXPRESSION_TYPE_MUL:
        {
            COMMENT_CODE(code, "Compile mul");
            compile_rhs_eax_lhs_ebx(code, expression, EXPRESSION_MODE_RHS);

            COMMENT_CODE(code, "Do mul operation");
            INST(X86_OP_CODE_MUL_REG_REG, X86_REG_EAX, X86_REG_EBX);
            INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);
            return (X86Value) { expression->data_type };
        }
        case EXPRESSION_TYPE_LESS_THAN:
        {
            COMMENT_CODE(code, "Compile less than");
            compile_rhs_eax_lhs_ebx(code, expression, EXPRESSION_MODE_RHS);

            COMMENT_CODE(code, "Do add operation");
            INST(X86_OP_CODE_CMP_REG_REG, X86_REG_EAX, X86_REG_EBX);
            INST(X86_OP_CODE_MOV_REG_IMM32, X86_REG_EAX, 0);
            INST(X86_OP_CODE_SET_REG_IF_LESS, X86_REG_AL);
            INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);
            return (X86Value) { expression->data_type };
        }
        case EXPRESSION_TYPE_DOT:
        {
            COMMENT_CODE(code, "Compile dot");

            COMMENT_CODE(code, "Left hand side");
            compile_expression(code, expression->left, EXPRESSION_MODE_LHS);
            compile_expression(code, expression->right, EXPRESSION_MODE_LHS);
            INST(X86_OP_CODE_POP_REG, X86_REG_EBX);
            INST(X86_OP_CODE_POP_REG, X86_REG_EAX);
            INST(X86_OP_CODE_ADD_REG_REG, X86_REG_EAX, X86_REG_EBX);

            if (mode == EXPRESSION_MODE_LHS)
                INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);
            else
                get_value_from_address(code, &expression->data_type);

            return (X86Value) { expression->data_type };
        }
        case EXPRESSION_TYPE_INDEX:
            return compile_index_expression(code, expression, mode);
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
            assert (0);
    }

    return (X86Value) { expression->data_type };
}
