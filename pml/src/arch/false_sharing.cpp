#include "cache_support.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>

using namespace std;

#ifndef PML_LINE_BYTES
#define PML_LINE_BYTES 64
#endif

// BEGIN countertypes
using Count = unsigned long long;

struct Counter
{
    atomic<Count> value{0};
};

struct alignas(PML_LINE_BYTES) PaddedCounter
{
    atomic<Count> value{0};
};

// END countertypes

// BEGIN increment
void increment(atomic<Count>& value, Count iterations)
{
    for (Count i = 0; i < iterations; ++i)
    {
        value.fetch_add(1);
    }
}

// END increment

template<class T>
class Experiment
{
    vector<T> counters;
    vector<thread> threads;

public:
    int shared_lines = 0;
    bool lock_free = true;

    explicit Experiment(int num_workers) : counters(num_workers)
    {
        threads.reserve(num_workers);
    }

    void describe(const string& name)
    {
        map<uintptr_t, int> line_writers;
        for (int id = 0; id < static_cast<int>(counters.size()); ++id)
        {
            const auto address = reinterpret_cast<uintptr_t>(&counters[id].value);
            const auto line = address / PML_LINE_BYTES;
            ++line_writers[line];
            lock_free = lock_free && counters[id].value.is_lock_free();
            cout << "layout=" << name << " id=" << id << " address=" << address
                 << " line=" << line << " offset=" << address % PML_LINE_BYTES << '\n';
        }
        for (const auto& item : line_writers)
        {
            if (item.second > 1)
            {
                ++shared_lines;
            }
        }
        cout << "layout=" << name << " counter_size=" << sizeof(T)
             << " line_bytes_assumed=" << PML_LINE_BYTES << " lock_free=" << lock_free
             << " shared_lines=" << shared_lines << '\n';
    }

    double run(Count iterations)
    {
        threads.clear();
        for (auto& counter : counters)
        {
            counter.value.store(0);
        }

        const auto start = chrono::steady_clock::now();
        for (int id = 0; id < static_cast<int>(counters.size()); ++id)
        {
            threads.emplace_back(increment, ref(counters[id].value), iterations);
        }
        for (auto& thread : threads)
        {
            thread.join();
        }
        const auto finish = chrono::steady_clock::now();

        for (auto& counter : counters)
        {
            if (counter.value.load() != iterations)
            {
                cache_experiment::fail("Incorrect per-thread counter");
            }
        }
        const double seconds = chrono::duration<double>(finish - start).count();
        cache_experiment::elapsed_ok(seconds);
        return seconds;
    }
};

void small_checks()
{
    for (int num_workers : {1, 2, 4})
    {
        Experiment<Counter> compact(num_workers);
        Experiment<PaddedCounter> padded(num_workers);
        compact.run(0);
        compact.run(37);
        compact.run(11);
        padded.run(0);
        padded.run(37);
        padded.run(11);
    }
    cout << "checks=passed cases=18\n";
}

int main(int argc, char** argv)
{
    if (argc == 2 && string(argv[1]) == "--check")
    {
        small_checks();
        return 0;
    }
    if (argc > 5)
    {
        cache_experiment::fail("usage: false_sharing THREADS N [compact-first|padded-first] [CSV]");
    }
    const int num_workers = argc > 1 ? cache_experiment::number(argv[1], 128) : 2;
    const Count iterations = argc > 2 ? cache_experiment::number(argv[2], 100000000) : 1000000;
    const string order = argc > 3 ? argv[3] : "compact-first";
    const string path = argc > 4 ? argv[4] : "false_sharing.csv";
    if (order != "compact-first" && order != "padded-first")
    {
        cache_experiment::fail("Expected compact-first or padded-first");
    }
    small_checks();

    Experiment<Counter> compact(num_workers);
    Experiment<PaddedCounter> padded(num_workers);
    compact.describe("compact");
    padded.describe("padded");
    const vector<string> names{"compact", "padded"};
    const int first = order == "compact-first" ? 0 : 1;
    vector<vector<double>> times(2);
    for (int offset = 0; offset < 2; ++offset)
    {
        if ((first + offset) % 2 == 0)
        {
            compact.run(iterations);
        }
        else
        {
            padded.run(iterations);
        }
    }
    auto csv = cache_experiment::csv_file(path);
    for (int repetition = 1; repetition <= 5; ++repetition)
    {
        for (int position = 1; position <= 2; ++position)
        {
            const int method = (first + repetition - 1 + position - 1) % 2;
            const double seconds = method == 0 ? compact.run(iterations) : padded.run(iterations);
            times[method].push_back(seconds);
            csv << names[method] << ",counters," << iterations << ',' << num_workers
                << ',' << repetition << ',' << position << ',' << seconds << ','
                << iterations * num_workers << ',' << iterations * num_workers
                << ',' << sizeof(Count) << ',' << PML_LINE_BYTES << ','
                << (method == 0 ? sizeof(Counter) : sizeof(PaddedCounter)) << ','
                << (method == 0 ? compact.lock_free : padded.lock_free) << ','
                << (method == 0 ? compact.shared_lines : padded.shared_lines) << '\n';
        }
    }
    csv.flush();
    cache_experiment::csv_ok(csv);
    cout << "compiler=" << __VERSION__ << " workers=" << num_workers
         << " iterations=" << iterations << " total_updates=" << iterations * num_workers
         << " repetitions=5 warmups=1\n";
    for (int method = 0; method < 2; ++method)
    {
        cache_experiment::summary(names[method], times[method]);
    }
    cout << "csv=" << path << '\n';
    return 0;
}
