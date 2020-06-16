#include "cron.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <libjson/libjson.hpp>
#include <mutex>
#include <pwd.h>
#include <sstream>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <webinterface/table.hpp>
#include <webinterface/template.hpp>
#include <webinterface/webinterface.hpp>
namespace fs = std::filesystem;

static auto g_web_dir = std::string("/usr/local/share/crond/web");

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

static std::optional<Json::Document> load_log(const std::string &date, const std::string &cron_path)
{
    auto now = Timer::make_time_stamp();
    if (date == now)
        return std::nullopt;

    auto doc = Json::Document::parse(std::ifstream(cron_path + "/log/log-" + date + ".json"));
    return std::move(doc);
}

static void web_thread(Json::Value *log, std::mutex *log_mutex, const std::string cron_path, int port)
{
    Web::Interface interface(port);
    auto log_template_or_error = Web::Template::load_from_file(g_web_dir + "/log.html");
    auto log_entry_template_or_error = Web::Template::load_from_file(g_web_dir + "/log_entry.html");
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
        std::ifstream file_stream(g_web_dir + "/style.css");
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
        auto log_doc = load_log(date, cron_path);
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

static std::string insure_cron_path()
{
    auto *pw = getpwuid(getuid());
    auto home_dir = pw->pw_dir;

    auto cron_path = std::string(home_dir) + "/.crons";
    if (!fs::exists(cron_path))
        fs::create_directory(cron_path);

    auto log_path = cron_path + "/log";
    if (!fs::exists(log_path))
        fs::create_directory(log_path);

    return cron_path;
}

static void daemonize()
{
    auto pid = fork();
    if (pid < 0)
    {
        perror("fork()");
        exit(EXIT_FAILURE);
    }

    // Terminate the parent
    if (pid > 0)
        exit(EXIT_SUCCESS);

    // Make the child the session leader
    if (setsid() < 0)
    {
        perror("setsid()");
        exit(EXIT_FAILURE);
    }

    // Start second fork
    pid = fork();
    if (pid < 0)
    {
        perror("fork()");
        exit(EXIT_FAILURE);
    }

    // Terminate the first parent
    if (pid > 0)
        exit(EXIT_SUCCESS);

    // Make root the working directory
    chdir("/");

    // New file permissions
    umask(0);

    // Close all open file descriptors
//    for (auto fd = sysconf(_SC_OPEN_MAX); fd > 0; --fd)
//        close(fd);

    // TODO: Make lock
}

static void start_cron(const std::string &cron_path, bool enable_web_interface, int port)
{
    auto log_path = cron_path + "/log/log-" + Timer::make_time_stamp() + ".json";
    std::vector<Cron> crons;
    size_t run_time = 0;
    std::mutex log_mutex;

    auto log_doc = Json::Document::parse(std::ifstream(log_path));
    auto &log = log_doc.root_or_new_array();

    auto save_log = [&log, &log_path, &log_mutex]()
    {
        std::cout << "Saving log file\n";
        log_mutex.lock();
        std::ofstream out(log_path);
        out << log.to_string(Json::PrintOption::PrettyPrint);
        out.close();
        log_mutex.unlock();
    };

    std::cout << "Loading crons\n";
    load_crons(crons, cron_path);

    std::thread web;
    if (enable_web_interface)
        web = std::thread(web_thread, &log, &log_mutex, cron_path, port);

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
}

static struct option cmd_options[] =
{
    { "help",       no_argument,        0, 'h' },
    { "daemon",     no_argument,        0, 'd' },
    { "web",        no_argument,        0, 'w' },
    { "port",       required_argument,  0, 'p' },
    { "output",     no_argument,        0, 'o' },
};

void show_help()
{
    std::cout << "usage: crond [-d] [-w] [-p port]\n";
    std::cout << "\nRun cron jobs\n";
    std::cout << "\noptional arguments:\n";
    std::cout << "  -h, --help\t\tShow this help message and exit\n";
    std::cout << "  -d, --daemon\t\tStart as a daemon\n";
    std::cout << "  -w, --web\t\tEnable the web interface\n";
    std::cout << "  -p, --port\t\tSpecify what port you want the web interface to run on\n";
    std::cout << "  -o, --output\t\tShow the output log for today\n";
}

int main(int argc, char *argv[])
{
    bool should_daemonize = false;
    bool enable_web_interface = false;
    int port = 8080;
    srand(time(NULL));

    for (;;)
    {
        int option_index;
        int c = getopt_long(argc, argv, "hdwp:o",
            cmd_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case 'h':
                show_help();
                return 0;

            case 'd':
                should_daemonize = true;
                break;

            case 'w':
                enable_web_interface = true;
                break;

            case 'p':
                port = std::atoi(optarg);
                if (port < 0)
                {
                    std::cerr << "Error: Invalid port " << optarg << ", must be positive\n";
                    return EXIT_FAILURE;
                }

                if (getuid() != 0 && (port == 80 || port == 443))
                {
                    std::cerr << "Error: Must be root to access port " << optarg << "\n";
                    return EXIT_FAILURE;
                }
                break;

            case 'o':
                std::system(("cat \"" + insure_cron_path() + "/log/log-" +
                    Timer::make_time_stamp() + ".json\" | less").c_str());
                return EXIT_SUCCESS;
        }
    }

    auto cron_path = insure_cron_path();
    std::ofstream log_file;
    if (should_daemonize)
    {
        daemonize();
        log_file.open("/tmp/crond_log");
        log_file.rdbuf()->pubsetbuf(0, 0);
        std::cout.rdbuf(log_file.rdbuf());
        std::cerr.rdbuf(log_file.rdbuf());
        std::clog.rdbuf(log_file.rdbuf());
    }

    std::cout << "Started\n";
    start_cron(cron_path, enable_web_interface, port);
    return EXIT_SUCCESS;
}
