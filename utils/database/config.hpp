#pragma once

// Debugging flags
// #define DEBUG_CHUNKS
// #define DEBUG_TABLE_LOAD

namespace DB::Config
{

    static int constexpr major_version = 1;
    static int constexpr minor_version = 0;

    static int constexpr chunk_header_size = 20;
    static int constexpr row_header_size = 4;

}
