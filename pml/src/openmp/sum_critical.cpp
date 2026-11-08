#include <omp.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <climits>

using namespace std;

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s N\n", argv[0]);
        return 1;
    }
    char* end = nullptr;
    const long n = strtol(argv[1], &end, 10);
    if (end == argv[1] || *end != '\0' || n < 0 || n > INT_MAX)
    {
        fprintf(stderr, "N must be in [0, INT_MAX]\n");
        return 1;
    }
    long long sum = 0;

#pragma omp parallel for
    for (long long i = 1; i <= n; ++i)
    {
#pragma omp critical
        sum += i;
    }

    const long long expected = n * (n + 1) / 2;
    if (sum != expected)
    {
        return 1;
    }
    cout << "Sum is " << sum << " .\n";
    return 0;
}
