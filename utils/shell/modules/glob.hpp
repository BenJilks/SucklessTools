#pragma once

#include "module.hpp"

class GlobModule : public Module
{
public:
    GlobModule() = default;

    virtual bool hook_on_token(Lexer&, Token&, int index) override;

private:
    bool is_glob(const std::string &str);
    bool does_glob_match(const std::string &glob, const std::string &option);
    std::vector<std::string> resolve_glob(const std::vector<std::string> &posible_paths, const std::string &glob);

};
