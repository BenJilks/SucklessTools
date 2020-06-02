#pragma once
#include <string>
#include <optional>
#include <vector>
#include <map>

namespace Web
{
    class Url
    {
    public:
        Url() {}

        static std::optional<Url> parse(const std::string &raw_url);

        std::string query(const std::string &name) const;
        inline const std::string &path() const { return m_path; }

    private:
        std::vector<std::string> m_path_list;
        std::map<std::string, std::string> m_queries;
        std::string m_path;

    };

    class Request
    {
    public:
        static std::optional<Request> parse(const std::string &raw_request);

        inline const Url &url() const { return m_url; }

    private:
        Request() {}

        std::string m_method;
        std::string m_user_agient;
        std::string m_body;
        Url m_url;

    };

    class Response : public Request
    {
    public:
        Response() = delete;
        Response(Request &&request)
            : Request(request)
            , m_content_type("text/html") {}

        void send_text(const std::string &str);
        void send(int client_sock);
        void code(int code) { m_response_code = code; }
        void content_type(const std::string &type) { m_content_type = type; }

    private:
        std::string m_data;
        std::string m_content_type;
        int m_response_code { 200 };

    };

}
