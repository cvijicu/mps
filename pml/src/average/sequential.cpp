#include "kernels.hpp"
#include <cstdlib>
#include <iostream>

using namespace std;

void moving_average_direct(const Samples& x, int w, Samples& y)
{
    // TODO: compute each window independently.
    int m = y.size();
    
    for (int i = 0; i < m; ++i) {
        double sum = 0.0;
        for (int j = 0; j < w; ++j) {
            sum += x[i + j];
        }
        y[i] = sum / w;
    }
}

void moving_average_sliding(const Samples& x, int w, Samples& y)
{
    // TODO: reuse the previous window's sum.
    int m = y.size();
    double sum = 0.0;

    for (int i = 0; i < w; ++i) {
        sum += x[i];
    }

    y[0] = sum / w;

    for (int i = 1; i < m; ++i) {
        sum = sum - x[i - 1] + x[i + w - 1];
        y[i] = sum / w;
    }
}
