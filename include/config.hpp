#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

enum ExtrapolationMode {FORWARDS, BACKWARDS, BI_DIRECTIONAL, NONE};

class Config {

    public:
        std::string gudrunInputFile; //input file for gudrun.
        std::string purgeInputFile; //input file for purge.
        std::vector<std::string> runs; //abspath to runs.
        ExtrapolationMode extrapolationMode;
        bool useDefinedPulses = false;
        std::vector<std::pair<std::string, std::pair<double, double>>> pulses;
        double pulsesBegin;
        

};


#endif