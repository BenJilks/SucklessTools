#pragma once
#include <libjson/forward.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class Timer
{
public:
    static std::optional<Timer> parse(const Json::Value &cron, const std::string &log_path, std::ostream&);

    bool should_run_and_mark_done(size_t run_time);
    void make_done();
    static std::string make_time_stamp(const struct tm *date_time);
    static std::string make_time_stamp();
    static std::string make_time_stamp_yeserday(const std::string &now = {});
    static std::string make_time_stamp_tomorrow(const std::string &now = {});
    static std::string time_string();

private:
    Timer(const std::string &log_path);

    enum class Week
    {
        None = 0,
        Monday = 1 << 0,
        Tuesday = 1 << 1,
        Wednessday = 1 << 2,
        Thursday = 1 << 3,
        Friday = 1 << 4,
        Saterday = 1 << 5,
        Sunday = 1 << 6,
    };

    struct DailyTime
    {
        size_t hour;
        size_t minute;
    };

    static std::optional<DailyTime> parse_daily_time(const std::string &str, std::ostream&);
    bool should_run(size_t run_time) const;
    bool has_done_daily(const DailyTime&, const std::string &timestamp) const;
    bool is_day(int index) const;

    std::unique_ptr<Json::Document> m_log_doc;
    std::string m_log_path;

    std::optional<DailyTime> m_every_daily_time;
    std::vector<DailyTime> m_daily;
    std::optional<Week> m_weekly;
};
