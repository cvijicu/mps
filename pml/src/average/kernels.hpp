#pragma once
#include <vector>

using Samples = std::vector<double>;

void moving_average_direct(const Samples& x, int w, Samples& y);
void moving_average_sliding(const Samples& x, int w, Samples& y);
void moving_average_threads(const Samples& x, int w, Samples& y, int num_workers);
int moving_average_openmp(const Samples& x, const std::vector<int>& widths,
    Samples& y);
