#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char** argv)
{
    size_t n = (argc > 1) ? strtoull(argv[1], nullptr, 10) : 20'000'000;
    vector<double> a(n);
    for (size_t i = 0; i < n; ++i)
    {
        a[i] = 0.001 * (i % 1000);
    }

    const auto t0 = chrono::steady_clock::now();
    double sum = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        sum += sin(a[i]) * sin(a[i]) + cos(a[i]) * cos(a[i]);
    }
    const auto t1 = chrono::steady_clock::now();

    const double elapsed = chrono::duration<double>(t1 - t0).count();
    cout << "n=" << n << " result=" << sum << " time_s=" << elapsed << "\n";
}
