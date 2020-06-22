#include "timer.hpp"
#include <fstream>
#include <libjson/libjson.hpp>
#include <database/database.hpp>
#include <memory.h>

Timer::Timer(DB::DataBase &log, std::string name)
    : m_name(name)
    , m_log(log)
{
}

std::optional<Timer::DailyTime> Timer::parse_daily_time(const std::string &str, std::ostream& out)
{
    DailyTime daily_time { 0, 0 };
    std::string buffer;

    for (char c : str)
    {
        if (std::isspace(c))
            continue;

        if (c == 'm')
        {
            daily_time.minute = atoi(buffer.c_str());
            buffer.clear();
            continue;
        }

        if (c == 'h')
        {
            daily_time.hour = atoi(buffer.c_str());
            buffer.clear();
            continue;
        }

        if (!isdigit(c))
        {
            out << "Timer: DailyTime: Invalid time unit '" << c << "'\n";
            return std::nullopt;
        }

        buffer += c;
    }

    return daily_time;
}

static std::string &to_lower(std::string &&str)
{
    for (size_t i = 0; i < str.length(); i++)
        str[i] = tolower(str[i]);

    return str;
}

std::optional<Timer> Timer::parse(const Json::Value &cron, DB::DataBase &log, std::ostream &out)
{
    Timer timer(log, cron["name"].as_string());

    auto &daily = cron["daily"];
    if (daily.is_string())
    {
        auto time_str = daily.as_string();
        auto daily_time = parse_daily_time(time_str, out);
        if (!daily_time)
            return std::nullopt;
        timer.m_every_daily_time = *daily_time;
    }
    else if (daily.is_array())
    {
        for (const auto &item : daily.as_array())
        {
            auto time_str = item.as_string();
            auto daily_time = parse_daily_time(time_str, out);
            if (!daily_time)
                return std::nullopt;
            timer.m_daily.push_back(*daily_time);
        }
    }
    else
    {
        // TODO: Much better error message here
        out << "Timer: Invalid daily, must be either array of times or one\n";
        return std::nullopt;
    }

    auto &weekly = cron["weekly"];
    if (weekly.is_array())
    {
        int week = (int)Week::None;
        for (const auto &item : weekly.as_array())
        {
            if (!item.is_string())
                assert (false);

            auto str = to_lower(item.as_string());
            if (str == "monday") week |= (int)Week::Monday;
            else if (str == "tuesday") week |= (int)Week::Tuesday;
            else if (str == "wednessday") week |= (int)Week::Wednessday;
            else if (str == "thursday") week |= (int)Week::Thursday;
            else if (str == "friday") week |= (int)Week::Friday;
            else if (str == "saterday") week |= (int)Week::Saterday;
            else if (str == "sunday") week |= (int)Week::Sunday;
            else if (str == "weekend") week |= ((int)Week::Saterday | (int)Week::Sunday);
            else if (str == "weekday") week |= ~((int)Week::Saterday | (int)Week::Sunday);
            else
            {
                // TODO: Error here
                assert (false);
                return std::nullopt;
            }
        }

        timer.m_weekly = (Week)week;
    }

    return timer;
}

bool Timer::is_day(int index) const
{
    assert (m_weekly);

    switch (index)
    {
        case 0: return (int)*m_weekly & (int)Week::Sunday;
        case 1: return (int)*m_weekly & (int)Week::Monday;
        case 2: return (int)*m_weekly & (int)Week::Tuesday;
        case 3: return (int)*m_weekly & (int)Week::Wednessday;
        case 4: return (int)*m_weekly & (int)Week::Thursday;
        case 5: return (int)*m_weekly & (int)Week::Friday;
        case 6: return (int)*m_weekly & (int)Week::Saterday;
    }

    return false;
}

bool Timer::has_done_daily(const DailyTime& daily_time) const
{
    auto this_daily_time = (daily_time.hour * 60) + daily_time.minute;
    auto now = make_time_stamp();

    auto result = m_log.execute_sql("SELECT * FROM History WHERE Name = '" + m_name + "'");
    if (!result.good())
    {
        result.output_errors();
        assert (false);
    }

    if (result.begin() != result.end())
    {
        auto &row = *result.begin();
        auto last_date = row["Date"]->as_string();
        auto last_daily_time = row["LastDailyTime"]->as_int();

        if (last_daily_time < this_daily_time || last_date != now)
        {
            assert (m_log.execute_sql(std::string("UPDATE History SET ")
                + "LastDailyTime = " + std::to_string(this_daily_time) + ", "
                + "Date = '" + now + "' "
                + "WHERE Name = " + m_name).good());

            return false;
        }

        return true;
    }

    assert (m_log.execute_sql(std::string("INSERT INTO History (Name, Date, LastDailyTime) VALUES (")
        + "'" + m_name + "', "
        + "'" + now + "', "
        + std::to_string(this_daily_time) + ")").good());

    return false;
}

std::string Timer::make_time_stamp(const struct tm *date_time)
{
    char buffer[80];
    sprintf(buffer, "%04d-%02d-%02d",
            1900 + date_time->tm_year,
            date_time->tm_mon,
            date_time->tm_mday);
    return std::string(buffer);
}

std::string Timer::make_time_stamp()
{
    const auto now = time(0);
    const auto *date_time = localtime(&now);
    return make_time_stamp(date_time);
}

static time_t find_unix_time(const std::string &now)
{
    int year, month, day;
    sscanf(now.c_str(), "%04d-%02d-%02d", &year, &month, &day);

    struct tm time_info;
    memset(&time_info, 0, sizeof time_info);
    time_info.tm_year = year - 1900;
    time_info.tm_mon = month;
    time_info.tm_mday = day;
    return mktime(&time_info);
}

std::string Timer::make_time_stamp_yeserday(const std::string &now)
{
    auto unix_time = time(NULL) - 60 * 60 * 24;
    if (!now.empty())
        unix_time = find_unix_time(now) - 60 * 60 * 24;

    const auto *date_time = localtime(&unix_time);
    return make_time_stamp(date_time);
}

std::string Timer::make_time_stamp_tomorrow(const std::string &now)
{
    auto unix_time = time(0) + 60 * 60 * 24;
    if (!now.empty())
        unix_time = find_unix_time(now) + 60 * 60 * 24;

    const auto *date_time = localtime(&unix_time);
    return make_time_stamp(date_time);
}

std::string Timer::time_string()
{
    const auto now = time(0);
    const auto *date_time = localtime(&now);
    char buffer[80];

    sprintf(buffer, "%02d:%02d:%02d",
            date_time->tm_hour,
            date_time->tm_min,
            date_time->tm_sec);
    return std::string(buffer);
}

bool Timer::should_run(size_t run_time) const
{
    const auto now = time(0);
    const auto *date_time = localtime(&now);

    if (m_weekly)
    {
        if (!is_day(date_time->tm_wday))
            return false;
    }

    for (const auto &daily_time : m_daily)
    {
        if (date_time->tm_hour >= daily_time.hour &&
            date_time->tm_min >= daily_time.minute)
        {
            return !has_done_daily(daily_time);
        }
    }

    if (m_every_daily_time)
    {
        size_t minutes = m_every_daily_time->hour * 60 + m_every_daily_time->minute;
        if (run_time % minutes == 0)
            return true;
    }

    return false;
}

bool Timer::should_run_and_mark_done(size_t run_time)
{
    return should_run(run_time);
}
