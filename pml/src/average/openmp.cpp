#include "kernels.hpp"
#include <omp.h>
#include <cstdlib>
#include <iostream>

using namespace std;

int moving_average_openmp(const Samples& x, const vector<int>& w, Samples& y)
{
    int num_workers = 1;
    int m = y.size();

    #pragma omp parallel shared(num_workers)
    {
        // Jedna nit belezi stvarnu velicinu tima
        #pragma omp single
        num_workers = omp_get_num_threads();

        // Iteracije spoljasnje petlje se dele izmedju niti prema OMP_SCHEDULE / zadatom rasporedu
        #pragma omp for schedule(runtime)
        for (int i = 0; i < m; ++i)
        {
            double sum = 0.0; // Lokalna promenljiva za sumu, svaka nit ima svoju kopiju
            for (int j = 0; j < w[i]; ++j)
            {
                sum += x[i + j];
            }
            y[i] = sum / w[i]; // Svaki izlaz upisuje tacno jedna nit, tako da nema potrebe za dodatnom sinhronizacijom
        }
    }
}
