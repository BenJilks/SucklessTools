#include "preproccessor.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

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
    MODE_CODE_NAME,
};

typedef struct Define
{
    char name[80];
    char value[80];
} Define;

static enum Mode g_mode;
static char g_name_buffer[80];
static int g_name_buffer_pointer;

static Define g_definitions[80];
static int g_definition_count;

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

static void check_define(char *name, OutBuffer *out_buffer)
{
    for (int i = 0; i < g_definition_count; i++)
    {
        if (!strcmp(name, g_definitions[i].name))
        {
            append_string(out_buffer, g_definitions[i].value);
            return;
        }
    }

    append_string(out_buffer, name);
}

static void proccess_char(char c, OutBuffer *out_buffer)
{
    switch (g_mode)
    {
        case MODE_START_OF_LINE:
            if (c == '#')
            {
                g_mode = MODE_STATEMENT_NAME;
                g_name_buffer_pointer = 0;
                break;
            }
            if (!isspace(c))
            {
                if (isalnum(c))
                    g_name_buffer[g_name_buffer_pointer++] = c;
                else
                    append_buffer(out_buffer, c);
                g_mode = MODE_CODE_NAME;
                break;
            }
            append_buffer(out_buffer, c);
            break;
        case MODE_STATEMENT_NAME:
            if (c == '\n' || c == '\r')
                assert (0);
            if (isspace(c))
            {
                g_name_buffer[g_name_buffer_pointer] = '\0';
                g_name_buffer_pointer = 0;
                if (!strcmp(g_name_buffer, "define"))
                    g_mode = MODE_DEFINE_NAME;
                else
                    assert (0);
                break;
            }

            g_name_buffer[g_name_buffer_pointer++] = c;
            break;
        case MODE_DEFINE_NAME:
            if (c == '\n' || c == '\r')
            {
                g_name_buffer[g_name_buffer_pointer] = '\0';
                g_name_buffer_pointer = 0;
                strcpy(g_definitions[g_definition_count].name, g_name_buffer);
                g_definitions[g_definition_count].value[0] = '\0';
                g_definition_count += 1;
                g_mode = MODE_START_OF_LINE;
                break;
            }
            if (isspace(c))
            {
                g_name_buffer[g_name_buffer_pointer] = '\0';
                g_name_buffer_pointer = 0;
                strcpy(g_definitions[g_definition_count].name, g_name_buffer);
                g_mode = MODE_DEFINE_BODY;
                break;
            }

            g_name_buffer[g_name_buffer_pointer++] = c;
            break;
        case MODE_DEFINE_BODY:
            if (c == '\n' || c == '\r')
            {
                g_name_buffer[g_name_buffer_pointer] = '\0';
                g_name_buffer_pointer = 0;
                strcpy(g_definitions[g_definition_count].value, g_name_buffer);
                g_definition_count += 1;
                g_mode = MODE_START_OF_LINE;
                break;
            }

            g_name_buffer[g_name_buffer_pointer++] = c;
            break;
        case MODE_CODE_NAME:
            if (c == '\n' || c == '\r')
                g_mode = MODE_START_OF_LINE;
            if (!isalnum(c))
            {
                g_name_buffer[g_name_buffer_pointer] = '\0';
                g_name_buffer_pointer = 0;
                check_define(g_name_buffer, out_buffer);
                append_buffer(out_buffer, c);
            }
            else
            {
                g_name_buffer[g_name_buffer_pointer++] = c;
            }
            break;
    }

}

const char *pre_proccess_file(const char *file_path, int *out_length)
{
    FILE *file = fopen(file_path, "r");
    OutBuffer out_buffer = make_buffer();

    char buffer[80];
    g_mode = MODE_START_OF_LINE;
    g_definition_count = 0;
    for (;;)
    {
        int nread = fread(buffer, sizeof(char), sizeof(buffer), file);
        if (nread == 0)
            break;

        for (int i = 0; i < nread; i++)
        {
            char c = buffer[i];
            proccess_char(c, &out_buffer);
        }
    }

    *out_length = out_buffer.size;
    return out_buffer.data;
}
