#include "cron.hpp"
#include <chrono>
#include <fstream>
#include <libjson/libjson.hpp>
#include <optional>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>

#define READ 		0
#define WRITE 		1

std::optional<Cron> Cron::load(const std::string &path, std::ostream &out)
{
    const auto config_path = path + "/config.json";
    const auto log_path = path + "/log.json";
    auto config_doc = Json::Document::parse(std::ifstream(config_path));
    if (config_doc.has_error())
        config_doc.log_errors(out);

    auto &config = config_doc.root();
    if (!config_doc.root().is_object())
        return std::nullopt;

    auto &json_name = config["name"];
    auto &json_script = config["script"];
    if (!json_script.is_string())
    {
        out << path << ": Invalid script given, not loading cron\n";
        return std::nullopt;
    }

    auto name = json_name.to_string_or(path);
    auto script = "./" + path + "/" + json_script.to_string();
    auto timer = Timer::parse(config, log_path, out);
    if (!timer)
    {
        // TODO: Error
        assert (false);
        return std::nullopt;
    }

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

void Cron::run(Json::Value &log, std::mutex &log_mutex)
{
    log_mutex.lock();
    auto start_time = Timer::time_string();
    auto &log_entry = log.append_new<Json::Object>();
    log_entry.add("name", m_name);
    log_entry.add("id", new_id());
    log_entry.add("start_time", start_time);
    log_entry.add("runtime", "In Progress...");
    log_entry.add("end_time", "In Progress...");
    log_entry.add_new<Json::Number>("exit_status", -1);
    log_mutex.unlock();

    std::cout << "Running cron " << m_name << " [|]";
    std::cout.flush();
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

    std::string log_buffer;
    if (pid == 0)
    {
        close(STDOUT_FILENO);
        dup(p[WRITE]);
        close(p[READ]);
        close(p[WRITE]);

        if (execl(m_script_path.c_str(), m_script_path.c_str(), nullptr) < 0)
            perror("execl()");

        // We shouldn't be here
        exit(-1);
    }

    close(p[WRITE]);
    int status = 0;
    int tick = 0;
    for (;;)
    {
        char c;
        while (read(p[READ], &c, 1))
            log_buffer += c;

        static std::vector<char> anim = {'|', '/', '-', '\\'};
        std::cout << "\b\b" << anim[tick % 4] << "]";
        std::cout.flush();
        tick += 1;

        log_mutex.lock();
        log_entry.add("output", log_buffer);
        log_mutex.unlock();

        if (waitpid(pid, &status, WNOHANG) == pid)
            break;
    }
    std::cout << "\b\bDone]\n";
    close(p[READ]);

    log_mutex.lock();
    auto end = std::chrono::steady_clock::now();
    auto end_time = Timer::time_string();
    auto runtime_sec = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
    auto runtime_msec = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    auto runtime = std::to_string(runtime_sec) + "s " + std::to_string(runtime_msec) + "ms";
    log_entry.add_new<Json::Number>("exit_status", status);
    log_entry.add("output", log_buffer);
    log_entry.add("runtime", runtime);
    log_entry.add("end_time", end_time);
    log_mutex.unlock();
}
