#include "kernels.hpp"
#include <omp.h>
#include <cstdlib>
#include <iostream>

using namespace std;

int moving_average_openmp(const Samples&, const vector<int>&,
    Samples&)
{
    // TODO: use one parallel region, record its team size, distribute outputs
    // TODO: use omp for schedule(runtime) and keep each window sum local.
    cerr << "Error: complete the TODO in average/openmp.cpp (moving_average_openmp).\n";
    exit(1);
}
