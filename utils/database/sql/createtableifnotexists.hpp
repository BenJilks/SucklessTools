#pragma once
#include "createtable.hpp"

namespace DB::Sql
{

    class CreateTableIfNotExistsStatement final : public CreateTableStatement
    {
        friend Parser;

    public:
        virtual SqlResult execute(DataBase&) const override;

    private:
        CreateTableIfNotExistsStatement()
            : CreateTableStatement(CreateTableIfNotExists) {}

    };

}
