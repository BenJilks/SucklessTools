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
    MODE_STATEMENT_NAME,
    MODE_DEFINE_NAME,
    MODE_DEFINE_BODY,
    MODE_INCLUDE,
    MODE_INCLUDE_LOCAL,
    MODE_INCLUDE_GLOBAL,
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
} State;

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

static void append_string(OutBuffer *buffer, char *str)
{
    int len = strlen(str);
    for (int i = 0; i < len; i++)
        append_buffer(buffer, str[i]);
}

static void check_define(char *name, OutBuffer *out_buffer, State *state)
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

static void include_file(char *file_path, OutBuffer *out_buffer)
{
    char absolute_path[80];
    absolute_include_file_path(file_path, absolute_path);

    FILE *file = fopen(absolute_path, "r");
    char buffer[80];
    for (;;)
    {
        int nread = fread(buffer, sizeof(char), sizeof(buffer), file);
        if (nread == 0)
            break;

        for (int i = 0; i < nread; i++)
            append_buffer(out_buffer, buffer[i]);
    }

    fclose(file);
}

static void proccess_char(char c, OutBuffer *out_buffer, State *state)
{
    switch (state->mode)
    {
        case MODE_START_OF_LINE:
            if (c == '#')
            {
                state->mode = MODE_STATEMENT_NAME;
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
        case MODE_STATEMENT_NAME:
            if (c == '\n' || c == '\r')
                assert (0);
            if (isspace(c))
            {
                state->name_buffer[state->name_buffer_pointer] = '\0';
                state->name_buffer_pointer = 0;
                if (!strcmp(state->name_buffer, "define"))
                    state->mode = MODE_DEFINE_NAME;
                else if (!strcmp(state->name_buffer, "include"))
                    state->mode = MODE_INCLUDE;
                else
                    assert (0);
                break;
            }

            state->name_buffer[state->name_buffer_pointer++] = c;
            break;
        case MODE_DEFINE_NAME:
            if (c == '\n' || c == '\r')
            {
                state->name_buffer[state->name_buffer_pointer] = '\0';
                state->name_buffer_pointer = 0;
                strcpy(state->definitions[state->definition_count].name, state->name_buffer);
                state->definitions[state->definition_count].value[0] = '\0';
                state->definition_count += 1;
                state->mode = MODE_START_OF_LINE;
                break;
            }
            if (isspace(c))
            {
                state->name_buffer[state->name_buffer_pointer] = '\0';
                state->name_buffer_pointer = 0;
                strcpy(state->definitions[state->definition_count].name, state->name_buffer);
                state->mode = MODE_DEFINE_BODY;
                break;
            }

            state->name_buffer[state->name_buffer_pointer++] = c;
            break;
        case MODE_DEFINE_BODY:
            if (c == '\n' || c == '\r')
            {
                state->name_buffer[state->name_buffer_pointer] = '\0';
                state->name_buffer_pointer = 0;
                strcpy(state->definitions[state->definition_count].value, state->name_buffer);
                state->definition_count += 1;
                state->mode = MODE_START_OF_LINE;
                break;
            }
            state->name_buffer[state->name_buffer_pointer++] = c;
            break;
        case MODE_CODE_NAME:
            if (c == '\n' || c == '\r')
                state->mode = MODE_START_OF_LINE;
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
        case MODE_INCLUDE:
            if (isspace(c))
                break;
            if (c == '"')
                state->mode = MODE_INCLUDE_LOCAL;
            else if (c == '<')
                state->mode = MODE_INCLUDE_GLOBAL;
            else
                assert (0);
            break;
        case MODE_INCLUDE_LOCAL:
            if (c == '"')
            {
                state->name_buffer[state->name_buffer_pointer] = '\0';
                state->name_buffer_pointer = 0;
                include_file(state->name_buffer, out_buffer);
                state->mode = MODE_CODE_NAME;
                break;
            }
            state->name_buffer[state->name_buffer_pointer++] = c;
            break;
        case MODE_INCLUDE_GLOBAL:
            if (c == '>')
            {
                state->name_buffer[state->name_buffer_pointer] = '\0';
                state->name_buffer_pointer = 0;
                include_file(state->name_buffer, out_buffer);
                state->mode = MODE_CODE_NAME;
                break;
            }
            state->name_buffer[state->name_buffer_pointer++] = c;
            break;
    }

}

const char *pre_proccess_file(const char *file_path, int *out_length)
{
    FILE *file = fopen(file_path, "r");
    OutBuffer out_buffer = make_buffer();

    char buffer[80];
    State state;
    state.mode = MODE_START_OF_LINE;
    state.definition_count = 0;
    for (;;)
    {
        int nread = fread(buffer, sizeof(char), sizeof(buffer), file);
        if (nread == 0)
            break;

        for (int i = 0; i < nread; i++)
        {
            char c = buffer[i];
            proccess_char(c, &out_buffer, &state);
        }
    }

    fclose(file);
    *out_length = out_buffer.size;
    return out_buffer.data;
}
