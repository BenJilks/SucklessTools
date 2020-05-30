#pragma once
#include "request.hpp"
#include <string>
#include <functional>
#include <vector>

namespace Web
{

    class Interface
    {
    public:
        Interface() {}

        void route(const std::string &path,
                   std::function<void(const Url&, Response&)> callback);

        void start();

    private:
        struct Route
        {
            std::string path;
            std::function<void(const Url&, Response&)> callback;
        };

        int m_port { 8080 };
        std::vector<Route> m_routes;
    };

}
