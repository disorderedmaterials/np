#include "modex.hpp"
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <iomanip>

ModEx::ModEx(Config cfg) {
    input = cfg.gudrunInputFile;
    purge = cfg.purgeInputFile;
    out = cfg.outputDir;
    dataDir = cfg.dataFileDir;
    runs = cfg.runs;
    truncatePath = cfg.runs[0];
    Nexus nxs(dataDir + "/" + truncatePath);
    nxs.loadBasicData();
    expStart = nxs.startSinceEpoch;
}

bool ModEx::run() {
    return true;
}

bool ModEx::run(std::vector<Pulse> &pulses, std::string pulseLabel) {
    // Perform a purge.
    system(std::string("./purge_det " + purge + "> /dev/null").c_str());

    // Iterate through pulses.
    for (Pulse &pulse : pulses) {
        
        std::size_t dot = truncatePath.rfind('.');
        std::string baseName = truncatePath.substr(0, dot);
        std::cout << truncatePath << std::endl;
        std::cout << baseName << std::endl;
        Nexus *nxs;
        if (pulse.startRun == pulse.endRun) {
            // Allocate a new Nexus objecs on the heap.
            std::cout << dataDir + "/" + baseName + ".nxs" << std::endl;
            nxs = new Nexus(dataDir + "/" + pulse.startRun, dataDir + "/" + baseName + ".nxs");
            // Load basic data.
            nxs->loadBasicData();
            // Load event mode data.
            nxs->loadEventModeData();
            // Create histogram using the pulse as boundaries.
            Pulse normPulse(pulseLabel, pulse.start-nxs->startSinceEpoch, pulse.end-nxs->startSinceEpoch);
            nxs->createHistogram(normPulse);
            // Write the histogram.
            nxs->writeCountsHistogram();
        }
        else {
            std::cout << currentPulse << " " << totalPulses << std::endl;
            progress = ((double) currentPulse / (double) totalPulses) * 100;
            std::cout << "Progress: " << progress << "%" << std::endl;
            ++currentPulse;
            continue;
        }
        // Call gudrun_dcs.
        system(std::string("mkdir modex_intermediate && cd modex_intermediate && ../gudrun_dcs ../" + input + "> /dev/null").c_str());
        // Move the mint01 file to the output directory.
        std::string output = out + "/" + std::to_string(pulse.start-expStart) + "-" + pulseLabel + ".mint01";
        std::string target = "modex_intermediate/" + baseName + ".mint01";
        system(std::string("mv " + target + " " + output).c_str());
        system("rm -rf modex_intermediate");
        // system(("rm " + dataDir + "/" + baseName + ".nxs").c_str());
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
    Nexus firstRunNXS(dataDir + "/" + firstRun);
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
    Nexus firstRunNXS(dataDir + "/" + firstRun);
    firstRunNXS.loadBasicData();
    Nexus lastRunNXS(dataDir + "/" + lastRun);
    lastRunNXS.loadBasicData();
    Nexus startRunNXS(dataDir + "/" + start_run);
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
        for (int j=0; j<runs.size(); ++j) {
            Nexus *runNXS = new Nexus(dataDir + "/" + runs[j]); // Heap allocation.
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