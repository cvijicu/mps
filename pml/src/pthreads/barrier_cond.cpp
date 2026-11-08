#include <cstdio>
#include <pthread.h>

using namespace std;

const int num_workers = 5;
int arrived = 0;
pthread_mutex_t mutex;
pthread_cond_t cv;

void* thread_job(void*)
{
    pthread_mutex_lock(&mutex);
    ++arrived;
    if (arrived == num_workers)
    {
        pthread_cond_broadcast(&cv);
    }
    else
    {
        while (arrived < num_workers)
        {
            pthread_cond_wait(&cv, &mutex);
        }
    }
    pthread_mutex_unlock(&mutex);

    printf("Thread code after barrier.\n");
    return nullptr;
}

int main()
{
    pthread_t threads[num_workers];
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&cv, nullptr);

    for (int t = 0; t < num_workers; ++t)
    {
        pthread_create(&threads[t], nullptr, thread_job, nullptr);
    }
    for (int t = 0; t < num_workers; ++t)
    {
        pthread_join(threads[t], nullptr);
    }

    pthread_cond_destroy(&cv);
    pthread_mutex_destroy(&mutex);
}
