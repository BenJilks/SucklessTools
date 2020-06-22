#include "cron.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <getopt.h>
#include <iostream>
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
#include <database/database.hpp>
#include <string>
namespace fs = std::filesystem;

static auto g_web_dir = std::string("/usr/local/share/crond/web");

static void load_crons(std::vector<Cron> &crons, DB::DataBase &log, const std::string &path)
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
        auto cron = Cron::load(entry.path(), log, std::cout);
        if (!cron)
            continue;

        crons.push_back(std::move(*cron));
        std::cout << "[Done]\n";
    }
}

static void web_thread(DB::DataBase *log, std::mutex *log_mutex, const std::string cron_path, int port)
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

        auto history_or_error = Web::Table::make("Start Time", "Name", "Runtime", "Exit Status");
        auto &history = *history_or_error;

        auto result = log->execute_sql("SELECT * FROM Log WHERE Date = '" + date + "'");
        if (!result.good())
        {
            result.output_errors();
            response.send_text("Error");
            return;
        }

        for (const auto &entry_row : result)
        {
            auto &row = history.add_row();
            auto id = entry_row["ID"]->as_string();
            auto start_time = entry_row["Time"]->as_string();
            auto link_start = "<a id=\"" + start_time
                + "\" href=\"/log_entry?date=" + date
                + ";id=" + id + "\">";

            row["Start Time"] = link_start + start_time + "</a>";
            row["Name"] = entry_row["Name"]->as_string();
            row["Runtime"] = entry_row["Runtime"]->as_string();
            row["Exit Status"] = std::to_string(entry_row["ExitStatus"]->as_int());
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

        auto result = log->execute_sql("SELECT * FROM Log WHERE ID = '" + id + "'");
        if (!result.good() || result.begin() == result.end())
        {
            result.output_errors();
            response.send_text("Error");
            return;
        }

        auto &row = *result.begin();
        log_entry_template["name"] = row["Name"]->as_string();
        log_entry_template["date"] = row["Date"]->as_string();
        log_entry_template["time"] = row["Time"]->as_string();
        log_entry_template["id"] = row["ID"]->as_string();
        log_entry_template["exit_status"] = std::to_string(row["ExitStatus"]->as_int());
        log_entry_template["runtime"] = row["Runtime"]->as_string();
        log_entry_template["output"] = row["Output"]->as_string();

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

static void setup_database(DB::DataBase &db)
{
    auto log_table = db.get_table("Log");
    if (!log_table)
    {
        db.execute_sql(std::string(
            "CREATE TABLE Log (")
                + "Date CHAR(80), "
                + "Time CHAR(80), "
                + "ExitStatus INTEGER, "
                + "Runtime CHAR(80), "
                + "ID CHAR(80), "
                + "Name CHAR(80), "
                + "Output TEXT)");
    }

    auto history_table = db.get_table("History");
    if (!history_table)
    {
        db.execute_sql(std::string(
            "CREATE TABLE History (")
                + "Name CHAR(80), "
                + "Date CHAR(80), "
                + "LastDailyTime INTEGER)");
    }
}

static void start_cron(const std::string &cron_path, bool enable_web_interface, int port)
{
    auto log_path = cron_path + "/log/log-" + Timer::make_time_stamp() + ".json";
    std::vector<Cron> crons;
    size_t run_time = 0;
    std::mutex log_mutex;

    std::cout << "Setting up database\n";
    auto log = DB::DataBase::open(cron_path + "/log.db");
    assert (log);
    setup_database(*log);

    std::cout << "Loading crons\n";
    load_crons(crons, *log, cron_path);

    // Insert startup message
    log->execute_sql(std::string("INSERT INTO Log (Date, Time, ExitStatus, Runtime, ID, Name, Output) VALUES (")
        + "'" + Timer::make_time_stamp() + "', "
        + "'" + Timer::time_string() + "', "
        + "0, "
        + "'0m 0s', "
        + "'" + std::to_string(rand()) + "', "
        + "'Start Up', "
        + "'Started up crond with " + std::to_string(crons.size()) + " job(s)')");

    std::thread web;
    if (enable_web_interface)
        web = std::thread(web_thread, log.get(), &log_mutex, cron_path, port);

    for (;;)
    {
        for (auto &cron : crons)
        {
            if (cron.should_run_and_mark_done(run_time))
                cron.run(*log, log_mutex);
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
