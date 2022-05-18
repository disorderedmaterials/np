#ifndef MODEX_H
#define MODEX_H

#include "nexus.hpp"
#include "config.hpp"
#include <vector>
#include <fstream>

class ModEx {

    private:
        int currentPulse = 1;
    public:
        Config cfg;
        std::string out;
        std::string dataDir;
        std::string diagnosticPath = "modex.diagnostics";
        std::ofstream diagnosticFile;
        int expStart;
        double progress;
        int totalPulses = 0;

        ModEx(Config cfg_);
        ModEx() = default;
        ~ModEx();

        bool process();
        bool processPulse(Pulse &pulse);
        bool epochPulses(std::vector<Pulse> &pulses);
        bool extrapolatePulseTimes(std::string start_run, double start, bool backwards, bool forwards, double periodDuration, PulseDefinition pulseDefinition, std::vector<Pulse> &pulses);
        bool binPulsesToRuns(std::vector<Pulse> &pulses);
};

#endif // MODEX_H