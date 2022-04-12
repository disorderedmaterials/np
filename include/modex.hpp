#ifndef MODEX_H
#define MODEX_H

#include "nexus.hpp"
#include "config.hpp"
#include <vector>

class ModEx {

    public:
        std::string input;
        std::string purge;
        std::string out;
        std::vector<std::string> runs;

        ModEx(std::string input_, std::string purge_, std::string out_, std::vector<std::string> runs_) : input(input_), purge(purge_), out(out_), runs(runs_) {}
        ModEx(Config cfg);
        ModEx() = default;
        
        bool run();
        bool run(std::string run, std::vector<std::pair<double, double>> pulses);
        bool run(std::string run, std::pair<double, double> pulses);
        bool run(std::map<std::string, std::vector<std::pair<double, double>>> &runPulses, std::string pulseLabel);
        bool epochPulses(std::vector<Pulse> &pulses);
        bool extrapolatePulseTimes(std::string start_run, double start, bool backwards, bool forwards, double periodDuration, double periodOffset, double duration, std::vector<std::pair<double, double>> &pulses);
        bool binPulsesToRuns(std::vector<std::pair<double, double>> &pulses, std::map<std::string, std::vector<std::pair<double, double>>> &runPulses);
};

#endif