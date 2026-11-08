#include "average/kernels.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
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

using Kernel = void (*)(const Samples&, int, Samples&);
constexpr int repetitions = 5;

bool close(double actual, double expected)
{
    return isfinite(actual) && isfinite(expected) &&
        abs(actual - expected) <= 1e-8 * max(1.0, abs(expected));
}

void compare(const Samples& actual, const Samples& expected,
    const string& context)
{
    if (actual.size() != expected.size())
    {
        fail(context + ": wrong output size");
    }
    for (size_t i = 0; i < actual.size(); ++i)
    {
        if (!close(actual[i], expected[i]))
        {
            fail(context +
            ": incorrect output at index " + to_string(i));
        }
    }
}

void check_small_inputs()
{
    const Samples x{1, 2, 3, 4, 5, 6, 7, 8};
    const array<int, 3> windows{3, 1, 8};
    const array<Samples, 3> expected{Samples{2, 3, 4, 5, 6, 7}, x, Samples{4.5}};
    for (Kernel kernel : {moving_average_direct, moving_average_sliding})
    {
        for (size_t test = 0; test < windows.size(); ++test)
        {
            Samples y(expected[test].size(), numeric_limits<double>::quiet_NaN());
            kernel(x, windows[test], y);
            compare(y, expected[test], "small-input check");
        }
    }

    cout << "checks=PASS (W=3, W=1, W=N; both versions)\n";
}

int positive_integer(const string& text)
{
    int value = 0;
    const auto [end, error] = from_chars(text.data(), text.data() + text.size(), value);
    if (error != errc{} || end != text.data() + text.size() || value <= 0 || value > 10'000'000)
    {
        fail("N and W must be integers in [1, 10000000]");
    }
    return value;
}

double measure(Kernel kernel, const Samples& x, int w, Samples& y)
{
    fill(y.begin(), y.end(), numeric_limits<double>::quiet_NaN());
    const auto start = chrono::steady_clock::now();
    kernel(x, w, y);
    const auto finish = chrono::steady_clock::now();
    if (y.size() != x.size() - w + 1)
    {
        fail("kernel changed the output size");
    }
    return chrono::duration<double>(finish - start).count();
}

double checksum(const Samples& y)
{
    double sum = 0;
    for (double value : y)
    {
        sum += value;
    }
    return sum;
}

void summary(const string& name, const array<double, repetitions>& times,
    size_t outputs)
{
    double total = 0;
    for (double time : times)
    {
        total += time;
    }
    const double mean = total / repetitions;
    const auto [minimum, maximum] = minmax_element(times.begin(), times.end());
    cout << "summary method=" << name << " mean_s=" << mean
        << " min_s=" << *minimum << " max_s=" << *maximum
        << " outputs_per_s=" << outputs / mean << '\n';
}

int main(int argc, char** argv)
{
    cout << setprecision(17);

    if (argc == 2 && string(argv[1]) == "--check")
    {
        check_small_inputs();
        return 0;
    }

    if (argc != 3 && argc != 4)
    {
        cerr << "usage: " << argv[0] << " N W [CSV_FILE] | --check\n";
        return 1;
    }
    const string csv_path = argc == 4 ? argv[3] : "moving_average.csv";
    const int n = positive_integer(argv[1]);
    const int w = positive_integer(argv[2]);
    if (w > n)
    {
        fail("W must not exceed N");
    }
    check_small_inputs();

    Samples x(n);
    for (int i = 0; i < n; ++i)
    {
        x[i] = 0.001 * (i % 1000);
    }
    const size_t outputs = n - w + 1;
    Samples direct(outputs), sliding(outputs);

    fill(direct.begin(), direct.end(), numeric_limits<double>::quiet_NaN());
    fill(sliding.begin(), sliding.end(), numeric_limits<double>::quiet_NaN());
    moving_average_direct(x, w, direct);
    moving_average_sliding(x, w, sliding);
    if (direct.size() != outputs || sliding.size() != outputs)
    {
        fail("kernel changed the output size");
    }
    compare(sliding, direct, "warm-up agreement");
    cout << "n=" << n << " w=" << w << " outputs=" << outputs
        << " repetitions=" << repetitions << " warmups_per_version=1\n"
        << "compiler=" << __VERSION__ << " tolerance=1e-8*max(1,abs(reference))\n";

    array<double, repetitions> direct_times{}, sliding_times{};
    ofstream csv(csv_path);
    if (!csv)
    {
        fail("Cannot open CSV file: " + csv_path);
    }
    csv << setprecision(17);
    csv << "method,n,w,repetition,time_s,checksum\n";

    for (int repeat = 0; repeat < repetitions; ++repeat)
    {

        if (repeat % 2 == 0)
        {
            direct_times[repeat] = measure(moving_average_direct, x, w, direct);
            sliding_times[repeat] = measure(moving_average_sliding, x, w, sliding);
        }
        else
        {
            sliding_times[repeat] = measure(moving_average_sliding, x, w, sliding);
            direct_times[repeat] = measure(moving_average_direct, x, w, direct);
        }
        compare(sliding, direct, "measured-output agreement");
        csv << "direct," << n << ',' << w << ',' << repeat + 1 << ','
            << direct_times[repeat] << ',' << checksum(direct) << '\n';
        csv << "sliding," << n << ',' << w << ',' << repeat + 1 << ','
            << sliding_times[repeat] << ',' << checksum(sliding) << '\n';
    }

    csv.close();
    if (!csv)
    {
        fail("Cannot write CSV file: " + csv_path);
    }
    summary("direct", direct_times, outputs);
    summary("sliding", sliding_times, outputs);
    cout << "Saved measurements to " << csv_path << '\n';
    return 0;
}
