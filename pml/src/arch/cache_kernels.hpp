#ifndef PML_CACHE_KERNELS_HPP
#define PML_CACHE_KERNELS_HPP

#include <vector>

using Matrix = std::vector<int>;
using Sum = long long;

Sum sum_rows(const Matrix& a, int n);
Sum sum_columns(const Matrix& a, int n);

#endif
