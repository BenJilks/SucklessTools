#ifndef BUFFER_H
#define BUFFER_H

#define BUFFER_SIZE 80

#define BUFFER_DECLARE(name, type) \
    type *name##s; \
    int name##_count; \
    int name##_buffer

#define BUFFER_INIT(name, type) \
    name##_count = 0; \
    name##_buffer = BUFFER_SIZE; \
    name##s = malloc(sizeof(type) * name##_buffer)

#define BUFFER_ADD(name, type, value) \
    if (name##_count >= name##_buffer) \
    { \
        name##_buffer += BUFFER_SIZE; \
        name##s = realloc(name##s, sizeof(type) * name##_buffer); \
    } \
     \
    name##s[name##_count] = *value; \
    name##_count += 1

#endif // BUFFER_H

