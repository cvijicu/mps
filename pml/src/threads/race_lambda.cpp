#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

using namespace std;

int main(int argc, char** argv)
{
    const int num_workers = argc > 1 ? atoi(argv[1]) : 5;
    const int n = argc > 2 ? atoi(argv[2]) : 10000;
    if (num_workers <= 0 || n <= 0)
    {
        return 1;
    }
    long long result = 0;
    vector<thread> threads;
    threads.reserve(num_workers);
    for (int t = 0; t < num_workers; ++t)
    {
        threads.emplace_back([&]
        {
            for (int i = 0; i < n; ++i)
            {
                ++result;
            }
        });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    printf("Undefined behavior: result=%lld, expected=%lld\n",
        result, static_cast<long long>(num_workers) * n);
}
