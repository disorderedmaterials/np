#include "histogram.h"
#include <algorithm>
#include <numeric>

// Initialise
void IntegerHistogram::initialise(const std::vector<double> &boundaries)
{
    boundaries_ = boundaries;
    bins_.resize(boundaries_.size() - 1);
    std::fill(bins_.begin(), bins_.end(), 0);
}

// Increment relevant bin
void IntegerHistogram::bin(double x)
{
    // Out of initial range?
    if (x < boundaries_.front())
        return;

    // Find relevant bin
    for (auto i = 1; i < boundaries_.size(); ++i)
        if (x < boundaries_[i])
        {
            ++bins_[i - 1];
            return;
        }
}

// Add to specific bin
void IntegerHistogram::add(int index, long int value) { bins_[index] += value; }

// Return specified bin value
long int IntegerHistogram::value(int index) const { return bins_[index]; }

// Return the sum of all bins
long int IntegerHistogram::sum() const { return std::accumulate(bins_.begin(), bins_.end(), (long int)0); }

// Scale bin values
void IntegerHistogram::scale(double factor)
{
    std::transform(bins_.begin(), bins_.end(), bins_.begin(), [factor](const long int value) { return value * factor; });
}
