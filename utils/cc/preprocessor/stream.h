#ifndef STREAM_H
#define STREAM_H

#include <stdio.h>

enum StreamType
{
    STREAM_TYPE_FILE,
    STREAM_TYPE_MEMORY,
};

enum StreamMode
{
    STREAM_MODE_INPUT,
    STREAM_MODE_OUTPUT,
};

typedef struct Stream
{
    // File
    FILE *file;

    // Memory
    char *memory;
    int memory_pointer, memory_length;

    // General
    enum StreamType type;
    enum StreamMode mode;
    char peek;
    int should_reconsume;
    int line_no;
} Stream;

Stream stream_create_input_file(const char *file_name);
Stream stream_create_input_memory(char *memory, int memory_length);
Stream stream_create_output_memory();
void stream_memory_output_to_input(Stream *stream);
void stream_read_next_char(Stream *stream);
void stream_write_char(Stream *stream, char c);
void stream_write_string(Stream *stream, const char *str);
void stream_close(Stream *stream);

#endif // STREAM_H
