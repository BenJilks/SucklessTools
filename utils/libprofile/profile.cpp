#include "profile.hpp"
#include <chrono>
#include <algorithm>
#include <cassert>
#include <iomanip>
#include <functional>
#include <set>
using namespace Profile;

Timer::Timer(const std::string &name)
{
    Profiler::the().start_scope(name);
    m_start = std::chrono::steady_clock::now();
}

Timer::~Timer()
{
    auto end = std::chrono::steady_clock::now();
    auto time_elapsed = std::chrono::duration_cast
            <std::chrono::nanoseconds>(end - m_start);

    Profiler::the().record_time_and_end_scope(time_elapsed.count());
}

Profiler &Profiler::the()
{
    static Profiler profiler;
    return profiler;
}

Profiler::Profiler()
{
    m_root = std::make_unique<TreeNode>("Root");
    m_current = m_root.get();
}

Profiler::TreeNode *Profiler::TreeNode::start_new(const std::string &name)
{
    // First check if there's alread a scope with that name
    for (const auto &child : m_children)
    {
        if (child->name() == name)
            return child.get();
    }

    // Otherwise, make a new one
    auto new_scope = std::make_unique<TreeNode>(name, this);
    m_children.push_back(std::move(new_scope));
    return m_children.back().get();
}

void Profiler::TreeNode::record_time(uint64_t time)
{
    m_times.push_back(time);
}

uint64_t Profiler::TreeNode::average() const
{
    // Avoid devide by zero
    if (m_times.size() == 0)
        return 0;

    uint64_t total = 0;
    for (auto time : m_times)
        total += time;

    return total / m_times.size();
}

uint64_t Profiler::TreeNode::total() const
{
    uint64_t total = 0;
    for (auto time : m_times)
        total += time;

    return total;
}

static std::string human_readable_time(uint64_t nanoseconds)
{
    if (nanoseconds < 10000) // < 10ns
        return std::to_string(nanoseconds) + "ns";
    if (nanoseconds < 10000 * 1000) // < 10us
        return std::to_string(nanoseconds / 1000) + "us";
    if (nanoseconds < 10000L * 1000L * 1000) // < 10ms
        return std::to_string(nanoseconds / (1000 * 1000)) + "ms";

    return std::to_string(nanoseconds / (1000 * 1000 * 1000)) + "s";
}

static std::string print_indent(int indent)
{
    std::string out;
    for (int i = 0; i < indent; i++)
        out += "  ";

    return out;
}

void Profiler::TreeNode::print_results(std::ostream &stream, int indent)
{
    stream << std::left << std::setw(60) << print_indent(indent) + name() + ": ";
    stream << "|" << print_indent(indent) << human_readable_time(total())
           << " (" << m_times.size() << " times, "
           << human_readable_time(average()) << " average)" << "\n";

    for (const auto &child : m_children)
        child->print_results(stream, indent + 1);
}

void Profiler::start_scope(const std::string &name)
{
    m_current = m_current->start_new(name);
}

void Profiler::record_time_and_end_scope(uint64_t time)
{
    m_current->record_time(time);

    // Go back one in the tree
    assert (m_current->parent());
    m_current = m_current->parent();
}

void Profiler::print_results(std::ostream& stream)
{
    stream << "\n";
    stream << "Profiler results:\n";
    m_root->print_results(stream);

    stream << "\n";
    stream << "Function spent most time in:\n";
    print_functions_spent_most_time_in(stream);

    stream << "\n";
}

void Profiler::print_functions_spent_most_time_in(std::ostream &stream)
{
    struct TotalAverage
    {
        uint64_t total;
        uint64_t average;
    };

    struct TotalCount
    {
        uint64_t total;
        int count;
    };

    std::map<std::string, TotalCount> timing_list;
    std::function<void(TreeNode*)> process_node;
    process_node = [&](TreeNode *node)
    {
        if (!node->is_leaf())
        {
            node->for_each_child(process_node);
            return;
        }

        timing_list[node->name()].total += node->total();
        timing_list[node->name()].count += node->call_count();
    };
    process_node(m_root.get());

    std::vector<std::pair<std::string, TotalAverage>> set;
    for (auto &[name, value] : timing_list)
        set.emplace_back(name, TotalAverage { value.total, value.total / value.count });

    std::sort(set.begin(), set.end(),
        [](const auto &a, const auto &b) { return a.second.average >= b.second.average; });
    for (const auto &[name, time] : set)
    {
        stream << "  " << std::left << std::setw(40) << name + ": "
               << human_readable_time(time.average) << " average"
               << " (" << human_readable_time(time.total) << " total)" << "\n";
    }
}
