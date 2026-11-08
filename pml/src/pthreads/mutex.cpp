#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

using namespace std;

void* add(void* rank);

const int num_workers = 5;
int res = 0;
pthread_mutex_t mutex;

int main()
{
    pthread_t threads[num_workers];

    pthread_mutex_init(&mutex, NULL);
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

    pthread_mutex_destroy(&mutex);
    printf("Result: %d\n", res);
    return 0;
}

void* add(void*)
{
    printf("Hello.\n");
    for (int i = 0; i != 10000; ++i)
    {
        pthread_mutex_lock(&mutex);
        res += 1;
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}
