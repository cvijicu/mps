#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <pthread.h>

using namespace std;

void* hello(void* arg)
{
    const auto rank = static_cast<long>(reinterpret_cast<intptr_t>(arg));
    printf("Hello from thread %ld\n", rank);
    return nullptr;
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s THREADS\n", argv[0]);
        return 1;
    }
    const int num_workers = atoi(argv[1]);
    if (num_workers <= 0)
    {
        fprintf(stderr, "THREADS must be positive\n");
        return 1;
    }
    pthread_t* threads = new pthread_t[num_workers];

    for (int t = 0; t < num_workers; ++t)
    {
        pthread_create(&threads[t], nullptr, hello,
            reinterpret_cast<void*>(static_cast<intptr_t>(t)));
    }

    printf("This is main thread\n");
    for (int t = 0; t < num_workers; ++t)
    {
        pthread_join(threads[t], nullptr);
    }

    delete [] threads;
}
