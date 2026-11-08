#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

using namespace std;

void* add(void* rank);

const int num_workers = 5;
int res = 0;

int main()
{
    pthread_t threads[num_workers];

    for (int thread = 0; thread != num_workers; ++thread)
    {
        pthread_create(&threads[thread], NULL,
            add, NULL);
    }

    printf("This is main thread\n");

    for (int thread = 0; thread != num_workers; ++thread)
    {
        pthread_join(threads[thread], NULL);
    }

    printf("Result: %d\n", res);
    return 0;
}

void* add(void*)
{
    printf("Hello.\n");
    for (int i = 0; i != 10000; ++i)
    {
        res += 1;
    }
    return NULL;
}
