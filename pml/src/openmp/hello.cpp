#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

using namespace std;

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s THREADS\n", argv[0]);
        return 1;
    }
    char* end = nullptr;
    long threads = strtol(argv[1], &end, 10);
    if (*end != '\0' || threads <= 0 || threads > 1024)
    {
        fprintf(stderr, "THREADS must be in [1, 1024]\n");
        return 1;
    }
    int requested_num_workers = static_cast<int>(threads);

#pragma omp parallel num_threads(requested_num_workers)
    {
        int worker_id = omp_get_thread_num();
        int tmp = omp_get_num_threads();

        printf("Hello from thread %d of %d threads.\n", worker_id, tmp);
    }

    return 0;
}
