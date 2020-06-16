#pragma once
#include "forward.hpp"
#include <string>

//#define DEBUG_ALLOCATOR

namespace Json
{

    class Allocator
    {
        friend Value;
        friend Object;
        friend Array;
        friend Document;

    public:
        Allocator() {}
        virtual ~Allocator() {}

        virtual void report_usage() = 0;
        virtual void did_delete() = 0;

        virtual Null *make_null() = 0;
        virtual Object *make_object() = 0;
        virtual Array *make_array() = 0;
        virtual String *make_string(const std::string_view) = 0;
        virtual Number *make_number(double) = 0;
        virtual Boolean *make_boolean(bool) = 0;
    };

}
