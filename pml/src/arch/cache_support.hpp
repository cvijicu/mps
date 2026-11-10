#ifndef PML_CACHE_SUPPORT_HPP
#define PML_CACHE_SUPPORT_HPP

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace cache_experiment
{
inline void fail(const std::string& message)
{
    std::cerr << "Error: " << message << '\n';
    std::exit(1);
}

inline int number(const std::string& text, int maximum)
{
    int value = 0;
    auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() ||
        value < 1 || value > maximum)
    {
        fail("Expected an integer in [1, " + std::to_string(maximum) + "]");
    }
    return value;
}

inline void summary(const std::string& name, const std::vector<double>& times)
{
    double total = 0;
    for (double value : times)
    {
        total += value;
    }
    std::cout << name << " mean_s=" << total / times.size()
              << " min_s=" << *std::min_element(times.begin(), times.end())
              << " max_s=" << *std::max_element(times.begin(), times.end()) << '\n';
}

inline void elapsed_ok(double seconds)
{
    if (!std::isfinite(seconds) || seconds <= 0)
    {
        fail("Nonpositive elapsed time; increase the amount of work");
    }
}

inline std::ofstream csv_file(const std::string& path)
{
    std::ofstream csv(path);
    if (!csv)
    {
        fail("Cannot open CSV: " + path);
    }
    csv << std::setprecision(17);
    csv << "method,workload,n,workers,repetition,position,time_s,checksum,elements,"
           "element_bytes,line_size,counter_size,lock_free,shared_lines\n";
    return csv;
}

inline void csv_ok(const std::ofstream& csv)
{
    if (!csv)
    {
        fail("Failed to write measurement CSV");
    }
}
}
#endif
