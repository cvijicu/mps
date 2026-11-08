#include "kernels.hpp"
#include <cstdlib>
#include <iostream>

using namespace std;

void moving_average_direct(const Samples&, int, Samples&)
{
    // TODO: compute each window independently.
    cerr << "Error: complete the TODO in average/sequential.cpp (moving_average_direct).\n";
    exit(1);
}

void moving_average_sliding(const Samples&, int, Samples&)
{
    // TODO: reuse the previous window's sum.
    cerr << "Error: complete the TODO in average/sequential.cpp (moving_average_sliding).\n";
    exit(1);
}
