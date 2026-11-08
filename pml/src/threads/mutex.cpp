#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <thread>
#include <functional>
#include <vector>

using namespace std;

void increment(int n, long long& result, mutex& counter_mutex)
{
    for (int i = 0; i < n; ++i)
    {
        lock_guard<mutex> guard(counter_mutex);
        ++result;
    }
}

int main(int argc, char** argv)
{
    const int num_workers = argc > 1 ? atoi(argv[1]) : 5;
    const int n = argc > 2 ? atoi(argv[2]) : 10000;
    if (num_workers <= 0 || n <= 0)
    {
        return 1;
    }
    long long result = 0;
    mutex counter_mutex;
    vector<thread> threads;
    threads.reserve(num_workers);

    const auto start = chrono::steady_clock::now();
    for (int t = 0; t < num_workers; ++t)
    {
        threads.emplace_back(increment, n, ref(result), ref(counter_mutex));
    }

    for (auto& t : threads)
    {
        t.join();
    }

    const auto finish = chrono::steady_clock::now();
    if (result != static_cast<long long>(num_workers) * n)
    {
        return 1;
    }

    printf("result=%lld time_s=%.9f\n", result,
        chrono::duration<double>(finish - start).count());
}
