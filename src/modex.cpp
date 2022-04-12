#include "modex.hpp"
#include <cstdlib>
#include <algorithm>
#include <iostream>

std::string extractRunName(std::string &path, std::string &run) {
    std::size_t preext = path.rfind('.');
    std::size_t leaf = path.rfind('/');

    return path.substr(leaf+1, preext);
}

bool ModEx::run() {
    // for (auto const run: runs) {
    //     Nexus nxs = Nexus(run, run.substr(run.find_last_of("/\\") + 1));
    // }
    return true;
}

bool ModEx::run(std::map<std::string, std::vector<std::pair<double, double>>> &runPulses) {
    std::cout << "ModEx begin." << std::endl;
    // Perform a purge.
    std::cout << "Performing a singular purge.." << std::endl;
    system(std::string("./purge_det " + purge).c_str());
    std::cout << "Purge complete." << std::endl;
    // Iterate through [run, pulse] pairs.

    int n=0;
    int m = 0;
    for (const auto runPair : runPulses)
        n+=runPair.second.size();

    std::cout << "There are " << n << "pulses across " << runPulses.size() << " to run on" << std::endl;

    for (const auto runPair : runPulses) {
        std::string run = runPair.first;
        std::vector<std::pair<double, double>> pulses = runPair.second;

        // Iterate through pulses.
        for (auto p : pulses) {
            std::cout << "Progress: " << (m/n)*100 << "%" << std::endl;
            // Output path = test/{runName}.nxs
            std::size_t sep = run.rfind('/');
            std::size_t dot = run.rfind('.');
            std::string baseName = run.substr(sep+1, dot-sep-1);
            std::cout << baseName << std::endl;
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
            system(std::string("mkdir modex_intermediate && chdir modex_intermediate && ../gudrun_dcs ../" + input).c_str());
            // Move the mint01 file to the output directory.
            std::string output = out + "/" + baseName + ".mint01";
            std::string target = "modex_intermediate/" + baseName + ".mint01";
            std::cout << target << "----> " << output << std::endl;
            system(std::string("mv " + target + " " + output).c_str());
            system("rm -rf modex_intermediate");
            delete nxs;
            ++m;
        }
    }
    return true;
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

    std::cout << "Performing extrapolation.." << std::endl;

    // Extrapolate backwardsa.
    if (backwards) {
        std::cout << "Extrapolating backwards.." << std::endl;
        pulse = startPulse - step;
        while (pulse > expStart) {
            pulses.push_back(std::make_pair(pulse, pulse+duration));
            pulse-=step;
        }
        std::cout  << "Finished extrapolating backwards.." << std::endl;
    }

    // Extrapolate forwards.
    if (forwards) {
        std::cout << "Extrapolating forwards" << std::endl;
        pulse = startPulse + step;
        while (pulse < expEnd) {
            pulses.push_back(std::make_pair(pulse, pulse+duration));
            pulse+=step;
        }
        std::cout << "FInished extrapolating forwards.." << std::endl;
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
    std::cout << "Extrapolation and run binning finished." << std::endl;
    return true;
}
