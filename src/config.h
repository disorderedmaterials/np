#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include "period.h"
#include "pulse_definition.h"
#include "pulse.h"

enum ExtrapolationMode {FORWARDS, BACKWARDS, BI_DIRECTIONAL, FORWARDS_SUMMED, NONE};

class Config {

    public:
        std::string path;
        std::string outputDir; //where to output data
        std::vector<std::string> runs; // abs path to runs.
        std::vector<std::string> nxsDefinitionPaths;
        ExtrapolationMode extrapolationMode;
        double periodBegin;
        int summedNSlices;
        PeriodDefinition periodDefinition;
        std::vector<Pulse> rawPulses;
        Config(std::string path_) : path(path_) {}
        Config() = default;

        bool parse();

};


#endif // CONFIG_H
