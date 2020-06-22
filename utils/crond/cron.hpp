#pragma once
#include "timer.hpp"
#include <string>
#include <libjson/forward.hpp>
#include <database/forward.hpp>
#include <mutex>

class Cron
{
public:
    static std::optional<Cron> load(const std::string &path, DB::DataBase&, std::ostream &);

    bool should_run_and_mark_done(size_t run_time);
    void run(DB::DataBase &log, std::mutex &log_mutex);

    inline const std::string &name() const { return m_name; }

private:
    Cron(const std::string &name, const std::string &script, Timer timer)
        : m_name(name)
        , m_script_path(script)
        , m_timer(std::move(timer)) {}

    std::string m_name;
    std::string m_script_path;
    Timer m_timer;

};
