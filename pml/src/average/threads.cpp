#include "kernels.hpp"
#include <cstdlib>
#include <iostream>
#include <thread>

using namespace std;

void moving_average_threads(const Samples&, int,
    Samples&, int)
{
    // TODO: partition the OUTPUT indices, compute windows, join every worker.
    cerr << "Error: complete the TODO in average/threads.cpp (moving_average_threads).\n";
    exit(1);
}
