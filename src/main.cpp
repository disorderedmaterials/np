#include "nexus.hpp"
#include "modex.hpp"
#include "config.hpp"
#include <iostream>
#include <map>
#include <vector>

int main(int argc, char** argv) {

    if (argc < 2) {
        std::cout << "Configuration file not provided." << std::endl;
        std::cout << "Usage: modulation_excitation {config.cfg}" << std::endl;
        return -1;
    }
    Config config(argv[1]);
    ModEx modex(config);

    if (!config.useDefinedPulses) {
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
    // if (config.extrapolationMode == FORWARDS) {

    // }

    // Config config("input.txt");

    // std::cout << config.gudrunInputFile << std::endl;
    // std::cout << config.purgeInputFile << std::endl;
    // for (auto r: config.runs)
    //     std::cout << r << std::endl;
    // std::cout << config.extrapolationMode << std::endl;
    // std::cout << config.useDefinedPulses << std::endl;
    // std::cout << config.pulsesBegin << std::endl;
    // std::cout << config.period.duration << std::endl;
    // for (auto p : config.period.pulses)
    //     std::cout << p.label << " " << p.periodOffset << " " << p.duration << std::endl;
    // return 0;
    // std::vector<std::string> runs = {"data/NIMROD00069862.nxs"};
    // ModEx m("azobenzene.txt", "purge_det.dat", "testout", runs);
    // std::map<std::string, std::vector<std::pair<double, double>>> pulses;
    // m.extrapolatePulseTimes(
    //     runs[0], 12129., false, true, 980., 300., pulses
    // );
    // m.run(pulses);

}