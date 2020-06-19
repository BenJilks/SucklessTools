#pragma once

namespace DB
{

    class DataBase;
    class Chunk;
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
        class Value;

    };

}
