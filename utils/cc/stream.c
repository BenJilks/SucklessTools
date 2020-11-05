#include "stream.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define STREAM_BUFFER_SIZE 1024

Stream stream_create_input_file(const char *file_name)
{
    Stream stream;
    stream.file = fopen(file_name, "r");
    stream.type = STREAM_TYPE_FILE;
    stream.mode = STREAM_MODE_INPUT;
    stream.peek = 0;
    stream.should_reconsume = 0;
    return stream;
}

Stream stream_create_input_memory(char *memory, int memory_length)
{
    Stream stream;
    stream.memory = memory;
    stream.memory_length = memory_length;
    stream.memory_pointer = 0;
    stream.type = STREAM_TYPE_MEMORY;
    stream.mode = STREAM_MODE_INPUT;
    stream.peek = 0;
    stream.should_reconsume = 0;
    return stream;
}

Stream stream_create_output_memory()
{
    Stream stream;
    stream.memory = malloc(STREAM_BUFFER_SIZE);
    stream.memory_pointer = STREAM_BUFFER_SIZE;
    stream.memory_length = 0;
    stream.type = STREAM_TYPE_MEMORY;
    stream.mode = STREAM_MODE_OUTPUT;
    return stream;
}

void stream_memory_output_to_input(Stream *stream)
{
    stream->mode = STREAM_MODE_INPUT;
    stream->memory_pointer = 0;
    stream->peek = 0;
    stream->should_reconsume = 0;
}

void stream_read_next_char(Stream *stream)
{
    assert (stream->mode == STREAM_MODE_INPUT);
    if (stream->should_reconsume)
    {
        stream->should_reconsume = 0;
        return;
    }

    switch (stream->type)
    {
        case STREAM_TYPE_MEMORY:
            if (stream->memory_pointer >= stream->memory_length)
                stream->peek = EOF;
            else
                stream->peek = stream->memory[stream->memory_pointer++];
            return;
        case STREAM_TYPE_FILE:
            stream->peek = fgetc(stream->file);
            return;
    }
    assert (0);
}

static void ensure_size(Stream *stream, int size)
{
    while (stream->memory_length + size >= stream->memory_pointer)
    {
        stream->memory_pointer += STREAM_BUFFER_SIZE;
        stream->memory = realloc(stream->memory, stream->memory_pointer);
    }
}

void stream_write_char(Stream *stream, char c)
{
    assert (stream->mode == STREAM_MODE_OUTPUT);
    assert (stream->type == STREAM_TYPE_MEMORY);
    ensure_size(stream, 1);
    stream->memory[stream->memory_length++] = c;
}

void stream_write_string(Stream *stream, const char *str)
{
    assert (stream->mode == STREAM_MODE_OUTPUT);
    assert (stream->type == STREAM_TYPE_MEMORY);
    int str_len = strlen(str);

    ensure_size(stream, str_len);
    strcpy(stream->memory + stream->memory_length, str);
    stream->memory_length += str_len;
}

void stream_close(Stream *stream)
{
    switch (stream->type)
    {
        case STREAM_TYPE_FILE:
            fclose(stream->file);
            break;
        case STREAM_TYPE_MEMORY:
            free(stream->memory);
            break;
    }
}
