#include "source_map.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

SourceMap source_map_new()
{
    SourceMapEntry *root = malloc(sizeof(SourceMapEntry));
    strcpy(root->file_name, "main");
    root->child_count = 0;
    root->children = NULL;
    root->parent = NULL;
    root->from_line = 0;
    root->to_line = -1;

    SourceMap map;
    map.root = root;
    map.current = root;
    return map;
}

static SourceMapEntry *new_child(SourceMapEntry *parent)
{
    if (!parent->children)
        parent->children = malloc(sizeof(SourceMapEntry));
    else
        parent->children = realloc(parent->children, sizeof(SourceMapEntry) * (parent->child_count + 1));
    return &parent->children[parent->child_count++];
}

SourceMapEntry *source_map_create_entry(SourceMap *map, const char *file_name)
{
    SourceMapEntry *entry = new_child(map->current);
    strcpy(entry->file_name, file_name);
    entry->child_count = 0;
    entry->children = NULL;
    entry->from_line = -1;
    entry->to_line = -1;

    entry->parent = map->current;
    map->current = entry;
    return entry;
}

void source_map_entry_finish(SourceMap *map)
{
    assert(map->current);
    map->current = map->current->parent;
}

static SourceLine search_entry(SourceMapEntry *entry, int line_no)
{
    if (entry->to_line != -1 && (line_no < entry->from_line || line_no > entry->to_line))
        return (SourceLine) { NULL, -1, -1, -1 };

    int lines_inserted_above = 0;
    for (int i = 0; i < entry->child_count; i++)
    {
        SourceMapEntry *child = &entry->children[i];
        SourceLine line = search_entry(child, line_no);
        if (line.file_name != NULL)
            return line;

        if (line_no > child->to_line)
            lines_inserted_above += child->to_line - child->from_line + 1;
    }

    SourceLine line;
    line.file_name = entry->file_name;
    line.line_no = line_no - entry->from_line - lines_inserted_above + 1;
    return line;
}

SourceLine source_map_entry_for_line(SourceMap *map, int source_index, int line_no, int column_no)
{
    SourceLine line = search_entry(map->root, line_no);
    line.source_index = source_index;
    line.column_no = column_no;
    return line;
}

static void free_map_entry(SourceMapEntry *entry)
{
    if (!entry->children)
        return;

    for (int i = 0; i < entry->child_count; i++)
        free_map_entry(&entry->children[i]);
    free(entry->children);
}

void free_source_map(SourceMap *map)
{
    free_map_entry(map->root);
    free(map->root);
    map->root = NULL;
    map->current = NULL;
}
