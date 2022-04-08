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

    for (const auto runPair : runPulses) {
        std::string run = runPair.first;
        std::vector<std::pair<double, double>> pulses = runPair.second;

        for (auto p : pulses) {
            std::string nxsPath = "test/" + run.substr(run.find_last_of("/\\") + 1);
            Nexus *nxs = new Nexus(run, nxsPath);
            nxs->loadEventModeData();
            nxs->createHistogram(p);
            nxs->writeCountsHistogram();
            system("purge_det purge_det.dat");
            system(std::string("gudrun_dcs " + input).c_str());
            std::string output = std::string(out + "/" + std::to_string(p.first) + ".mint01");
            rename(nxsPath.c_str(), output.c_str());
        }
    }
}

bool ModEx::run(std::string run, std::vector<std::pair<double, double>> pulses) {
    return true;
}

bool ModEx::extrapolatePulseTimes(std::string start_run, double start, bool backwards, bool forwards, double step, double duration, std::map<std::string, std::vector<std::pair<double, double>>> &runPulses) {

    std::vector<std::pair<double, double>> pulses;

    const std::string firstRun = runs[0];
    const std::string lastRun = runs[runs.size()-1];

    Nexus firstRunNXS(firstRun);
    firstRunNXS.loadBasicData();
    Nexus lastRunNXS(lastRun);
    lastRunNXS.loadBasicData();
    Nexus startRunNXS(start_run);
    startRunNXS.loadBasicData();

    const int expStart = firstRunNXS.startSinceEpoch;
    const int expEnd = lastRunNXS.endSinceEpoch;
    double startPulse = startRunNXS.startSinceEpoch + start;
    double pulse = 0;
    pulses.push_back(std::make_pair(startPulse, startPulse+step));

    if (backwards) {
        pulse = startPulse - step;
        std::cout << startPulse << " " << step << std::endl;
        while (pulse > expStart) {
            pulses.push_back(std::make_pair(pulse, pulse+duration));
            pulse-=step;
        }
    }

    if (forwards) {
        pulse = startPulse + step;
        while (pulse < expEnd) {
            pulses.push_back(std::make_pair(pulse, pulse+duration));
            pulse+=step;
        }
    }
    std::sort(
        pulses.begin(), pulses.end(),
        [](
            const std::pair<double, double> a,
            const std::pair<double, double> b
            ){
                return a.first < b.first;
            }
    );
    for (int i=0; i<pulses.size(); ++i) {
        for (int j=0; j<runs.size(); ++j) {
            Nexus *runNXS = new Nexus(runs[j]);
            runNXS->loadBasicData();
            if ((pulses[i].first>=runNXS->startSinceEpoch) && (pulses[i].second<=runNXS->endSinceEpoch)) {
                runPulses[runs[j]].push_back(std::make_pair(pulses[i].first-runNXS->startSinceEpoch,pulses[i].second-runNXS->startSinceEpoch));
                delete runNXS;
                break;
            }
        }
    }
    return true;
}
