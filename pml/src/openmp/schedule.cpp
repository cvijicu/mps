#include <iostream>
#include <omp.h>
#include <vector>

using namespace std;

int main()
{
    vector<int> owner(12, -1);
    int num_workers = 0;
    omp_set_dynamic(0);

#pragma omp parallel shared(owner, num_workers)
    {
#pragma omp single
        num_workers = omp_get_num_threads();

#pragma omp for schedule(runtime)
        for (int i = 0; i < 12; ++i)
        {
            owner[i] = omp_get_thread_num();
        }
    }

    cout << "workers=" << num_workers << '\n';
    for (int i = 0; i < 12; ++i)
    {
        cout << i << ": " << owner[i] << '\n';
    }
}
