#include "preproccessor.h"
#include "macro_condition.h"
#include "stream.h"
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#define DEBUG_PRE_PROCCESSOR

#ifdef DEBUG_PRE_PROCCESSOR
static int g_debug_indent = 0;
#define DEBUG(...) \
    do { \
        for (int i = 0; i < g_debug_indent; i++) fprintf(stderr, "  "); \
        fprintf(stderr, __VA_ARGS__); \
    } while (0)
#define DEBUG_START_SCOPE g_debug_indent++
#define DEBUG_END_SCOPE g_debug_indent--
#else
#define DEBUG(...)
#define DEBUG_START_SCOPE
#define DEBUG_END_SCOPE
#endif

enum CodeBlockState
{
    CODE_BLOCK_STATE_DEFAULT,
    CODE_BLOCK_STATE_IDENTIFIER,
    CODE_BLOCK_STATE_SLASH,
    CODE_BLOCK_STATE_COMMENT_LINE,
    CODE_BLOCK_STATE_COMMENT_MULTI,
    CODE_BLOCK_STATE_COMMENT_MULTI_STAR,
    CODE_BLOCK_STATE_MACRO_START,
    CODE_BLOCK_STATE_MACRO,
};

enum DefineState
{
    DEFINE_STATE_START,
    DEFINE_STATE_NAME,
    DEFINE_STATE_ARGUMENT_START,
    DEFINE_STATE_ARGUMENT_NEXT,
    DEFINE_STATE_ARGUMENT,
    DEFINE_STATE_VALUE,
    DEFINE_STATE_ESCAPE,
};

enum IdentifierState
{
    IDENTIFIER_STATE_DEFAULT,
    IDENTIFIER_STATE_ARGUMENT,
};

enum CallState
{
    CALL_STATE_START,
    CALL_STATE_IN_NAME,
    CALL_STATE_END,
};

enum IncludeState
{
    INCLUDE_STATE_START,
    INCLUDE_STATE_LOCAL,
    INCLUDE_STATE_GLOBAL,
};

enum BlockMode
{
    BLOCK_MODE_DEFUALT,
    BLOCK_MODE_DISABLED,
    BLOCK_MODE_IGNORE_MACROS,
    BLOCK_MODE_CONDITION,
};

enum EndBlockMode
{
    END_BLOCK_MODE_EOF,
    END_BLOCK_MODE_ENDIF,
    END_BLOCK_MODE_ELSE,
    END_BLOCK_MODE_ELIF,
};

typedef struct Define
{
    char name[80];
    char value[80];
    char params[80][80];
    int param_count;
} Define;

typedef struct DefinitionScope
{
    Define *defines;
    int count;
} DefinitionScope;

static enum EndBlockMode parse_block(Stream *input, Stream *output, const char *type_name,
    enum BlockMode mode, DefinitionScope *scope);

static DefinitionScope definition_scope_create()
{
    DefinitionScope scope;
    scope.defines = NULL;
    scope.count = 0;
    return scope;
}

static void definition_scope_add(
    DefinitionScope *scope, Define define)
{
    if (scope->defines == NULL)
        scope->defines = malloc(sizeof(Define));
    else
        scope->defines = realloc(scope->defines, sizeof(Define) * (scope->count + 1));
    scope->defines[scope->count++] = define;
}

static DefinitionScope definition_scope_copy(DefinitionScope *original)
{
    DefinitionScope scope;
    scope.count = original->count;
    scope.defines = malloc(sizeof(Define) * scope.count);
    memcpy(scope.defines, original->defines, sizeof(Define) * scope.count);
    return scope;
}

static void skip_macro(Stream *input)
{
    for (;;)
    {
        stream_read_next_char(input);
        if (input->peek == '\\')
        {
            stream_read_next_char(input);
            if (input->peek == '\n')
                continue;
        }
        if (input->peek == '\n' || input->peek == (char)EOF)
            break;
    }
}

static void parse_define(
    Stream *input, enum BlockMode mode, DefinitionScope *scope)
{
    if (mode == BLOCK_MODE_DISABLED)
    {
        skip_macro(input);
        return;
    }

    Define define;
    define.param_count = 0;

    enum DefineState state = DEFINE_STATE_START;
    enum DefineState return_state;
    int buffer_pointer = 0;
    for (;;)
    {
        stream_read_next_char(input);
        switch (state)
        {
            case DEFINE_STATE_START:
                if (input->peek == '\n')
                    assert (0);
                if (isspace(input->peek))
                    break;
                state = DEFINE_STATE_NAME;
                input->should_reconsume = 1;
                break;

            case DEFINE_STATE_NAME:
                if (!isalnum(input->peek) && input->peek != '_')
                {
                    define.name[buffer_pointer] = '\0';
                    buffer_pointer = 0;

                    if (input->peek == '(')
                    {
                        state = DEFINE_STATE_ARGUMENT_START;
                        break;
                    }
                    state = DEFINE_STATE_VALUE;
                    input->should_reconsume = 1;
                    break;
                }
                define.name[buffer_pointer++] = input->peek;
                break;

            case DEFINE_STATE_ARGUMENT_START:
                if (input->peek == '\n')
                    assert (0);
                if (isspace(input->peek))
                    break;
                state = DEFINE_STATE_ARGUMENT;
                input->should_reconsume = 1;
                break;

            case DEFINE_STATE_ARGUMENT:
                if (!isalnum(input->peek) && input->peek != '_')
                {
                    define.params[define.param_count][buffer_pointer] = '\0';
                    define.param_count++;
                    buffer_pointer = 0;
                    input->should_reconsume = 1;
                    state = DEFINE_STATE_ARGUMENT_NEXT;
                    break;
                }
                define.params[define.param_count][buffer_pointer++] = input->peek;
                break;

            case DEFINE_STATE_ARGUMENT_NEXT:
                if (input->peek == '\n')
                    assert (0);
                if (isspace(input->peek))
                    break;
                if (input->peek == ',')
                    state = DEFINE_STATE_ARGUMENT_START;
                else if (input->peek == ')')
                    state = DEFINE_STATE_VALUE;
                else
                    assert (0);
                break;

            case DEFINE_STATE_VALUE:
                if (input->peek == '\\')
                {
                    state = DEFINE_STATE_ESCAPE;
                    return_state = DEFINE_STATE_VALUE;
                    break;
                }
                if (input->peek == '\n')
                {
                    define.value[buffer_pointer] = '\0';
                    definition_scope_add(scope, define);
                    DEBUG("Define %s = %s\n", define.name, define.value);
                    return;
                }
                define.value[buffer_pointer++] = input->peek;
                break;

            case DEFINE_STATE_ESCAPE:
                state = return_state;
                if (input->peek == '\n')
                    break;
                input->should_reconsume = 1;
                break;
        }
    }
}

static int parse_if_condition(Stream *input, DefinitionScope *scope)
{
    Stream condition_value = stream_create_output_memory();
    for (;;)
    {
        stream_read_next_char(input);
        if (input->peek == '\\')
        {
            stream_read_next_char(input);
            continue;
        }
        if (input->peek == '\n')
            break;
        stream_write_char(&condition_value, input->peek);
    }
    stream_memory_output_to_input(&condition_value);

    Stream raw_condition = stream_create_output_memory();
    parse_block(&condition_value, &raw_condition, "If", BLOCK_MODE_CONDITION, scope);
    stream_memory_output_to_input(&raw_condition);
    raw_condition.memory[raw_condition.memory_length] = '\0';
    DEBUG("Condition: %s\n", raw_condition.memory);

    int condition_result = macro_condition_parse(&raw_condition);
    stream_close(&condition_value);
    stream_close(&raw_condition);
    return condition_result;
}

static void parse_if_block(
    Stream *input, Stream *output, enum BlockMode mode, DefinitionScope *scope, int condition_result)
{
    int is_disabled = (mode == BLOCK_MODE_DISABLED);

    enum EndBlockMode end_mode = parse_block(input, output, "Scope",
        condition_result && !is_disabled ? BLOCK_MODE_DEFUALT : BLOCK_MODE_DISABLED, scope);

    while (end_mode == END_BLOCK_MODE_ELIF)
    {
        int elif_result = parse_if_condition(input, scope);
        if (!condition_result && elif_result && !is_disabled)
            end_mode = parse_block(input, output, "Else Scope", BLOCK_MODE_DEFUALT, scope);
        else
            end_mode = parse_block(input, output, "Else Scope", BLOCK_MODE_DISABLED, scope);
        condition_result |= elif_result;
    }
    if (end_mode == END_BLOCK_MODE_ELSE)
    {
        parse_block(input, output, "Else Scope",
            condition_result || is_disabled ? BLOCK_MODE_DISABLED : BLOCK_MODE_DEFUALT, scope);
    }
}

static void parse_if(
    Stream *input, Stream *output, enum BlockMode mode, DefinitionScope *scope)
{
    int condition_result = 0;
    if (mode == BLOCK_MODE_DISABLED)
        skip_macro(input);
    else
        condition_result = parse_if_condition(input, scope);

    DEBUG("If\n");
    parse_if_block(input, output, mode, scope, condition_result);
}

static void parse_message(Stream *input, enum BlockMode mode, const char *name)
{
    if (mode == BLOCK_MODE_DISABLED)
    {
        skip_macro(input);
        return;
    }

    fprintf(stderr, "%s: ", name);
    for (;;)
    {
        stream_read_next_char(input);
        if (input->peek == '\n')
            break;
        fprintf(stderr, "%c", input->peek);
    }
    fprintf(stderr, "\n");
}

static Define *find_definition(DefinitionScope *scope, char *name)
{
    for (int i = 0; i < scope->count; i++)
    {
        if (!strcmp(scope->defines[i].name, name))
            return &scope->defines[i];
    }
    return NULL;
}

static void parse_name(Stream *input, char *buffer)
{
    int buffer_pointer = 0;
    int is_start = 1;
    for (;;)
    {
        stream_read_next_char(input);
        if (is_start)
        {
            if (isspace(input->peek))
                continue;
            buffer[buffer_pointer++] = input->peek;
            is_start = 0;
        }
        else
        {
            if (isspace(input->peek))
                break;
            buffer[buffer_pointer++] = input->peek;
        }
    }
    buffer[buffer_pointer] = '\0';
}

static void parse_ifdef(Stream *input, Stream *output, enum BlockMode mode, DefinitionScope *scope)
{
    char buffer[80];
    parse_name(input, buffer);
    DEBUG("Ifdef %s\n", buffer);

    Define *def = find_definition(scope, buffer);
    parse_if_block(input, output, mode, scope, def != NULL);
}

static void parse_ifndef(Stream *input, Stream *output, enum BlockMode mode, DefinitionScope *scope)
{
    char buffer[80];
    parse_name(input, buffer);
    DEBUG("Ifndef %s\n", buffer);

    Define *def = find_definition(scope, buffer);
    parse_if_block(input, output, mode, scope, def == NULL);
}

static void absolute_include_file_path(char *local_file_path, char *out_buffer)
{
    static char *include_locations[] =
    {
        "/usr/local/include/",
        "/usr/include/",
        "/usr/lib/clang/10.0.1/include/"
    };

    strcpy(out_buffer, local_file_path);
    if (access(out_buffer, F_OK) != -1)
        return;

    int include_location_count = (int)sizeof(include_locations) / (int)sizeof(include_locations[0]);
    for (int i = 0; i < include_location_count; i++)
    {
        strcpy(out_buffer, include_locations[i]);
        strcat(out_buffer, local_file_path);
        if (access(out_buffer, F_OK) != -1)
            return;
    }

    fprintf(stderr, "No file found with the name %s\n", local_file_path);
    assert (0);
}

static void parse_include(Stream *input, Stream *output, enum BlockMode mode, DefinitionScope *scope)
{
    if (mode == BLOCK_MODE_DISABLED)
    {
        skip_macro(input);
        return;
    }

    enum IncludeState state = INCLUDE_STATE_START;
    char buffer[80];
    int buffer_pointer = 0;
    int is_end = 0;
    while (!is_end)
    {
        stream_read_next_char(input);
        switch (state)
        {
            case INCLUDE_STATE_START:
                if (isspace(input->peek))
                    break;
                else if (input->peek == '"')
                    state = INCLUDE_STATE_LOCAL;
                else if (input->peek == '<')
                    state = INCLUDE_STATE_GLOBAL;
                else
                    assert (0);
                break;

            case INCLUDE_STATE_LOCAL:
                if (input->peek == '"')
                {
                    buffer[buffer_pointer] = '\0';
                    is_end = 1;
                    stream_read_next_char(input);
                    break;
                }
                buffer[buffer_pointer++] = input->peek;
                break;

            case INCLUDE_STATE_GLOBAL:
                if (input->peek == '>')
                {
                    buffer[buffer_pointer] = '\0';
                    is_end = 1;
                    stream_read_next_char(input);
                    break;
                }
                buffer[buffer_pointer++] = input->peek;
                break;
        }
    }

    char include_path[80];
    absolute_include_file_path(buffer, include_path);

    Stream file_stream = stream_create_input_file(include_path);
    parse_block(&file_stream, output, include_path, BLOCK_MODE_DEFUALT, scope);
    stream_close(&file_stream);
}

static void parse_undef(Stream *input, enum BlockMode mode, DefinitionScope *scope)
{
    if (mode == BLOCK_MODE_DISABLED)
    {
        skip_macro(input);
        return;
    }

    char buffer[80];
    parse_name(input, buffer);
    DEBUG("Undef %s\n", buffer);

    for (int i = 0; i < scope->count; i++)
    {
        if (!strcmp(scope->defines[i].name, buffer))
        {
            memcpy(scope->defines + i - 1, scope->defines + i, sizeof(Define) * (scope->count - i));
            break;
        }
    }
}

static void parse_macro(Stream *input, Stream *output, enum BlockMode mode,
    char *macro_name, DefinitionScope *scope)
{
    if (!strcmp(macro_name, "define"))
        parse_define(input, mode, scope);
    else if (!strcmp(macro_name, "if"))
        parse_if(input, output, mode, scope);
    else if (!strcmp(macro_name, "ifdef"))
        parse_ifdef(input, output, mode, scope);
    else if (!strcmp(macro_name, "ifndef"))
        parse_ifndef(input, output, mode, scope);
    else if (!strcmp(macro_name, "error"))
        parse_message(input, mode, "Error");
    else if (!strcmp(macro_name, "warning"))
        parse_message(input, mode, "Warning");
    else if (!strcmp(macro_name, "include"))
        parse_include(input, output, mode, scope);
    else if (!strcmp(macro_name, "undef"))
        parse_undef(input, mode, scope);
    else
    {
        fprintf(stderr, "Error: Unkown macro '%s'\n", macro_name);
        assert (0);
    }
}

static DefinitionScope parse_identifier_arguments(
    Stream *input, Define *def, DefinitionScope *scope)
{
    enum IdentifierState state = IDENTIFIER_STATE_DEFAULT;
    DefinitionScope arguments = definition_scope_copy(scope);
    Define current_argument;
    int buffer_pointer = 0;
    int arguement_count = 0;
    for (;;)
    {
        stream_read_next_char(input);
        switch (state)
        {
            case IDENTIFIER_STATE_DEFAULT:
                if (isspace(input->peek))
                    break;
                if (input->peek == '(')
                {
                    state = IDENTIFIER_STATE_ARGUMENT;
                    break;
                }
                input->should_reconsume = 1;
                return arguments;

            case IDENTIFIER_STATE_ARGUMENT:
                if (input->peek == ',' || input->peek == ')')
                {
                    strcpy(current_argument.name, def->params[arguement_count++]);
                    current_argument.value[buffer_pointer] = '\0';
                    definition_scope_add(&arguments, current_argument);

                    buffer_pointer = 0;
                    if (input->peek == ')')
                        return arguments;
                    break;
                }
                current_argument.value[buffer_pointer++] = input->peek;
                break;
        }
    }
}

static void parse_identifier(
    Stream *input, Stream *output, char *identifier, DefinitionScope *scope)
{
    Define *def = find_definition(scope, identifier);
    if (def == NULL)
    {
        stream_write_string(output, identifier);
        return;
    }

    DefinitionScope argument_scope = parse_identifier_arguments(input, def, scope);
    Stream macro_value_stream = stream_create_input_memory(def->value, strlen(def->value));
    parse_block(&macro_value_stream, output, def->name, BLOCK_MODE_IGNORE_MACROS, &argument_scope);
    free(argument_scope.defines);
}

static void parse_defined_call(Stream *input, Stream *output, DefinitionScope *scope)
{
    enum CallState state = CALL_STATE_START;
    int is_in_backets = 0;
    int buffer_pointer = 0;
    int is_end = 0;
    char buffer[80];

    while (!is_end)
    {
        stream_read_next_char(input);
        switch (state)
        {
            case CALL_STATE_START:
                if (isspace(input->peek))
                    continue;
                if (input->peek == '(')
                {
                    is_in_backets = 1;
                    break;
                }
                if (isalpha(input->peek) || input->peek == '_')
                {
                    state = CALL_STATE_IN_NAME;
                    input->should_reconsume = 1;
                    break;
                }
                assert (0);
                break;

            case CALL_STATE_IN_NAME:
                if (!isalnum(input->peek) && input->peek != '_')
                {
                    input->should_reconsume = 1;
                    if (is_in_backets)
                        state = CALL_STATE_END;
                    else
                        is_end = 1;
                    break;
                }
                buffer[buffer_pointer++] = input->peek;
                break;

            case CALL_STATE_END:
                if (input->peek == ')')
                    is_end = 1;
                break;
        }
    }
    buffer[buffer_pointer] = '\0';
    DEBUG("Defined call %s\n", buffer);

    Define *def = find_definition(scope, buffer);
    stream_write_string(output, def != NULL ? "1" : "0");
}

static enum EndBlockMode parse_block(Stream *input, Stream *output, const char *type_name,
    enum BlockMode mode, DefinitionScope *scope)
{
    DEBUG("Block %s%s\n", type_name, mode == BLOCK_MODE_DISABLED ? " (disabled)" : "");
    DEBUG_START_SCOPE;

    enum CodeBlockState state = CODE_BLOCK_STATE_DEFAULT;
    char buffer[80];
    int buffer_pointer = 0;
    for (;;)
    {
        stream_read_next_char(input);
        switch (state)
        {
            case CODE_BLOCK_STATE_DEFAULT:
                if (input->peek == (char)EOF)
                {
                    DEBUG_END_SCOPE;
                    DEBUG("End Block %s (EOF)\n", type_name);
                    return END_BLOCK_MODE_EOF;
                }
                if (isalpha(input->peek) || input->peek == '_')
                {
                    state = CODE_BLOCK_STATE_IDENTIFIER;
                    input->should_reconsume = 1;
                    break;
                }
                if (input->peek == '/')
                {
                    state = CODE_BLOCK_STATE_SLASH;
                    break;
                }
                if (input->peek == '#' && mode != BLOCK_MODE_IGNORE_MACROS && mode != BLOCK_MODE_CONDITION)
                {
                    state = CODE_BLOCK_STATE_MACRO_START;
                    break;
                }
                if (mode != BLOCK_MODE_DISABLED)
                    stream_write_char(output, input->peek);
                break;

            case CODE_BLOCK_STATE_IDENTIFIER:
                if (!isalnum(input->peek) && input->peek != '_')
                {
                    buffer[buffer_pointer] = '\0';
                    buffer_pointer = 0;
                    state = CODE_BLOCK_STATE_DEFAULT;
                    input->should_reconsume = 1;
                    if (mode == BLOCK_MODE_DISABLED)
                        break;

                    if (mode == BLOCK_MODE_CONDITION && !strcmp(buffer, "defined"))
                        parse_defined_call(input, output, scope);
                    else
                        parse_identifier(input, output, buffer, scope);
                    break;
                }
                buffer[buffer_pointer++] = input->peek;
                break;

            case CODE_BLOCK_STATE_SLASH:
                if (input->peek == '/')
                {
                    state = CODE_BLOCK_STATE_COMMENT_LINE;
                    break;
                }
                if (input->peek == '*')
                {
                    state = CODE_BLOCK_STATE_COMMENT_MULTI;
                    break;
                }
                state = CODE_BLOCK_STATE_DEFAULT;
                if (mode != BLOCK_MODE_DISABLED)
                    stream_write_char(output, input->peek);
                input->should_reconsume = 1;
                break;

            case CODE_BLOCK_STATE_COMMENT_LINE:
                if (input->peek == '\n')
                    state = CODE_BLOCK_STATE_DEFAULT;
                break;

            case CODE_BLOCK_STATE_COMMENT_MULTI:
                if (input->peek == '*')
                    state = CODE_BLOCK_STATE_COMMENT_MULTI_STAR;
                break;

            case CODE_BLOCK_STATE_COMMENT_MULTI_STAR:
                if (input->peek == '/')
                    state = CODE_BLOCK_STATE_DEFAULT;
                else
                    state = CODE_BLOCK_STATE_COMMENT_MULTI;
                break;

            case CODE_BLOCK_STATE_MACRO_START:
                if (isspace(input->peek))
                    break;
                state = CODE_BLOCK_STATE_MACRO;
                input->should_reconsume = 1;
                break;

            case CODE_BLOCK_STATE_MACRO:
                if (isspace(input->peek))
                {
                    buffer[buffer_pointer] = '\0';
                    buffer_pointer = 0;
                    if (!strcmp(buffer, "endif"))
                    {
                        DEBUG_END_SCOPE;
                        DEBUG("End Block %s (endif)\n", type_name);
                        return END_BLOCK_MODE_ENDIF;
                    }
                    if (!strcmp(buffer, "else"))
                    {
                        DEBUG_END_SCOPE;
                        DEBUG("End Block %s (else)\n", type_name);
                        return END_BLOCK_MODE_ELSE;
                    }
                    if (!strcmp(buffer, "elif"))
                    {
                        DEBUG_END_SCOPE;
                        DEBUG("End Block %s (elif)\n", type_name);
                        return END_BLOCK_MODE_ELIF;
                    }

                    parse_macro(input, output, mode, buffer, scope);
                    state = CODE_BLOCK_STATE_DEFAULT;
                    input->should_reconsume = 1;
                    break;
                }
                buffer[buffer_pointer++] = input->peek;
                break;
        }
    }
}

void dump_macro_map(DefinitionScope *scope)
{
    for (int i = 0; i < scope->count; i++)
    {
        Define *def = &scope->defines[i];
        printf("\"%s\",\"%s\"\n", def->name, def->value);
    }
}

void pre_proccess_file(
    Stream *input, Stream *output)
{
    DefinitionScope scope = definition_scope_create();
    parse_block(input, output, "Main", BLOCK_MODE_DEFUALT, &scope);
    dump_macro_map(&scope);
}
