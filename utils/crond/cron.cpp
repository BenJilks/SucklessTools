#include "cron.hpp"
#include <chrono>
#include <fstream>
#include <libjson/libjson.hpp>
#include <database/database.hpp>
#include <optional>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>

#define READ 		0
#define WRITE 		1

std::optional<Cron> Cron::load(const std::string &path, DB::DataBase &log, std::ostream &out)
{
    const auto config_path = path + "/config.json";
    auto config = Json::parse(std::ifstream(config_path));
    if (config.is_null())
        return std::nullopt;

    if (config["disable"].as_boolean())
        return std::nullopt;

    auto &json_name = config["name"];
    auto &json_script = config["script"];
    if (!json_script.is_string())
    {
        out << path << ": Invalid script given, not loading cron\n";
        return std::nullopt;
    }

    auto name = json_name.otherwise(Json::Value::string(path)).as_string();
    auto script = path + "/" + json_script.as_string();
    auto timer = Timer::parse(config, log, out);
    if (!timer)
    {
        // TODO: Error
        assert (false);
        return std::nullopt;
    }

    if (!script.empty() && script[0] != '/')
        script = "./" + script;
    return Cron(name, script, std::move(*timer));
}

bool Cron::should_run_and_mark_done(size_t run_time)
{
    return m_timer.should_run_and_mark_done(run_time);
}

static std::string new_id()
{
    return std::to_string(rand());
}

void Cron::run(DB::DataBase &log, std::mutex &log_mutex)
{
    auto start_time = Timer::time_string();
    auto id = new_id();

    log_mutex.lock();
    log.execute_sql(std::string("INSERT INTO Log (Name, ID, Time, Date) VALUES (")
        + "'" + m_name + "', "
        + "'" + id + "', "
        + "'" + start_time + "', "
        + "'" + Timer::make_time_stamp() + "')");
    log_mutex.unlock();

    std::cout << "\nRunning cron " << m_name << "...\n";
    auto begin = std::chrono::steady_clock::now();

    int p[2];
	if (pipe(p) < 0)
	{
		perror("pipe()");
        return;
	}

	auto pid = fork();
    if (pid < 0)
    {
        perror("fork()");
        return;
    }

    if (pid == 0)
    {
        close(STDOUT_FILENO);
        dup(p[WRITE]);
        close(p[READ]);
        close(p[WRITE]);

        if (execl("/bin/bash", "/bin/bash", m_script_path.c_str(), nullptr) < 0)
            perror("execl()");

        // We shouldn't be here
        std::cout << "Error: Unable to execute " << m_script_path << "\n";
        exit(-1);
    }

    close(p[WRITE]);
    int status = 0;
    bool is_still_running = true;

    std::string log_buffer;
    int log_buffer_pointer = 0;
    while (is_still_running)
    {
        if (waitpid(pid, &status, WNOHANG) == pid)
            is_still_running = false;

        if (log_buffer_pointer + 255 > log_buffer.size())
            log_buffer.resize(log_buffer.size() + 255);

        for (;;)
        {
            auto len = read(p[READ], log_buffer.data() + log_buffer_pointer, 255);
            if (len <= 0)
                break;

            log_buffer_pointer += len;
        }
    }
    std::cout << "Done.\n";
    close(p[READ]);
    log_buffer.resize(log_buffer_pointer);

    auto end = std::chrono::steady_clock::now();
    auto end_time = Timer::time_string();
    auto runtime_sec = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
    auto runtime_msec = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    auto runtime = std::to_string(runtime_sec) + "s " + std::to_string(runtime_msec) + "ms";

    log_mutex.lock();
    log.execute_sql(std::string("UPDATE Log SET ")
        + "ExitStatus = " + std::to_string(status) + ", "
        + "Output = '" + log_buffer + "', "
        + "Runtime = '" + runtime + "' WHERE ID = '" + id + "'");
    log_mutex.unlock();
}
