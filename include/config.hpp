#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include "period.hpp"
#include "pulse.hpp"

enum ExtrapolationMode {FORWARDS, BACKWARDS, BI_DIRECTIONAL, NONE};

class Config {

    public:
        std::string gudrunInputFile; //input file for gudrun.
        std::string purgeInputFile; //input file for purge.
        std::string outputDir; //where to output data
        std::vector<std::string> runs; //abspath to runs.
        ExtrapolationMode extrapolationMode;
        bool useDefinedPulses = false;
        std::vector<Pulse> definedPulses;
        double periodBegin;
        Period period;

        Config(std::string path);
        Config() = default;

};


#endif