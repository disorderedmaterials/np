#include "modex.hpp"
#include <cstdlib>
#include <algorithm>
#include <iostream>

bool ModEx::run() {
    // for (auto const run: runs) {
    //     Nexus nxs = Nexus(run, run.substr(run.find_last_of("/\\") + 1));
    // }
    return true;
}

bool ModEx::run(std::map<std::string, std::vector<std::pair<double, double>>> &runPulses) {

    // Perform a purge.
    system(std::string("purge " + purge).c_str());
    // Iterate through [run, pulse] pairs.
    for (const auto runPair : runPulses) {
        std::string run = runPair.first;
        std::vector<std::pair<double, double>> pulses = runPair.second;

        // Iterate through pulses.
        for (auto p : pulses) {
            // Output path = test/{runName}.nxs
            std::string nxsPath = "test/" + run.substr(run.find_last_of("/\\") + 1);
            // Allocate a new Nexus objecs on the heap.
            Nexus *nxs = new Nexus(run, nxsPath);
            // Load event mode data.
            nxs->loadEventModeData();
            // Create histogram using the pulse as boundaries.
            nxs->createHistogram(p);
            // Write the histogram.
            nxs->writeCountsHistogram();
            // Call gudrun_dcs.
            system(std::string("gudrun_dcs " + input).c_str());
            // Move the mint01 file to the output directory.
            std::string output = std::string(out + "/" + std::to_string(p.first) + ".mint01");
            rename(nxsPath.c_str(), output.c_str());
            delete nxs;
        }
    }
}

bool ModEx::run(std::string run, std::vector<std::pair<double, double>> pulses) {
    return true;
}

bool ModEx::extrapolatePulseTimes(std::string start_run, double start, bool backwards, bool forwards, double step, double duration, std::map<std::string, std::vector<std::pair<double, double>>> &runPulses) {

    std::vector<std::pair<double, double>> pulses;

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
    double startPulse = startRunNXS.startSinceEpoch + start;
    double pulse = 0;
    pulses.push_back(std::make_pair(startPulse, startPulse+step));

    // Extrapolate backwardsa.
    if (backwards) {
        pulse = startPulse - step;
        std::cout << startPulse << " " << step << std::endl;
        while (pulse > expStart) {
            pulses.push_back(std::make_pair(pulse, pulse+duration));
            pulse-=step;
        }
    }

    // Extrapolate forwards.
    if (forwards) {
        pulse = startPulse + step;
        while (pulse < expEnd) {
            pulses.push_back(std::make_pair(pulse, pulse+duration));
            pulse+=step;
        }
    }

    // Sort pulses by their start time.
    std::sort(
        pulses.begin(), pulses.end(),
        [](
            const std::pair<double, double> a,
            const std::pair<double, double> b
            ){
                return a.first < b.first;
            }
    );

    // Bin pulses by runs.
    for (int i=0; i<pulses.size(); ++i) {
        for (int j=0; j<runs.size(); ++j) {
            Nexus *runNXS = new Nexus(runs[j]); // Heap allocation again.
            runNXS->loadBasicData();
            // Pulse start is between the start and end of the run.
            if ((pulses[i].first>=runNXS->startSinceEpoch) && (pulses[i].second<=runNXS->endSinceEpoch)) {
                // Add the pulse, making it relative to the start of that run.
                runPulses[runs[j]].push_back(std::make_pair(pulses[i].first-runNXS->startSinceEpoch,pulses[i].second-runNXS->startSinceEpoch));
                delete runNXS;
                break;
            }
            delete runNXS;
        }
    }
    return true;
}
