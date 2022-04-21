#include "modex.hpp"
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <iomanip>

ModEx::ModEx(Config cfg) {
    input = cfg.gudrunInputFile;
    purge = cfg.purgeInputFile;
    out = cfg.outputDir;
    runs = cfg.runs;
}

bool ModEx::run() {
    return true;
}

bool ModEx::run(std::vector<Pulse> &pulses, std::string pulseLabel) {
    // Perform a purge.
    system(std::string("./purge_det " + purge + "> /dev/null").c_str());

    // Iterate through pulses.
    for (Pulse &pulse : pulses) {
        
        // Output path = test/{runName}.nxs
        std::size_t sep = pulse.startRun.rfind('/');
        std::size_t dot = pulse.startRun.rfind('.');
        std::string baseName = pulse.startRun.substr(sep+1, dot-sep-1);
        // Allocate a new Nexus objecs on the heap.
        Nexus *nxs;
        if (pulse.startRun == pulse.endRun) {
            nxs = new Nexus(pulse.startRun, "test/" + baseName + ".nxs");
            // Load basic data.
            nxs->loadBasicData();
            // Load event mode data.
            nxs->loadEventModeData();
            // Create histogram using the pulse as boundaries.
            nxs->createHistogram(pulse);
            // Write the histogram.
            nxs->writeCountsHistogram();
        }
        // Call gudrun_dcs.
        system(std::string("mkdir modex_intermediate && cd modex_intermediate && ../gudrun_dcs ../" + input + "> /dev/null").c_str());
        // Move the mint01 file to the output directory.
        std::string output = out + "/" + std::to_string(pulse.start-nxs->startSinceEpoch) + "-" + pulseLabel + ".mint01";
        std::string target = "modex_intermediate/" + baseName + ".mint01";
        system(std::string("mv " + target + " " + output).c_str());
        system("rm -rf modex_intermediate");
        delete nxs;
        std::cout << currentPulse << " " << totalPulses << std::endl;
        progress = ((double) currentPulse / (double) totalPulses) * 100;
        std::cout << "Progress: " << progress << "%" << std::endl;
        ++currentPulse;
    }
    return true;
}

bool ModEx::epochPulses(std::vector<Pulse> &pulses) {

    // Assume runs are ordered.
    const std::string firstRun = runs[0];
    // Load the first run.
    Nexus firstRunNXS(firstRun);
    firstRunNXS.loadBasicData();

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
    const std::string firstRun = runs[0];
    const std::string lastRun = runs[runs.size()-1];

    // Load the first, last and start runs.
    Nexus firstRunNXS(firstRun);
    firstRunNXS.loadBasicData();
    Nexus lastRunNXS(lastRun);
    lastRunNXS.loadBasicData();
    Nexus startRunNXS(start_run);
    startRunNXS.loadBasicData();

    // Determine start, end and first pulse times, since unix epoch.
    const int expStart = firstRunNXS.startSinceEpoch;
    const int expEnd = lastRunNXS.endSinceEpoch;
    double startPulse = startRunNXS.startSinceEpoch + start + pulseDefinition.periodOffset;
    double pulse = 0;
    // pulses.push_back(std::make_pair(startPulse, startPulse+pulseDefinition.duration));
    pulses.push_back(Pulse(pulseDefinition.label, startPulse, startPulse+pulseDefinition.duration));

    // Extrapolate backwardsa.
    if (backwards) {
        pulse = startPulse - periodDuration;
        while (pulse > expStart) {
            pulses.push_back(Pulse(pulseDefinition.label, pulse, startPulse+pulseDefinition.duration));
            pulse-=periodDuration;
        }
    }

    // Extrapolate forwards.
    if (forwards) {
        pulse = startPulse + periodDuration;
        while (pulse < expEnd) {
            pulses.push_back(Pulse(pulseDefinition.label, pulse, startPulse+pulseDefinition.duration));
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
        for (int j=0; j<runs.size(); ++j) {
            Nexus *runNXS = new Nexus(runs[j]); // Heap allocation.
            runNXS->loadBasicData();
            // Find start and end run.
            if ((pulses[i].start >= runNXS->startSinceEpoch) && (pulses[i].start<=runNXS->endSinceEpoch)) {
                pulses[i].startRun = runs[j];
            }
            if ((pulses[i].end >= runNXS->startSinceEpoch) && (pulses[i].end<=runNXS->endSinceEpoch)) {
                pulses[i].endRun = runs[j];
                break;
            }
            delete runNXS;
        }
    }
    return true;
}