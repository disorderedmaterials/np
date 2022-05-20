#include <period.hpp>


bool Period::isComplete() {
    for (auto &p : pulses) {
        if (p.startRun.empty() || p.endRun.empty())
            return false;
    }
    return true;
}