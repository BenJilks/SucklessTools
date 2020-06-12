#include "request.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unistd.h>
#include <cassert>
using namespace Web;

//#define DEBUG_REQUEST

static std::string parse_escapes(const std::string &str)
{
    std::string out;

    enum class State
    {
        Normal,
        EscapeStart,
        EscapeSecond
    };

    auto state = State::Normal;
    for (char c : str)
    {
        switch (state)
        {
            case State::Normal:
                if (c == '%')
                {
                    state = State::EscapeStart;
                    break;
                }

                out += c;
                break;

            case State::EscapeStart:
                if (c != '2')
                {
                    out += '%';
                    out += c;
                    state = State::Normal;
                    break;
                }

                state = State::EscapeSecond;
                break;

            case State::EscapeSecond:
                if (c != '0')
                {
                    out += '%';
                    out += '2';
                    out += c;
                    state = State::Normal;
                    break;
                }

                out += ' ';
                state = State::Normal;
                break;
        }
    }

    return out;
}

std::optional<Url> Url::parse(const std::string &raw_url)
{
    Url url;

    enum class State
    {
        Path,
        QueryName,
        QueryValue,
        Done
    };

    auto state = State::Path;
    size_t index = 0;
    std::string buffer;
    std::string path_buffer;
    std::string curr_query_name;

    for (;;)
    {
        char c = 0;
        if (index < raw_url.size())
            c = raw_url[index];
        index += 1;

        if (state == State::Done)
            break;

        switch (state)
        {
            default:
                // TODO: Error, invalid state
                assert (false);
                return std::nullopt;

            case State::Path:
                if (c == '?')
                {
                    url.m_path_list.push_back(buffer);
                    buffer.clear();
                    state = State::QueryName;
                    break;
                }

                if (!c)
                {
                    url.m_path_list.push_back(buffer);
                    buffer.clear();
                    state = State::Done;
                    break;
                }

                path_buffer += c;
                if (c == '/')
                {
                    url.m_path_list.push_back(buffer);
                    buffer.clear();
                    break;
                }

                buffer += c;
                break;

            case State::QueryName:
                if (c == '=')
                {
                    curr_query_name = std::move(buffer);
                    buffer.clear();

                    state = State::QueryValue;
                    break;
                }

                if (!c)
                {
                    state = State::Done;
                    break;
                }

                buffer += c;
                break;

            case State::QueryValue:
                if (!c || c == ';')
                {
                    url.m_queries[curr_query_name] = parse_escapes(std::move(buffer));
                    curr_query_name.clear();
                    buffer.clear();

                    state = !c ? State::Done : State::QueryName;
                    break;
                }

                buffer += c;
                break;
        }
    }

    url.m_path = std::move(path_buffer);
    return std::move(url);
}

std::string Url::query(const std::string &name) const
{
    if (m_queries.find(name) == m_queries.end())
        return {};

    return m_queries.at(name);
}

std::optional<Request> Request::parse(const std::string &raw_request)
{
    Request request;

#define ENUMERATE_STATES \
    __ENUMERATE_STATE(Method) \
    __ENUMERATE_STATE(Url) \
    __ENUMERATE_STATE(Protocal) \
    __ENUMERATE_STATE(HeaderName) \
    __ENUMERATE_STATE(HeaderContentStart) \
    __ENUMERATE_STATE(HeaderContent) \
    __ENUMERATE_STATE(BodyStart) \
    __ENUMERATE_STATE(Body) \
    __ENUMERATE_STATE(Done)

    enum class State
    {
#define __ENUMERATE_STATE(x) x,
        ENUMERATE_STATES
#undef __ENUMERATE_STATE
    };

    auto string_name = [](State state) -> std::string
    {
        switch (state)
        {
#define __ENUMERATE_STATE(x) case State::x: return #x;
            ENUMERATE_STATES
#undef __ENUMERATE_STATE
        }
    };
#undef ENUMERATE_STATES

    auto state = State::Method;
    size_t index = 0;
    std::string buffer;
    std::string curr_header_name;

    auto crash = [&]()
    {
        std::cout << "Crash in state " << string_name(state) << "\n";
        std::cout << "Raw request: \n" << raw_request << "\n";
        assert (false);
    };

    for (;;)
    {
        char c = 0;
        if (index < raw_request.size())
            c = raw_request[index];

#ifdef DEBUG_REQUEST
        std::cout << "State: " << string_name(state) << "\n";
#endif

        if (state == State::Done)
            break;

        if (index > raw_request.size())
        {
            // TODO: Error, unexpected end
            crash();
            return std::nullopt;
        }
        index += 1;

        switch (state)
        {
            default:
                // TODO: Error, invalid state
                crash();
                return std::nullopt;

            case State::Method:
                if (c == ' ')
                {
                    request.m_method = std::move(buffer);
                    buffer.clear();
                    state = State::Url;
                    break;
                }

                if (!c)
                {
                    // NOTE: This is a blank request
                    return std::nullopt;
                }

                buffer += c;
                break;

            case State::Url:
                if (c == ' ')
                {
                    state = State::Protocal;
                    auto url = Url::parse(std::move(buffer));
                    buffer.clear();

                    if (!url)
                    {
                        // TODO: Error, Invalid URL
                        assert (false);
                        break;
                    }

                    request.m_url = *url;
                    break;
                }

                buffer += c;
                break;

            case State::Protocal:
                if (c == '\n')
                {
                    state = State::HeaderName;
                    break;
                }

                // NOTE: We don't care about this for now
                break;

            case State::HeaderName:
                if (c == ':')
                {
                    curr_header_name = std::move(buffer);
                    buffer.clear();
                    state = State::HeaderContentStart;
                    break;
                }

                if (c == '\r')
                {
                    state = State::BodyStart;
                    break;
                }

                if (!c)
                {
                    state = State::Done;
                    break;
                }

                buffer += c;
                break;

            case State::HeaderContentStart:
                if (c == ' ')
                    break;

                if (!c)
                {
                    state = State::Done;
                    break;
                }

                state = State::HeaderContent;
                break;

            case State::HeaderContent:
                if (c == '\n')
                {
                    if (curr_header_name == "User-Agent")
                        request.m_user_agient = std::move(buffer);

                    buffer.clear();
                    curr_header_name.clear();
                    state = State::HeaderName;
                    break;
                }

                if (!c)
                {
                    state = State::Done;
                    break;
                }

                buffer += c;
                break;

            case State::BodyStart:
                if (c == '\n')
                {
                    buffer.clear();
                    state = State::Body;
                    break;
                }

                // TODO: Error, invalid body
                crash();
                return std::nullopt;

            case State::Body:
                if (!c)
                {
                    request.m_body = std::move(buffer);
                    state = State::Done;
                    break;
                }

                buffer += c;
                break;
        }
    }

    return std::move(request);
}

void Response::send_text(const std::string &str)
{
    m_data += str;
}

void Response::send(int client_sock)
{
    std::string response =
        "HTTP/1.1 " + std::to_string(m_response_code) + " OK\n"
        "Accept-Ranges: bytes\n"
        "Content-Length: " + std::to_string(m_data.size()) + "\n" +
        "Content-Type: " + m_content_type + "\n\r\n" +
        m_data;

    write(client_sock, response.data(), response.size());
}
