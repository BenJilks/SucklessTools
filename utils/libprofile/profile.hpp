#pragma once

#include <string>
#include <chrono>
#include <map>
#include <vector>
#include <iostream>
#include <memory>

namespace Profile
{

    class Timer
    {
    public:
        Timer(const std::string &name);
        ~Timer();

    private:
        std::chrono::steady_clock::time_point m_start;

    };

    class Profiler
    {
        friend Timer;

    public:
        static Profiler &the();

        void print_results(std::ostream& = std::cout);

    private:
        class TreeNode
        {
        public:
            TreeNode(const std::string &name, TreeNode *parent = nullptr)
                : m_name(name)
                , m_parent(parent) {}

            void record_time(uint64_t time);
            TreeNode *start_new(const std::string &name);
            void print_results(std::ostream& = std::cout, int indent = 1);

            TreeNode *parent() { return m_parent; }
            const std::string &name() const { return m_name; }
            uint64_t average() const;
            uint64_t total() const;
            int call_count() const { return m_times.size(); }
            bool is_leaf() const { return m_children.size() == 0; }

            template<typename Callback>
            void for_each_child(Callback callback)
            {
                for (const auto &child : m_children)
                    callback(child.get());
            }

        private:
            std::string m_name;
            std::vector<uint64_t> m_times;
            std::vector<std::unique_ptr<TreeNode>> m_children;
            TreeNode *m_parent;

        };

        std::unique_ptr<TreeNode> m_root;
        TreeNode *m_current { nullptr };

        Profiler();
        void start_scope(const std::string &name);
        void record_time_and_end_scope(uint64_t time);
        void print_functions_spent_most_time_in(std::ostream&);

    };

}
