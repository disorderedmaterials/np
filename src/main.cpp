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
    ModEx modex(config);

    if (config.useDefinedPulses) {
        modex.epochPulses(config.definedPulses);
        std::set<std::string> labels;
        std::vector<std::pair<double, double>> pulses;
        std::map<std::string, std::vector<std::pair<double, double>>> runPulses;
        for (const auto &p : config.definedPulses) {
            labels.insert(p.label);
        }
            
        for (const auto &label : labels) {
            for (const auto &p : config.definedPulses) {
                if (p.label == label) {
                    pulses.push_back(std::make_pair(p.pulseStart, p.pulseEnd));
                }
            }
            modex.binPulsesToRuns(pulses, runPulses);
            modex.run(runPulses, label);
            pulses.clear();
            runPulses.clear();
        }
    }

    else {
        std::vector<std::pair<double, double>> pulses;
        std::map<std::string, std::vector<std::pair<double, double>>> runPulses;
        for (const auto &p : config.period.pulses) {
            modex.extrapolatePulseTimes(
                modex.runs[0],
                config.periodBegin,
                config.extrapolationMode == BACKWARDS || config.extrapolationMode == BI_DIRECTIONAL,
                config.extrapolationMode == FORWARDS || config.extrapolationMode == BI_DIRECTIONAL,
                config.period.duration,
                p.periodOffset,
                p.duration,
                pulses
            );
            modex.binPulsesToRuns(pulses, runPulses);
            modex.run(runPulses, p.label);
            pulses.clear();
            runPulses.clear();
        }
    }

    return 0;

}