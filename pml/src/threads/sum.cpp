#include <chrono>
#include <climits>
#include <cstdlib>
#include <iostream>
#include <functional>
#include <numeric>
#include <thread>
#include <vector>

using namespace std;
using Sum = long long;

void sum_block(int begin, int end, Sum& result)
{
    Sum local = 0;

    for (int i = begin; i < end; ++i)
    {
        local += i + 1;
    }

    result = local;
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        cerr << "usage: " << argv[0] << " THREADS N\n";
        return 1;
    }
    char* end = nullptr;
    const long requested_num_workers = strtol(argv[1], &end, 10);
    if (end == argv[1] || *end != '\0' || requested_num_workers < 1 || requested_num_workers > INT_MAX)
    {
        return 1;
    }
    const int num_workers = requested_num_workers;
    const Sum size = strtoll(argv[2], &end, 10);
    if (end == argv[2] || *end != '\0' || size < 0 || size > INT_MAX)
    {
        return 1;
    }
    const int n = size;
    vector<Sum> partial(num_workers, 0);
    vector<thread> threads;
    threads.reserve(num_workers);

    const auto start = chrono::steady_clock::now();
    Sum sequential = 0;
    for (int i = 0; i < n; ++i)
    {
        sequential += i + 1;
    }

    const auto split = chrono::steady_clock::now();
    for (int t = 0; t < num_workers; ++t)
    {
        const int begin = Sum(n) * t / num_workers;
        const int end_index = Sum(n) * (t + 1) / num_workers;
        threads.emplace_back(sum_block, begin, end_index, ref(partial[t]));
    }

    for (auto& t : threads)
    {
        t.join();
    }

    const Sum parallel =
        accumulate(partial.begin(), partial.end(), Sum{0});
    const auto finish = chrono::steady_clock::now();
    const Sum num_values = n;
    const Sum expected = num_values * (num_values + 1) / 2;
    if (sequential != expected || parallel != expected)
    {
        return 1;
    }
    cout << "n=" << n << " threads=" << num_workers
        << " result=" << parallel << " sequential_s="
        << chrono::duration<double>(split - start).count()
        << " parallel_s="
        << chrono::duration<double>(finish - split).count()
        << '\n';
}
