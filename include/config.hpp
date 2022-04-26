#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include "period_definition.hpp"
#include "pulse_definition.hpp"
#include "pulse.hpp"

enum ExtrapolationMode {FORWARDS, BACKWARDS, BI_DIRECTIONAL, NONE};

class Config {

    public:
        std::string gudrunInputFile; //input file for gudrun.
        std::string purgeInputFile; //input file for purge.
        std::string outputDir; //where to output data
        std::string dataFileDir; // data file directory.
        std::vector<std::string> runs; // path to runs.
        ExtrapolationMode extrapolationMode;
        // std::vector<Pulse> definedPulses;
        double periodBegin;
        PeriodDefinition period;
        std::vector<Pulse> pulses;

        Config(std::string path);
        Config() = default;

};


#endif