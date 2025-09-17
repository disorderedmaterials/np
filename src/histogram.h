#pragma once

#include <vector>

class IntegerHistogram
{
    public:
    IntegerHistogram() = default;
    ~IntegerHistogram() = default;

    private:
    // Bins
    std::vector<long int> bins_;
    // Boundaries
    std::vector<double> boundaries_;

    public:
    // Initialise
    void initialise(const std::vector<double> &boundaries);
    // Increment relevant bin
    void bin(double x);
    // Add to specific bin
    void add(int index, long int value);
    // Return specified bin value
    long int value(int index) const;
    // Return the sum of all bins
    long int sum() const;
    // Scale bin values
    void scale(double factor);
};
