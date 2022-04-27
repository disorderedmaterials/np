#include <algorithm>
#include <iostream>
#include <modex.hpp>
#include <nexus.hpp>

bool ModEx::process() {
    if (cfg.extrapolationMode == NONE) {
        ;
    }
    else {
        for (auto &p : cfg.period.pulses) {
            std::vector<Pulse> pulses;
            extrapolatePulseTimes(
                cfg.runs[0],
                cfg.periodBegin,
                cfg.extrapolationMode == BACKWARDS || cfg.extrapolationMode == BI_DIRECTIONAL,
                cfg.extrapolationMode == FORWARDS || cfg.extrapolationMode == BI_DIRECTIONAL,
                cfg.period.duration,
                p,
                pulses
            );
            binPulsesToRuns(pulses);
            for (Pulse &pulse : pulses) {
                Nexus *nxs;
                if (pulse.startRun == pulse.endRun) {
                    nxs = new Nexus(pulse.startRun, cfg.outputDir + "/" + std::to_string((int) pulse.start) + ".nxs");
                    if (!nxs->load(true))
                        return false;
                    if (!nxs->createHistogram(pulse, nxs->startSinceEpoch))
                        return false;
                    if (!nxs->output(cfg.nxsDefinitionPaths))
                        return false;
                    delete nxs;
                }
                
            }
        }
    }
    return true;

}

bool ModEx::epochPulses(std::vector<Pulse> &pulses) {

    // Assume runs are ordered.
    std::string firstRun = cfg.runs[0];
    // Load the first run.
    Nexus firstRunNXS(firstRun);
    firstRunNXS.load();

    // Apply offset.

    const int expStart = firstRunNXS.startSinceEpoch;

    for (int i=0; i<pulses.size(); ++i) {
        pulses[i].start+= expStart;
        pulses[i].end+= expStart;
    }

    return true;

}

bool ModEx::extrapolatePulseTimes(std::string start_run, double start, bool backwards, bool forwards, double periodDuration, PulseDefinition pulseDefinition, std::vector<Pulse> &pulses) {

    // Assume runs are ordered.
    const std::string firstRun = cfg.runs[0];
    const std::string lastRun = cfg.runs[cfg.runs.size()-1];

    // Load the first, last and start runs.
    Nexus firstRunNXS(firstRun);
    firstRunNXS.load();
    Nexus lastRunNXS(lastRun);
    lastRunNXS.load();
    Nexus startRunNXS(start_run);
    startRunNXS.load();
    // Determine start, end and first pulse times, since unix epoch.
    const int expStart = firstRunNXS.startSinceEpoch;
    const int expEnd = lastRunNXS.endSinceEpoch;
    double startPulse = startRunNXS.startSinceEpoch + start + pulseDefinition.periodOffset;
    double pulse = 0;
    // pulses.push_back(std::make_pair(startPulse, startPulse+pulseDefinition.duration));
    pulses.push_back(Pulse(pulseDefinition.label, startPulse, startPulse+pulseDefinition.duration));

    // Extrapolate backwards.
    if (backwards) {
        pulse = startPulse - periodDuration;
        while (pulse > expStart) {
            pulses.push_back(Pulse(pulseDefinition.label, pulse, pulse+pulseDefinition.duration));
            pulse-=periodDuration;
        }
    }

    // Extrapolate forwards.
    if (forwards) {
        pulse = startPulse + periodDuration;
        while (pulse < expEnd) {
            pulses.push_back(Pulse(pulseDefinition.label, pulse, pulse+pulseDefinition.duration));
            pulse+=periodDuration;
        }
    }

    // Sort pulses by their start time.
    std::sort(
        pulses.begin(), pulses.end(),
        [](
            const Pulse a,
            const Pulse b
            ){
                return a.start < b.end;
            }
    );
    return true;
}

bool ModEx::binPulsesToRuns(std::vector<Pulse> &pulses) {

    for (int i=0; i<pulses.size(); ++i) {
        for (int j=0; j<cfg.runs.size(); ++j) {
            Nexus *runNXS = new Nexus(cfg.runs[j]); // Heap allocation.
            runNXS->load();
            // Find start and end run.
            if ((pulses[i].start >= runNXS->startSinceEpoch) && (pulses[i].start<=runNXS->endSinceEpoch)) {
                pulses[i].startRun = cfg.runs[j];
            }
            if ((pulses[i].end >= runNXS->startSinceEpoch) && (pulses[i].end<=runNXS->endSinceEpoch)) {
                pulses[i].endRun = cfg.runs[j];
                break;
            }
            delete runNXS;
        }
    }
    return true;
}