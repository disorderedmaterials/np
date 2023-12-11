#ifndef MODEX_H
#define MODEX_H

#include "config.h"
#include "nexus.h"
#include "period.h"
#include "pulse.h"
#include <fstream>
#include <vector>

class ModEx
{

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
    bool extrapolatePeriods(std::vector<Period> &periods);
    bool createSuperPeriod(Period &period, int nSlices);
    bool processPeriod(Period &period);
    bool binPulsesToRuns(std::vector<Pulse> &pulses);
    bool binPeriodsToRuns(std::vector<Period> &periods);
};

#endif // MODEX_H
