#ifndef SOURCE_MAP_H
#define SOURCE_MAP_H

typedef struct SourceMapEntry
{
    char file_name[80];
    int from_line;
    int to_line;

    struct SourceMapEntry *parent;
    struct SourceMapEntry *children;
    int child_count;
} SourceMapEntry;

typedef struct SourceMap
{
    SourceMapEntry *root;
    SourceMapEntry *current;
} SourceMap;

typedef struct SourceLine
{
    const char *file_name;
    int source_index;
    int line_no;
    int column_no;
} SourceLine;

SourceMap source_map_new();
void free_source_map(SourceMap *map);

SourceMapEntry *source_map_create_entry(SourceMap *map, const char *file_name);
void source_map_entry_finish(SourceMap *map);
SourceLine source_map_entry_for_line(SourceMap *map, int source_index, int line_no, int column_no);

#endif // SOURCE_MAP_H
