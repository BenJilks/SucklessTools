#pragma once

namespace DB
{

    class DataBase;
    class Chunk;
    class DynamicData;
    class Table;
    class Column;
    class Row;
    class Entry;

    namespace Sql
    {

        class Lexer;
        class Parser;
        class Statement;
        class SelectStatement;
        class InsertStatement;
        class CreateTableStatement;
        class CreateTableIfNotExistsStatement;
        class UpdateStatement;
        class DeleteStatement;
        class Value;
        class ValueNode;

    };

}
