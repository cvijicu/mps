#include "kernels.hpp"
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <functional>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <cstdlib>
#include <string>
#include <vector>

using namespace std;

void fail(const string& message)
{
    cerr << "Error: " << message << '\n';
    exit(1);
}

struct Method
{
    string name;
    string schedule;
    int requested_num_workers;
    function<int(Samples&)> compute;
    vector<double> times;
    int num_workers = 0;
};

int number(const string& text, int maximum)
{
    int value = 0;
    auto [end, error] = from_chars(text.data(), text.data() + text.size(), value);
    if (error != errc{} || end != text.data() + text.size() ||
        value < 1 || value > maximum)
    {
        fail("Expected an integer in [1, " + to_string(maximum) + "]");
    }
    return value;
}

void check(const Samples& actual, const Samples& expected)
{
    if (actual.size() != expected.size())
    {
        fail("Wrong output size");
    }
    for (size_t i = 0; i < expected.size(); ++i)
    {
        if (!isfinite(actual[i]) || !isfinite(expected[i]) ||
            abs(actual[i] - expected[i]) > 1e-8 * max(1.0, abs(expected[i])))
        {
            fail("Incorrect output at index " + to_string(i));
        }
    }
}

Samples empty_output(int m)
{
    return Samples(m, numeric_limits<double>::quiet_NaN());
}

void small_checks()
{
    struct Case
    {
        Samples x;
        int w;
        Samples expected;
    };

    vector<Case> cases{
        {{1, 2, 3, 4, 5, 6, 7, 8}, 3, {2, 3, 4, 5, 6, 7}},
        {{1, 2, 3, 4, 5, 6, 7, 8}, 1, {1, 2, 3, 4, 5, 6, 7, 8}},
        {{1, 2, 3, 4, 5, 6, 7, 8}, 8, {4.5}},
        {{-3}, 1, {-3}},
        {{4, -2, 7, 1, -5}, 2, {1, 2.5, 4, -2}}
    };

    for (const auto& test : cases)
    {
        int m = test.expected.size();
        Samples y = empty_output(m);

        moving_average_direct(test.x, test.w, y);
        check(y, test.expected);

        y = empty_output(m);
        moving_average_sliding(test.x, test.w, y);
        check(y, test.expected);

        for (int num_workers : {1, 3, 8})
        {
            y = empty_output(m);
            moving_average_threads(test.x, test.w, y, num_workers);
            check(y, test.expected);
        }
    }

    cout << "checks=PASS (known outputs, negative values, uneven/empty blocks)\n";
}

void experiment(vector<Method>& methods, const Samples& expected,
    int n, int w, const string& csv_path)
{
    int m = expected.size();
    vector<Samples> outputs(methods.size(), empty_output(m));

    for (size_t k = 0; k < methods.size(); ++k)
    {
        methods[k].num_workers = methods[k].compute(outputs[k]);
        if (methods[k].num_workers < 1 || methods[k].num_workers > methods[k].requested_num_workers)
        {
            fail("Invalid actual worker count");
        }
        check(outputs[k], expected);
    }

    ofstream csv(csv_path);
    if (!csv)
    {
        fail("Cannot open CSV file: " + csv_path);
    }
    csv << setprecision(17);
    csv << "method,workload,n,w,requested_workers,workers,schedule,repetition,time_s,checksum\n";

    for (int repeat = 0; repeat < 5; ++repeat)
    {
        for (size_t slot = 0; slot < methods.size(); ++slot)
        {
            size_t k = repeat % 2 == 0 ? slot : methods.size() - 1 - slot;
            auto& method = methods[k];
            auto& y = outputs[k];

            fill(y.begin(), y.end(), numeric_limits<double>::quiet_NaN());
            auto start = chrono::steady_clock::now();
            int actual_num_workers = method.compute(y);
            auto finish = chrono::steady_clock::now();

            if (actual_num_workers != method.num_workers)
            {
                fail("Team size changed between runs");
            }
            check(y, expected);

            double seconds = chrono::duration<double>(finish - start).count();
            method.times.push_back(seconds);

            double checksum = 0;
            for (double value : y)
            {
                checksum += value;
            }

            csv << method.name << ",regular," << n << ',' << w << ','
                << method.requested_num_workers << ',' << actual_num_workers << ',' << quoted(method.schedule)
                << ',' << repeat + 1 << ',' << seconds << ',' << checksum << '\n';
        }
    }

    csv.close();
    if (!csv)
    {
        fail("Cannot write CSV file: " + csv_path);
    }

    double baseline = 0;
    for (double seconds : methods.front().times)
    {
        baseline += seconds / 5;
    }

    for (const auto& method : methods)
    {
        double mean = 0;
        for (double seconds : method.times)
        {
            mean += seconds / 5;
        }
        auto [low, high] = minmax_element(method.times.begin(), method.times.end());
        cout << "summary method=" << method.name << " schedule=" << method.schedule
            << " workers=" << method.num_workers << " mean_s=" << mean
            << " min_s=" << *low << " max_s=" << *high
            << " outputs_per_s=" << m / mean;
        if (method.name == "threads")
        {
            cout << " speedup=" << baseline / mean
                << " efficiency=" << baseline / mean / method.num_workers;
        }
        cout << '\n';
    }

    cout << "Saved measurements to " << csv_path << '\n';
}

int main(int argc, char** argv)
{
    cout << setprecision(17);

    if (argc == 2 && string(argv[1]) == "--check")
    {
        small_checks();
        return 0;
    }

    if (argc != 4 && argc != 5)
    {
        fail("usage: thread_average N W WORKERS [CSV_FILE]");
    }
    const string csv_path = argc == 5 ? argv[4] : "thread_average.csv";
    int n = number(argv[1], 10'000'000);
    int w = number(argv[2], 10'000'000);
    int num_workers = number(argv[3], 128);
    if (w > n)
    {
        fail("W must not exceed N");
    }
    small_checks();

    int m = n - w + 1;
    Samples x(n);
    for (int i = 0; i < n; ++i)
    {
        x[i] = 0.001 * (i % 1000);
    }
    Samples expected = empty_output(m);
    moving_average_direct(x, w, expected);

    vector<Method> methods;
    methods.push_back({"direct", "none", 1, [&](Samples& y)
        {
            moving_average_direct(x, w, y);
            return 1;
        }, {}});
    methods.push_back({"sliding", "none", 1, [&](Samples& y)
        {
            moving_average_sliding(x, w, y);
            return 1;
        }, {}});
    methods.push_back({"threads", "block", num_workers, [&](Samples& y)
        {
            moving_average_threads(x, w, y, num_workers);
            return num_workers;
        }, {}});
    cout << "compiler=" << __VERSION__ << " workload=regular"
        << " n=" << n << " w=" << w << " outputs=" << m
        << " requested_workers=" << num_workers << " repetitions=5 warmups=1\n";
    experiment(methods, expected, n, w, csv_path);
    return 0;
}
