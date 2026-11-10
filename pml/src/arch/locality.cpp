#include "cache_kernels.hpp"
#include "cache_support.hpp"
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

using Kernel = Sum (*)(const Matrix&, int);

void check_result(Sum actual, Sum expected)
{
    if (actual != expected)
    {
        cache_experiment::fail("Incorrect matrix sum: expected " + to_string(expected) +
                               ", got " + to_string(actual));
    }
}

void small_checks()
{
    struct Case
    {
        Matrix a;
        int n;
        Sum expected;
    };
    const vector<Case> cases{
        {{-3}, 1, -3},
        {{1, 2, 3, 4}, 2, 10},
        {{4, -2, 7, 1, -5, 3, 0, 8, -6}, 3, 10}
    };

    for (const auto& test : cases)
    {
        check_result(sum_rows(test.a, test.n), test.expected);
        check_result(sum_columns(test.a, test.n), test.expected);
    }
    cout << "checks=passed cases=" << cases.size() << '\n';
}

double measure(Kernel kernel, const Matrix& a, int n, Sum expected)
{
    const auto start = chrono::steady_clock::now();
    const Sum result = kernel(a, n);
    const auto finish = chrono::steady_clock::now();

    check_result(result, expected);
    const double seconds = chrono::duration<double>(finish - start).count();
    cache_experiment::elapsed_ok(seconds);
    return seconds;
}

int main(int argc, char** argv)
{
    if (argc == 2 && string(argv[1]) == "--check")
    {
        small_checks();
        return 0;
    }
    if (argc > 4)
    {
        cache_experiment::fail("usage: locality N [row-first|column-first] [CSV]");
    }
    const int n = argc > 1 ? cache_experiment::number(argv[1], 8192) : 2048;
    const string order = argc > 2 ? argv[2] : "row-first";
    const string path = argc > 3 ? argv[3] : "locality.csv";
    if (order != "row-first" && order != "column-first")
    {
        cache_experiment::fail("Expected row-first or column-first");
    }
    small_checks();

    Matrix a(n * n);
    Sum expected = 0;
    for (int i = 0; i < n * n; ++i)
    {
        a[i] = i % 17 - 8;
        expected += a[i];
    }
    const vector<string> names{"row", "column"};
    const vector<Kernel> kernels{sum_rows, sum_columns};
    vector<vector<double>> times(2);
    const int first = order == "row-first" ? 0 : 1;
    for (int offset = 0; offset < 2; ++offset)
    {
        measure(kernels[(first + offset) % 2], a, n, expected);
    }
    auto csv = cache_experiment::csv_file(path);
    for (int repetition = 1; repetition <= 5; ++repetition)
    {
        for (int position = 1; position <= 2; ++position)
        {
            const int method = (first + repetition - 1 + position - 1) % 2;
            const double seconds = measure(kernels[method], a, n, expected);
            times[method].push_back(seconds);
            csv << names[method] << ",locality," << n << ",1," << repetition
                << ',' << position << ',' << seconds << ',' << expected << ','
                << n * n << ',' << sizeof(int) << ",0,0,0,0\n";
        }
    }
    csv.flush();
    cache_experiment::csv_ok(csv);
    cout << "compiler=" << __VERSION__ << " n=" << n << " element_bytes="
         << sizeof(int) << " array_bytes=" << a.size() * sizeof(int)
         << " checksum=" << expected << " repetitions=5 warmups=1\n";
    for (int method = 0; method < 2; ++method)
    {
        cache_experiment::summary(names[method], times[method]);
    }
    cout << "csv=" << path << '\n';
    return 0;
}
