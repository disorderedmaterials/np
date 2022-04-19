#include "nexus.hpp"
#include "modex.hpp"
#include "config.hpp"
#include <iostream>
#include <map>
#include <vector>
#include <set>

int main(int argc, char** argv) {

    if (argc < 2) {
        std::cout << "Configuration file not provided." << std::endl;
        std::cout << "Usage: modulation_excitation {config.cfg}" << std::endl;
        return -1;
    }
    Config config(argv[1]);
    // return 1;
    ModEx modex(config);

    if (config.extrapolationMode == NONE) {
        modex.epochPulses(config.pulses);
        std::set<std::string> labels;
        std::vector<Pulse> pulses;
        for (const auto &p : config.pulses) {
            labels.insert(p.label);
        }
            
        for (const auto &label : labels) {
            for (const auto &p : config.pulses) {
                if (p.label == label) {
                    pulses.push_back(p);
                }
            }
            modex.binPulsesToRuns(pulses);
            modex.run(pulses, label);
            pulses.clear();
        }
    }

    else {
        // std::vector<std::pair<double, double>> pulses;
        std::vector<Pulse> pulses;
        for (const auto &p : config.period.pulses) {
            modex.extrapolatePulseTimes(
                modex.runs[0],
                config.periodBegin,
                config.extrapolationMode == BACKWARDS || config.extrapolationMode == BI_DIRECTIONAL,
                config.extrapolationMode == FORWARDS || config.extrapolationMode == BI_DIRECTIONAL,
                config.period.duration,
                p,
                pulses
            );
            modex.binPulsesToRuns(pulses);
            modex.run(pulses, p.label);
            pulses.clear();
        }
    }

    return 0;

}