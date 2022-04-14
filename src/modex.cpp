#include "modex.hpp"
#include <cstdlib>
#include <algorithm>
#include <iostream>

ModEx::ModEx(Config cfg) {
    input = cfg.gudrunInputFile;
    purge = cfg.purgeInputFile;
    out = cfg.outputDir;
    runs = cfg.runs;
}

bool ModEx::run() {
    return true;
}

bool ModEx::run(std::map<std::string, std::vector<Pulse>> &runPulses, std::string pulseLabel) {
    // Perform a purge.
    system(std::string("./purge_det " + purge + "> /dev/null").c_str());
    // Iterate through [run, pulse] pairs.

    int n=0;
    int m = 0;
    for (const auto runPair : runPulses)
        n+=runPair.second.size();

    for (const auto runPair : runPulses) {
        std::string run = runPair.first;
        std::vector<Pulse> pulses = runPair.second;

        // Iterate through pulses.
        for (auto p : pulses) {
            // Output path = test/{runName}.nxs
            std::size_t sep = run.rfind('/');
            std::size_t dot = run.rfind('.');
            std::string baseName = run.substr(sep+1, dot-sep-1);
            // Allocate a new Nexus objecs on the heap.
            Nexus *nxs = new Nexus(run, "test/" + baseName + ".nxs");
            // Load basic data.
            nxs->loadBasicData();
            // Load event mode data.
            nxs->loadEventModeData();
            // Create histogram using the pulse as boundaries.
            nxs->createHistogram(p);
            // Write the histogram.
            nxs->writeCountsHistogram();
            // Call gudrun_dcs.
            system(std::string("mkdir modex_intermediate && chdir modex_intermediate && ../gudrun_dcs ../" + input + " > /dev/null").c_str());
            // Move the mint01 file to the output directory.
            std::string output = out + "/" + std::to_string(p.start) + "-" + pulseLabel + ".mint01";
            std::string target = "modex_intermediate/" + baseName + ".mint01";
            system(std::string("mv " + target + " " + output).c_str());
            system("rm -rf modex_intermediate");
            delete nxs;
            ++m;
        }
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

bool ModEx::binPulsesToRuns(std::vector<Pulse> &pulses, std::map<std::string, std::vector<Pulse>> &runPulses) {

    for (int i=0; i<pulses.size(); ++i) {
        for (int j=0; j<runs.size(); ++j) {
            Nexus *runNXS = new Nexus(runs[j]); // Heap allocation.
            runNXS->loadBasicData();
            // Pulse start is between the start and end of the run.
            if ((pulses[i].start>=runNXS->startSinceEpoch) && (pulses[i].end<=runNXS->endSinceEpoch)) {
                // Add the pulse, making it relative to the start of that run.
                runPulses[runs[j]].push_back(Pulse(pulses[i].label, pulses[i].start-runNXS->startSinceEpoch, pulses[i].end-runNXS->startSinceEpoch));                delete runNXS;
                break;
            }
            else {
                std::cout << pulses[i].start << " " << pulses[i].end << " " << runNXS->startSinceEpoch << " " << runNXS->endSinceEpoch << std::endl;
            }
            delete runNXS;
        }
    }
    return true;
}