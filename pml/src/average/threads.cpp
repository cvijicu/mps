#include "kernels.hpp"
#include <cstdlib>
#include <iostream>
#include <thread>

using namespace std;

void average_block(const Samples& x, int w, Samples& y, int begin, int end)
{
    for (int i = begin; i < end; ++i) {
        double sum = 0.0;
        for (int j = 0; j < w; ++j) {
            sum += x[i + j];
        }
        y[i] = sum / w;
    }
}

void moving_average_threads(const Samples& x, int w, Samples& y, int num_workers)
{
    // TODO: partition the OUTPUT indices, compute windows, join every worker.
    int m = y.size();
    vector<thread> threads;
    threads.reserve(num_workers);

    for (int t = 0; t < num_workers; ++t) {
        int begin = (long long)m * t / num_workers;
        int end = (long long)m * (t + 1) / num_workers;
        threads.emplace_back(average_block, std::cref(x), w, std::ref(y), begin, end);
    }
}
