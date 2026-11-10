#include "cache_kernels.hpp"
#include <cstdlib>
#include <iostream>

using namespace std;

Sum sum_rows(const Matrix&, int)
{
    // TODO: visit rows; the inner loop changes the column index.
    cerr << "Error: complete sum_rows in arch/cache_kernels.cpp.\n";
    exit(1);
}

Sum sum_columns(const Matrix&, int)
{
    // TODO: visit columns; the inner loop changes the row index.
    cerr << "Error: complete sum_columns in arch/cache_kernels.cpp.\n";
    exit(1);
}
