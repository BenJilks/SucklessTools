#include "cron.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <libjson/libjson.hpp>
#include <webinterface/webinterface.hpp>
#include <webinterface/template.hpp>
#include <webinterface/table.hpp>
#include <thread>
#include <mutex>
namespace fs = std::filesystem;

static const auto g_cron_path = std::string("crons");

static void load_crons(std::vector<Cron> &crons, const std::string &path)
{
    crons.clear();

    for (const auto &entry : fs::directory_iterator(path))
    {
        if (!entry.is_directory())
            continue;

        if (entry.path().filename() == "log")
            continue;

        std::cout << "Loading cron: " << entry.path() << "... ";
        std::cout.flush();
        auto cron = Cron::load(entry.path(), std::cout);
        if (!cron)
            continue;

        crons.push_back(std::move(*cron));
        std::cout << "[Done]\n";
    }
}

static std::string replace_all(std::string str, const std::string& from, const std::string& to)
{
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }

    return str;
}

static std::optional<Json::Document> load_log(const std::string &date)
{
    auto now = Timer::make_time_stamp();
    if (date == now)
        return std::nullopt;

    auto doc = Json::Document::parse(std::ifstream(g_cron_path + "/log/log-" + date + ".json"));
    return std::move(doc);
}

static void web_thread(Json::Value *log, std::mutex *log_mutex)
{
    Web::Interface interface;
    auto log_template_or_error = Web::Template::load_from_file("web/log.html");
    auto log_entry_template_or_error = Web::Template::load_from_file("web/log_entry.html");
    if (!log_template_or_error || !log_entry_template_or_error)
    {
        // TODO: Error, unable to load template
        assert (false);
        return;
    }
    auto &log_template = *log_template_or_error;
    auto &log_entry_template = *log_entry_template_or_error;

    interface.route("/style.css", [&](const auto&, auto &response)
    {
        std::ifstream file_stream("web/style.css");
        std::stringstream stream;
        stream << file_stream.rdbuf();

        response.send_text(stream.str());
        response.content_type("text/css");
    });

    interface.route("/", [&](const auto &url, auto &response)
    {
        log_mutex->lock();
        auto date = url.query("date");
        if (date.empty())
            date = Timer::make_time_stamp();

        auto this_log = log;
        auto log_doc = load_log(date);
        if (log_doc)
            this_log = &log_doc->root();

        auto history_or_error = Web::Table::make("Start Time", "Name", "Runtime", "Exit Status");
        auto &history = *history_or_error;
        for (const auto &entry : this_log->to_array())
        {
            auto &row = history.add_row();
            auto id = (*entry)["id"].to_string();
            auto start_time = (*entry)["start_time"].to_string();
            auto link_start = "<a id=\"" + start_time + "\" href=\"/log_entry?date=" + date + ";id=" + id + "\">";

            row["Start Time"] = link_start + start_time + "</a>";
            row["Name"] = (*entry)["name"].to_string();
            row["Runtime"] = (*entry)["runtime"].to_string();
            row["Exit Status"] = (*entry)["exit_status"].to_string();
        }
        history.sort_by("Start Time", Web::Table::SortMode::Descending);

        log_template["yesterday"] = Timer::make_time_stamp_yeserday(date);
        log_template["tomorrow"] = Timer::make_time_stamp_tomorrow(date);
        log_template["date"] = date;
        log_template["history"] = history.build();

        response.send_text(log_template.build());
        log_mutex->unlock();
    });

    interface.route("/log_entry", [&](const auto& url, auto &response)
    {
        log_mutex->lock();
        const auto &id = url.query("id");
        const auto now = Timer::make_time_stamp();
        const Json::Value *entry_or_error = nullptr;

        for (const auto &it : log->to_array())
        {
            if ((*it)["id"].to_string() == id)
            {
                entry_or_error = it;
                break;
            }
        }

        if (!entry_or_error)
        {
            response.send_text("No log with id " + id);
            return;
        }
        const auto &entry = *entry_or_error;
        auto output = entry["output"].to_string();
        output = replace_all(output, "\n", "<br>");

        log_entry_template["id"] = id;
        log_entry_template["date"] = now;
        log_entry_template["output"] = output;
        log_entry_template["name"] = entry["name"].to_string();
        log_entry_template["time"] = entry["start_time"].to_string();
        log_entry_template["exit_status"] = entry["exit_status"].to_string();
        log_entry_template["runtime"] = entry["runtime"].to_string();
        response.send_text(log_entry_template.build());
        log_mutex->unlock();
    });

    interface.start();
}

int main()
{
    auto log_path = g_cron_path + "/log/log-" + Timer::make_time_stamp() + ".json";
    std::vector<Cron> crons;
    size_t run_time = 0;
    std::mutex log_mutex;

    auto log_doc = Json::Document::parse(std::ifstream(log_path));
    auto &log = log_doc.root_or_new<Json::Array>();

    auto save_log = [&log, &log_path, &log_mutex]()
    {
        log_mutex.lock();
        std::ofstream out(log_path);
        out << log.to_string(Json::PrintOption::PrettyPrint);
        out.close();
        log_mutex.unlock();
    };

    load_crons(crons, g_cron_path);
    std::thread web(web_thread, &log, &log_mutex);

    for (;;)
    {
        for (auto &cron : crons)
        {
            if (cron.should_run_and_mark_done(run_time))
            {
                cron.run(log, log_mutex);
                save_log();
            }
        }

        run_time += 1;
        std::this_thread::sleep_for(std::chrono::minutes(1));
    }

    return 0;
}
