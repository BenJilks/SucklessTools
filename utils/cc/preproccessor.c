#include "preproccessor.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

typedef struct OutBuffer
{
    char *data;
    int buffer_size;
    int size;
} OutBuffer;

enum Mode
{
    MODE_START_OF_LINE,
    MODE_STATEMENT,
    MODE_CODE_NAME,
};

typedef struct Define
{
    char name[80];
    char value[80];
} Define;

typedef struct State
{
    enum Mode mode;
    char name_buffer[80];
    int name_buffer_pointer;

    Define definitions[80];
    int definition_count;

    int should_ignore_code_depth;
    int if_depth;
} State;

static char *pre_proccess_file_with_state(const char *file_path, int *out_length, State *state);

static OutBuffer make_buffer()
{
    OutBuffer buffer;
    buffer.data = malloc(80);
    buffer.buffer_size = 80;
    buffer.size = 0;
    return buffer;
}

static void append_buffer(OutBuffer *buffer, char c)
{
    if (buffer->size >= buffer->buffer_size)
    {
        buffer->buffer_size += 80;
        buffer->data = realloc(buffer->data, buffer->buffer_size);
    }
    buffer->data[buffer->size++] = c;
}

static void append_string(OutBuffer *buffer, const char *str)
{
    int len = strlen(str);
    for (int i = 0; i < len; i++)
        append_buffer(buffer, str[i]);
}

static int ifdef(const char *name, State *state)
{
    for (int i = 0; i < state->definition_count; i++)
    {
        if (!strcmp(name, state->definitions[i].name))
            return 1;
    }
    return 0;
}

static void check_define(const char *name, OutBuffer *out_buffer, State *state)
{
    for (int i = 0; i < state->definition_count; i++)
    {
        if (!strcmp(name, state->definitions[i].name))
        {
            append_string(out_buffer, state->definitions[i].value);
            return;
        }
    }

    append_string(out_buffer, name);
}

static void absolute_include_file_path(char *local_file_path, char *out_buffer)
{
    strcpy(out_buffer, local_file_path);
    if (access(out_buffer, F_OK) != -1)
        return;

    strcpy(out_buffer, "/usr/local/include/");
    strcat(out_buffer, local_file_path);
    if (access(out_buffer, F_OK) != -1)
        return;

    strcpy(out_buffer, "/usr/include/");
    strcat(out_buffer, local_file_path);
    if (access(out_buffer, F_OK) != -1)
        return;

    fprintf(stderr, "No file found with the name %s\n", local_file_path);
    assert (0);
}

static void include_file(char *file_path, OutBuffer *out_buffer, State *state)
{
    char absolute_path[80];
    int data_len;
    absolute_include_file_path(file_path, absolute_path);

    const char *data = pre_proccess_file_with_state(absolute_path, &data_len, state);
    for (int i = 0; i < data_len; i++)
        append_buffer(out_buffer, data[i]);
    free((void*)data);
}

static void parse_statement(const char *src, OutBuffer *out_buffer, State *state)
{
    char buffer[80];
    int buffer_pointer = 0;

#define READ_UNTIL(condition, into) \
do { \
    while(*src != '\0' && !(condition)) \
        into[buffer_pointer++] = *(src++); \
    into[buffer_pointer] = '\0'; \
    buffer_pointer = 0; \
} while (0)
#define SKIP_WHITE_SPACE READ_UNTIL(!isspace(*src), buffer)

    READ_UNTIL(isspace(*src), buffer);
    if (!strcmp(buffer, "define"))
    {
        Define *define = &state->definitions[state->definition_count++];
        SKIP_WHITE_SPACE;
        READ_UNTIL(isspace(*src), define->name);
        SKIP_WHITE_SPACE;
        READ_UNTIL(0, define->value);
    }
    else if (!strcmp(buffer, "include"))
    {
        SKIP_WHITE_SPACE;
        src += 1;
        if (*(src-1) == '"')
            READ_UNTIL(*src == '"', buffer);
        else if (*(src-1) == '<')
            READ_UNTIL(*src == '>', buffer);
        else
            assert (0);
        include_file(buffer, out_buffer, state);
    }
    else if (!strcmp(buffer, "if"))
    {
        SKIP_WHITE_SPACE;
        READ_UNTIL(!isdigit(*src), buffer);
        state->if_depth += 1;
        if (!atoi(buffer))
            state->should_ignore_code_depth = state->if_depth;
    }
    else if (!strcmp(buffer, "ifdef"))
    {
        SKIP_WHITE_SPACE;
        READ_UNTIL(isspace(*src), buffer);
        state->if_depth += 1;
        if (!ifdef(buffer, state))
            state->should_ignore_code_depth = state->if_depth;
    }
    else if (!strcmp(buffer, "ifndef"))
    {
        SKIP_WHITE_SPACE;
        READ_UNTIL(isspace(*src), buffer);
        state->if_depth += 1;
        if (ifdef(buffer, state))
            state->should_ignore_code_depth = state->if_depth;
    }
    else if (!strcmp(buffer, "endif"))
    {
        state->if_depth -= 1;
        if (state->if_depth < state->should_ignore_code_depth)
            state->should_ignore_code_depth = -1;
    }
    else
    {
        printf("Unkown pre-proccessor statement '%s'\n", buffer);
        assert (0);
    }
}

static void proccess_char(char c, OutBuffer *out_buffer, State *state)
{
    switch (state->mode)
    {
        case MODE_START_OF_LINE:
            if (c == '#')
            {
                state->mode = MODE_STATEMENT;
                state->name_buffer_pointer = 0;
                break;
            }
            if (!isspace(c))
            {
                if (isalnum(c))
                    state->name_buffer[state->name_buffer_pointer++] = c;
                else
                    append_buffer(out_buffer, c);
                state->mode = MODE_CODE_NAME;
                break;
            }
            append_buffer(out_buffer, c);
            break;
        case MODE_STATEMENT:
            if (c == '\n' || c == '\r')
            {
                state->name_buffer[state->name_buffer_pointer] = '\0';
                state->name_buffer_pointer = 0;
                state->mode = MODE_START_OF_LINE;
                parse_statement(state->name_buffer, out_buffer, state);
                break;
            }
            state->name_buffer[state->name_buffer_pointer++] = c;
            break;
        case MODE_CODE_NAME:
            if (c == '\n' || c == '\r')
                state->mode = MODE_START_OF_LINE;
            if (state->should_ignore_code_depth != -1 && state->if_depth >= state->should_ignore_code_depth)
                break;
            if (!isalnum(c))
            {
                state->name_buffer[state->name_buffer_pointer] = '\0';
                state->name_buffer_pointer = 0;
                check_define(state->name_buffer, out_buffer, state);
                append_buffer(out_buffer, c);
                break;
            }
            state->name_buffer[state->name_buffer_pointer++] = c;
            break;
    }

}

static char *pre_proccess_file_with_state(const char *file_path, int *out_length, State *state)
{
    FILE *file = fopen(file_path, "r");
    OutBuffer out_buffer = make_buffer();

    char buffer[80];
    for (;;)
    {
        int nread = fread(buffer, sizeof(char), sizeof(buffer), file);
        if (nread == 0)
            break;

        for (int i = 0; i < nread; i++)
        {
            char c = buffer[i];
            proccess_char(c, &out_buffer, state);
        }
    }

    fclose(file);
    *out_length = out_buffer.size;
    return out_buffer.data;
}

const char *pre_proccess_file(const char *file_path, int *out_length)
{
    State state;
    state.definition_count = 0;
    state.mode = MODE_START_OF_LINE;
    state.should_ignore_code_depth = -1;
    state.if_depth = 0;
    return pre_proccess_file_with_state(file_path, out_length, &state);
}
