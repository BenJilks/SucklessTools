#include "../dumpast.h"
#include "x86_expression.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static void unimplemented_cast(DataType *from, DataType *to)
{
    fprintf(stderr, "Internal Error (x86): Unimplemented cast from '%s'", printable_data_type(from));
    fprintf(stderr, " to '%s'\n", printable_data_type(to));
    assert (0);
}

static void comment_cast(X86Code *code, DataType *from, DataType *to)
{
    char comment[80];
    sprintf(comment, "Cast %s", printable_data_type(from));
    sprintf(comment + strlen(comment), " to %s", printable_data_type(to));
    COMMENT_CODE(code, comment);
}

X86Value compile_cast(X86Code *code, X86Value *value, DataType *to_type)
{
    if (!(value->data_type.flags & DATA_TYPE_PRIMITIVE) || !(to_type->flags & DATA_TYPE_PRIMITIVE))
    {
        if (data_type_equals(&value->data_type, to_type))
            return *value;

        // NOTE: This is invalid and we should have 
        //       not allowed this in parsing.
        assert (0);
    }
    
    comment_cast(code, &value->data_type, to_type);
    switch (value->data_type.primitive)
    {
        case PRIMITIVE_INT:
            switch (to_type->primitive)
            {
                case PRIMITIVE_INT:
                    return *value;

                case PRIMITIVE_CHAR:
                    INST(X86_OP_CODE_MOV_REG_MEM32_REG_OFF, X86_REG_EAX, X86_REG_ESP, 0);
                    INST(X86_OP_CODE_ADD_REG_IMM8, X86_REG_ESP, 3);
                    INST(X86_OP_CODE_MOV_MEM8_REG_OFF_REG, X86_REG_ESP, 0, X86_REG_AL);
                    return (X86Value) { dt_char() };

                case PRIMITIVE_FLOAT:
                    INST(X86_OP_CODE_FLOAD_INT_MEM32_REG_OFF, X86_REG_ESP, 0);
                    INST(X86_OP_CODE_FSTORE_FLOAT_POP_MEM32_REG_OFF, X86_REG_ESP, 0);
                    return (X86Value) { dt_float() };
                
                case PRIMITIVE_DOUBLE:
                    INST(X86_OP_CODE_FLOAD_INT_MEM32_REG_OFF, X86_REG_ESP, 0);
                    INST(X86_OP_CODE_SUB_REG_IMM8, X86_REG_ESP, 4);
                    INST(X86_OP_CODE_FSTORE_FLOAT_POP_MEM64_REG_OFF, X86_REG_ESP, 0);
                    return (X86Value) { dt_double() };

                default:
                    break;
            }
            break;
        case PRIMITIVE_FLOAT:
            switch (to_type->primitive)
            {
                case PRIMITIVE_INT:
                    INST(X86_OP_CODE_FLOAD_FLOAT_MEM32_REG_OFF, X86_REG_ESP, 0);
                    INST(X86_OP_CODE_FSTORE_INT_POP_MEM32_REG_OFF, X86_REG_ESP, 0);
                    return (X86Value) { dt_int() };
                
                case PRIMITIVE_CHAR:
                    INST(X86_OP_CODE_FLOAD_FLOAT_MEM32_REG_OFF, X86_REG_ESP, 0);
                    INST(X86_OP_CODE_FSTORE_INT_POP_MEM32_REG_OFF, X86_REG_ESP, 0);

                    INST(X86_OP_CODE_MOV_REG_MEM32_REG_OFF, X86_REG_EAX, X86_REG_ESP, 0);
                    INST(X86_OP_CODE_ADD_REG_IMM8, X86_REG_ESP, 3);
                    INST(X86_OP_CODE_MOV_MEM8_REG_OFF_REG, X86_REG_ESP, 0, X86_REG_AL);
                    return (X86Value) { dt_char() };

                case PRIMITIVE_FLOAT:
                    return *value;

                case PRIMITIVE_DOUBLE:
                    INST(X86_OP_CODE_FLOAD_FLOAT_MEM32_REG_OFF, X86_REG_ESP, 0);
                    INST(X86_OP_CODE_FSTORE_FLOAT_POP_MEM64_REG_OFF, X86_REG_ESP, -4);
                    INST(X86_OP_CODE_SUB_REG_IMM8, X86_REG_ESP, 4);
                    return (X86Value) { dt_double() };

                default:
                    break;
            }
            break;
        case PRIMITIVE_DOUBLE:
            switch (to_type->primitive)
            {
                case PRIMITIVE_INT:
                    INST(X86_OP_CODE_FLOAD_FLOAT_MEM64_REG_OFF, X86_REG_ESP, 0);
                    INST(X86_OP_CODE_ADD_REG_IMM8, X86_REG_ESP, 4);
                    INST(X86_OP_CODE_FSTORE_INT_POP_MEM32_REG_OFF, X86_REG_ESP, 0);
                    return (X86Value) { dt_int() };

                case PRIMITIVE_CHAR:
                    INST(X86_OP_CODE_FLOAD_FLOAT_MEM64_REG_OFF, X86_REG_ESP, 0);
                    INST(X86_OP_CODE_ADD_REG_IMM8, X86_REG_ESP, 4);
                    INST(X86_OP_CODE_FSTORE_INT_POP_MEM32_REG_OFF, X86_REG_ESP, 0);
                    
                    INST(X86_OP_CODE_MOV_REG_MEM32_REG_OFF, X86_REG_EAX, X86_REG_ESP, 0);
                    INST(X86_OP_CODE_ADD_REG_IMM8, X86_REG_ESP, 3);
                    INST(X86_OP_CODE_MOV_MEM8_REG_OFF_REG, X86_REG_ESP, 0, X86_REG_AL);
                    return (X86Value) { dt_char() };

                case PRIMITIVE_FLOAT:
                    INST(X86_OP_CODE_FLOAD_FLOAT_MEM64_REG_OFF, X86_REG_ESP, 0);
                    INST(X86_OP_CODE_ADD_REG_IMM8, X86_REG_ESP, 4);
                    INST(X86_OP_CODE_FSTORE_FLOAT_POP_MEM32_REG_OFF, X86_REG_ESP, 0);
                    return (X86Value) { dt_float() };

                case PRIMITIVE_DOUBLE:
                    return *value;

                default:
                    break;
            }
            break;
        case PRIMITIVE_CHAR:
            switch (to_type->primitive)
            {
                case PRIMITIVE_CHAR:
                    return *value;

                case PRIMITIVE_INT:
                    if (value->data_type.flags & DATA_TYPE_UNSIGNED)
                        INST(X86_OP_CODE_MOV_REG_MEM8_REG_OFF, X86_REG_AL, X86_REG_ESP, 0);
                    else
                        INST(X86_OP_CODE_MOVSX_REG_MEM8_REG_OFF, X86_REG_EAX, X86_REG_ESP, 0);
                    INST(X86_OP_CODE_ADD_REG_IMM8, X86_REG_ESP, 1);
                    INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);
                    return (X86Value) { dt_int() };
                
                case PRIMITIVE_FLOAT:
                    if (value->data_type.flags & DATA_TYPE_UNSIGNED)
                        INST(X86_OP_CODE_MOV_REG_MEM8_REG_OFF, X86_REG_AL, X86_REG_ESP, 0);
                    else
                        INST(X86_OP_CODE_MOVSX_REG_MEM8_REG_OFF, X86_REG_EAX, X86_REG_ESP, 0);
                    INST(X86_OP_CODE_ADD_REG_IMM8, X86_REG_ESP, 1);
                    INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);

                    INST(X86_OP_CODE_FLOAD_INT_MEM32_REG_OFF, X86_REG_ESP, 0);
                    INST(X86_OP_CODE_FSTORE_FLOAT_POP_MEM32_REG_OFF, X86_REG_ESP, 0);
                    return (X86Value) { dt_char() };
                
                case PRIMITIVE_DOUBLE:
                    if (value->data_type.flags & DATA_TYPE_UNSIGNED)
                        INST(X86_OP_CODE_MOV_REG_MEM8_REG_OFF, X86_REG_AL, X86_REG_ESP, 0);
                    else
                        INST(X86_OP_CODE_MOVSX_REG_MEM8_REG_OFF, X86_REG_EAX, X86_REG_ESP, 0);
                    INST(X86_OP_CODE_ADD_REG_IMM8, X86_REG_ESP, 1);
                    INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);

                    INST(X86_OP_CODE_FLOAD_INT_MEM32_REG_OFF, X86_REG_ESP, 0);
                    INST(X86_OP_CODE_SUB_REG_IMM8, X86_REG_ESP, 4);
                    INST(X86_OP_CODE_FSTORE_FLOAT_POP_MEM64_REG_OFF, X86_REG_ESP, 0);
                    return (X86Value) { dt_char() };

                default:
                    break;
            }
            break;
        default:
            break;
    }

    unimplemented_cast(&value->data_type, to_type);
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
    assert (!(variable->flags & SYMBOL_ENUM));

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
    if (variable->flags & SYMBOL_ENUM)
    {
        INST(X86_OP_CODE_PUSH_IMM32, variable->location);
        return (X86Value) { dt_int() };
    }

    int location = get_variable_location(variable);
    int type_size = data_type_size(&variable->data_type);

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
        case 8:
            INST(X86_OP_CODE_PUSH_MEM32_REG_OFF, X86_REG_EBP, location+4);
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
            INST(X86_OP_CODE_PUSH_IMM32, *(int*)&value->f);
            return (X86Value) { dt_float() };
        case VALUE_TYPE_STRING:
            compile_string(code, value);
            return (X86Value) { dt_const_char_pointer() };
        case VALUE_TYPE_CHAR:
            INST(X86_OP_CODE_MOV_REG_IMM8, X86_REG_AL, value->s.data[0]);
            INST(X86_OP_CODE_SUB_REG_IMM8, X86_REG_ESP, 1);
            INST(X86_OP_CODE_MOV_MEM8_REG_OFF_REG, X86_REG_ESP, 0, X86_REG_AL);
            return (X86Value) { dt_char() };
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
        X86Value value = compile_expression(code, expression->arguments[i], EXPRESSION_MODE_RHS);

        DataType argument_type = func_symbol->params[i].data_type;
        if (i >= func_symbol->param_count)
        {
            argument_type = value.data_type;
            if (argument_type.flags & DATA_TYPE_PRIMITIVE)
            {
                // NOTE: I think this is how it works?
                if (argument_type.primitive == PRIMITIVE_FLOAT)
                    argument_type = dt_double();
                else if (argument_type.primitive == PRIMITIVE_CHAR)
                    argument_type = dt_char();
            }
        }

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

static void compile_rhs_eax_lhs_ebx(X86Code *code, Expression *expression, enum ExpressionMode lhs_mode,
    X86Value *lhs, X86Value *rhs)
{
    COMMENT_CODE(code, "Left hand side");
    *lhs = compile_expression(code, expression->left, lhs_mode);
    if (lhs_mode == EXPRESSION_MODE_RHS)
        compile_cast(code, lhs, &expression->common_type);
    COMMENT_CODE(code, "Right hand side");
    *rhs = compile_expression(code, expression->right, EXPRESSION_MODE_RHS);
    compile_cast(code, rhs, &expression->common_type);

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
            INST(X86_OP_CODE_MOV_REG_MEM8_REG_OFF, X86_REG_AL, X86_REG_ESP, 0);
            INST(X86_OP_CODE_MOV_MEM8_REG_OFF_REG, X86_REG_EBP, location, X86_REG_AL);
            INST(X86_OP_CODE_ADD_REG_IMM8, X86_REG_ESP, 1);
            break;
        case 4:
            INST(X86_OP_CODE_POP_REG, X86_REG_EAX);
            INST(X86_OP_CODE_MOV_MEM32_REG_OFF_REG, X86_REG_EBP, location, X86_REG_EAX);
            break;
        case 8:
            INST(X86_OP_CODE_POP_REG, X86_REG_EAX);
            INST(X86_OP_CODE_MOV_MEM32_REG_OFF_REG, X86_REG_EBP, location, X86_REG_EAX);
            INST(X86_OP_CODE_POP_REG, X86_REG_EAX);
            INST(X86_OP_CODE_MOV_MEM32_REG_OFF_REG, X86_REG_EBP, location+4, X86_REG_EAX);
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

X86Value compile_add_operation(X86Code *code, X86Value *lhs, X86Value *rhs, DataType *return_type)
{
    // NOTE: we'll need this when we optimise, but not yet
    (void) lhs;
    (void) rhs;

    assert (return_type->flags & DATA_TYPE_PRIMITIVE);
    switch (return_type->primitive)
    {
        case PRIMITIVE_INT:
            INST(X86_OP_CODE_ADD_REG_REG, X86_REG_EAX, X86_REG_EBX);
            INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);
            break;
        case PRIMITIVE_CHAR:
            // NOTE: This doesn't look right to me
            INST(X86_OP_CODE_ADD_REG_REG, X86_REG_AL, X86_REG_BL);
            INST(X86_OP_CODE_MOV_MEM8_REG_OFF_REG, X86_REG_ESP, 0, X86_REG_AL);
            INST(X86_OP_CODE_SUB_REG_IMM8, X86_REG_ESP, 1);
            break;
        case PRIMITIVE_FLOAT:
            INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);
            INST(X86_OP_CODE_FLOAD_FLOAT_MEM32_REG_OFF, X86_REG_ESP, 0);
            INST(X86_OP_CODE_PUSH_REG, X86_REG_EBX);
            INST(X86_OP_CODE_FLOAD_FLOAT_MEM32_REG_OFF, X86_REG_ESP, 0);
            INST(X86_OP_CODE_ADD_REG_IMM8, X86_REG_ESP, 8);

            INST(X86_OP_CODE_FADD_POP_REG_REG, X86_REG_ST1, X86_REG_ST0);
            INST(X86_OP_CODE_FSTORE_FLOAT_POP_MEM32_REG_OFF, X86_REG_ESP, -4);
            INST(X86_OP_CODE_SUB_REG_IMM8, X86_REG_ESP, 4);
            break;
        default:
            assert (0);
    }

    return (X86Value) { *return_type };
}

X86Value compile_subtract_operation(X86Code *code, X86Value *lhs, X86Value *rhs, DataType *return_type)
{
    // NOTE: we'll need this when we optimise, but not yet
    (void) lhs;
    (void) rhs;

    assert (return_type->flags & DATA_TYPE_PRIMITIVE);
    switch (return_type->primitive)
    {
        case PRIMITIVE_INT:
            INST(X86_OP_CODE_SUB_REG_REG, X86_REG_EAX, X86_REG_EBX);
            INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);
            break;
        default:
            assert (0);
    }

    return (X86Value) { *return_type };
}

X86Value compile_greater_than_operation(X86Code *code, X86Value *lhs, X86Value *rhs, DataType *common_type, DataType *return_type)
{
    (void) lhs;
    (void) rhs;

    INST(X86_OP_CODE_CMP_REG_REG, X86_REG_EAX, X86_REG_EBX);
    INST(X86_OP_CODE_MOV_REG_IMM32, X86_REG_EAX, 0);
    if (common_type->flags & DATA_TYPE_UNSIGNED)
        INST(X86_OP_CODE_SET_REG_IF_GREATER_UNSIGNED, X86_REG_AL);
    else
        INST(X86_OP_CODE_SET_REG_IF_GREATER, X86_REG_AL);
    INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);
    return (X86Value) { *return_type };
}

X86Value compile_less_than_operation(X86Code *code, X86Value *lhs, X86Value *rhs, DataType *common_type, DataType *return_type)
{
    (void) lhs;
    (void) rhs;

    INST(X86_OP_CODE_CMP_REG_REG, X86_REG_EAX, X86_REG_EBX);
    INST(X86_OP_CODE_MOV_REG_IMM32, X86_REG_EAX, 0);
    if (common_type->flags & DATA_TYPE_UNSIGNED)
        INST(X86_OP_CODE_SET_REG_IF_LESS_UNSIGNED, X86_REG_AL);
    else
        INST(X86_OP_CODE_SET_REG_IF_LESS, X86_REG_AL);
    INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);
    return (X86Value) { *return_type };
}

X86Value compile_not_equals_operation(X86Code *code, X86Value *lhs, X86Value *rhs, DataType *return_type)
{
    (void) lhs;
    (void) rhs;
    assert (return_type->flags & DATA_TYPE_PRIMITIVE);

    switch (return_type->primitive)
    {
        case PRIMITIVE_CHAR:
        case PRIMITIVE_INT:
        case PRIMITIVE_FLOAT:
            INST(X86_OP_CODE_CMP_REG_REG, X86_REG_EAX, X86_REG_EBX);
            INST(X86_OP_CODE_MOV_REG_IMM32, X86_REG_EAX, 0);
            INST(X86_OP_CODE_SET_REG_IF_NOT_ZERO, X86_REG_AL);
            INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);
            break;
        default:
            assert (0);
    }
    return (X86Value) { *return_type };
}

// NOTE: We only can do some operation for ints right now
#define CHECK_INT_TYPES assert (expression->data_type.flags & DATA_TYPE_PRIMITIVE && expression->data_type.primitive == PRIMITIVE_INT)

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
            X86Value lhs, rhs;
            compile_rhs_eax_lhs_ebx(code, expression, EXPRESSION_MODE_RHS, &lhs, &rhs);

            COMMENT_CODE(code, "Do add operation");
            return compile_add_operation(code, &lhs, &rhs, &expression->data_type);
        }
        case EXPRESSION_TYPE_SUB:
        {
            COMMENT_CODE(code, "Compile sub");
            X86Value lhs, rhs;
            compile_rhs_eax_lhs_ebx(code, expression, EXPRESSION_MODE_RHS, &lhs, &rhs);

            COMMENT_CODE(code, "Do sub operation");
            return compile_subtract_operation(code, &lhs, &rhs, &expression->data_type);
        }
        case EXPRESSION_TYPE_MUL:
        {
            CHECK_INT_TYPES;
            COMMENT_CODE(code, "Compile mul");
            X86Value lhs, rhs;
            compile_rhs_eax_lhs_ebx(code, expression, EXPRESSION_MODE_RHS, &lhs, &rhs);

            COMMENT_CODE(code, "Do mul operation");
            INST(X86_OP_CODE_MUL_REG_REG, X86_REG_EAX, X86_REG_EBX);
            INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);
            return (X86Value) { expression->data_type };
        }
        case EXPRESSION_TYPE_DIV:
        {
            CHECK_INT_TYPES;
            COMMENT_CODE(code, "Compile div");
            X86Value lhs, rhs;
            compile_rhs_eax_lhs_ebx(code, expression, EXPRESSION_MODE_RHS, &lhs, &rhs);

            COMMENT_CODE(code, "Do div operation");
            INST(X86_OP_CODE_MOV_REG_REG, X86_REG_ECX, X86_REG_EBX);
            INST(X86_OP_CODE_DOUBLE_TO_QUADWORD);
            INST(X86_OP_CODE_DIV_REG, X86_REG_ECX);
            INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);
            return (X86Value) { expression->data_type };
        }
        case EXPRESSION_TYPE_LESS_THAN:
        {
            CHECK_INT_TYPES;
            COMMENT_CODE(code, "Compile less than");
            X86Value lhs, rhs;
            compile_rhs_eax_lhs_ebx(code, expression, EXPRESSION_MODE_RHS, &lhs, &rhs);

            COMMENT_CODE(code, "Do operation");
            return compile_less_than_operation(code, &lhs, &rhs, &expression->common_type, &expression->data_type);
        }
        case EXPRESSION_TYPE_GREATER_THAN:
        {
            CHECK_INT_TYPES;
            COMMENT_CODE(code, "Compile greater than");
            X86Value lhs, rhs;
            compile_rhs_eax_lhs_ebx(code, expression, EXPRESSION_MODE_RHS, &lhs, &rhs);

            COMMENT_CODE(code, "Do operation");
            return compile_greater_than_operation(code, &lhs, &rhs, &expression->common_type, &expression->data_type);
        }
        case EXPRESSION_TYPE_NOT_EQUALS:
        {
            COMMENT_CODE(code, "Compile not equals");
            X86Value lhs, rhs;
            compile_rhs_eax_lhs_ebx(code, expression, EXPRESSION_MODE_RHS, &lhs, &rhs);

            COMMENT_CODE(code, "Do operation");
            return compile_not_equals_operation(code, &lhs, &rhs, &expression->data_type);
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
        case EXPRESSION_TYPE_INVERT:
        {
            CHECK_INT_TYPES;

            compile_expression(code, expression->left, EXPRESSION_MODE_RHS);
            INST(X86_OP_CODE_POP_REG, X86_REG_EBX);
            INST(X86_OP_CODE_MOV_REG_IMM32, X86_REG_EAX, 0);
            INST(X86_OP_CODE_SUB_REG_REG, X86_REG_EAX, X86_REG_EBX);
            INST(X86_OP_CODE_PUSH_REG, X86_REG_EAX);
            return (X86Value) { expression->data_type };
        }
        case EXPRESSION_TYPE_CAST:
        {
            X86Value value = compile_expression(code, expression->left, EXPRESSION_MODE_RHS);
            compile_cast(code, &value, &expression->data_type);
            return (X86Value) { expression->data_type };
        }
        case EXPRESSION_TYPE_REF:
            assert(expression->left->type == EXPRESSION_TYPE_VALUE);
            if (expression->left->value.type != VALUE_TYPE_VARIABLE)
                lexer_error(NULL, "Can only take pointer of non literals");
            else
                return compile_variable_pointer(code, expression->left->value.v);
            break;
        default:
            assert (0);
    }

    return (X86Value) { expression->data_type };
}
