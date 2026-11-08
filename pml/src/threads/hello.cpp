#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

using namespace std;

void worker(int id)
{
    printf("Hello from worker %d\n", id);
}

int main(int argc, char** argv)
{
    const int num_workers = argc > 1 ? atoi(argv[1]) : 4;
    if (num_workers <= 0)
    {
        return 1;
    }

    vector<thread> threads;
    threads.reserve(num_workers);

    for (int t = 0; t < num_workers; ++t)
    {
        threads.emplace_back(worker, t);
    }

    printf("This is main thread\n");

    for (auto& t : threads)
    {
        t.join();
    }
}
