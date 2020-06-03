#include "webinterface.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
using namespace Web;

//#define DEBUG_REQUESTS

void Interface::route(const std::string &path,
                      std::function<void(const Url&, Response&)> callback)
{
    m_routes.push_back({ path, callback });
}

void Interface::start()
{
    auto sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
       perror("socket()");
       return;
    }

    int enable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR)");
        return;
    }

    struct sockaddr_in server_addr, client_addr;
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(m_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind()");
        return;
    }

    for (;;)
    {
        listen(sock, 5);

        socklen_t client_count = sizeof(client_addr);
        auto client_sock = accept(sock, (struct sockaddr*)&client_addr, &client_count);
        if (client_sock < 0)
        {
            perror("ERROR on accept");
            return;
        }

        // TODO: This can be made much more better by not allocating a new buffer each time
        std::vector<char> buffer(255);
        for (int i = 0; ; i++)
        {
            auto len = read(client_sock, buffer.data() + i * 255, 255);
            if (len < 0)
            {
                perror("read()");
                return;
            }

            if (len < 255)
                break;

            buffer.resize(255 * (i + 1));
        }

        auto request = Request::parse(std::string(buffer.data(), buffer.size()));
        if (!request)
        {
            // TODO: Error, invalid request
            close(client_sock);
            continue;
        }

#ifdef DEBUG_REQUESTS
        std::cout << "Got request: " << request->url().path() << "\n";
#endif

        const Route *route = nullptr;
        for (const auto &it : m_routes)
        {
            if (!std::strcmp(it.path.c_str(), request->url().path().c_str()))
            {
                route = &it;
                break;
            }
        }

        Response response(std::move(*request));
        if (!route)
        {
            response.code(404);
            response.send_text("Error 404: page not found");
        }
        else
        {
            response.code(200);
            route->callback(request->url(), response);
        }

        response.send(client_sock);
        close(client_sock);
    }
}
