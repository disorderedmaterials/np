#include <algorithm>
#include <iostream>

#include <modex.hpp>
#include <nexus.hpp>


ModEx::ModEx(Config cfg_) : cfg(cfg_) {
    diagnosticFile = std::ofstream(diagnosticPath);
}

ModEx::~ModEx() {
    if (diagnosticFile)
        diagnosticFile.close();
}

bool ModEx::process() {
    if (cfg.extrapolationMode == NONE) {
        epochPulses(cfg.pulses);
        binPulsesToRuns(cfg.pulses);
        totalPulses = cfg.pulses.size();
        for (Pulse &pulse : cfg.pulses) {
            processPulse(pulse);
        }
    }
    else {
        std::vector<std::vector<Pulse>> allPulses;
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
            std::cout << "binning pulses to runs" << std::endl;
            binPulsesToRuns(pulses);
            allPulses.push_back(pulses);
        }
        for (auto &pulses : allPulses) {
            totalPulses += pulses.size();
        }        
        for (auto &pulses : allPulses) {
            for (Pulse &pulse : pulses) {
                std::cout << "Processing " << pulse.start << " " << pulse.end << std::endl;
                processPulse(pulse);
            }
        }

    }
    return true;

}

bool ModEx::processPulse(Pulse &pulse) {
    if (pulse.startRun == pulse.endRun) {
        std::string outpath;
        if (!pulse.label.empty())
            outpath = cfg.outputDir + "/" + std::to_string((int) pulse.start) + "-" + pulse.label + ".nxs";
        else
            outpath = cfg.outputDir + "/" + std::to_string((int) pulse.start) + ".nxs";
        Nexus nxs = Nexus(pulse.startRun, outpath);
        if (!nxs.load(true))
            return false;
        if (!nxs.createHistogram(pulse, nxs.startSinceEpoch))
            return false;
        if (!nxs.reduceMonitors((double) nxs.goodFrames / (double) *nxs.rawFrames))
            return false;
        if (!nxs.output(cfg.nxsDefinitionPaths))
            return false;
        diagnosticFile << outpath << " " << nxs.goodFrames << std::endl;
        progress = (double) currentPulse / (double) totalPulses;
        progress*=100;
        ++currentPulse;
        std::cout << "Finished processing: " << outpath << std::endl;
        std::cout << "Progress: " << progress << "%" << std::endl;
        return true;
    }
    else {
        std::string outpath;
        if (!pulse.label.empty())
            outpath = cfg.outputDir + "/" + std::to_string((int) pulse.start) + "-" + pulse.label + ".nxs";
        else
            outpath = cfg.outputDir + "/" + std::to_string((int) pulse.start) + ".nxs";
        std::cout << outpath << std::endl;
        std::cout << pulse.startRun << std::endl;
        std::cout << pulse.endRun << std::endl;
        Nexus startNxs = Nexus(pulse.startRun);
        Nexus endNxs = Nexus(pulse.endRun, outpath);

        if (!startNxs.load(true))
            return false;
        if (!endNxs.load(true))
            return false;

        std::cout << pulse.start << " " << pulse.end << std::endl;
        Pulse firstPulse(pulse.label, pulse.start, startNxs.endSinceEpoch);
        Pulse secondPulse(pulse.label, startNxs.endSinceEpoch, pulse.end);

        std::map<int, std::vector<int>> monitors = startNxs.monitors;
        std::cout << "Sum monitors" << std::endl;
        for (auto &pair : monitors) {
            for (int i=0; i<pair.second.size(); ++i) {
                pair.second[i]+=endNxs.monitors[pair.first][i];
            }
        }

        if (!startNxs.createHistogram(firstPulse, startNxs.startSinceEpoch))
            return false;
        if (!endNxs.createHistogram(secondPulse, startNxs.histogram, endNxs.startSinceEpoch))
            return false;
        int totalGoodFrames = startNxs.goodFrames + endNxs.goodFrames;
        int totalFrames = *startNxs.rawFrames + *endNxs.rawFrames;
        if (!endNxs.reduceMonitors((double) totalGoodFrames / (double) totalFrames))
            return false;
        if (!endNxs.output(cfg.nxsDefinitionPaths))
            return false;
        diagnosticFile << outpath << " " << startNxs.goodFrames + endNxs.goodFrames << std::endl;
        progress = (double) currentPulse / (double) totalPulses;
        progress*=100;
        ++currentPulse;
        std::cout << "Finished processing: " << outpath << std::endl;
        std::cout << "Progress: " << progress << "%" << std::endl;
        return true;
    }
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

    std::vector<std::pair<std::string, std::pair<int, int>>> runBoundaries;

    for (auto &r : cfg.runs) {
        Nexus nxs(r);
        nxs.load();
        runBoundaries.push_back(std::make_pair(r, std::make_pair(nxs.startSinceEpoch, nxs.endSinceEpoch)));
    }

    for (int i=0; i<pulses.size(); ++i) {
        for (auto& pair: runBoundaries) {
            if ((pulses[i].start >= pair.second.first) && (pulses[i].start < pair.second.second)) {
                pulses[i].startRun = pair.first;
                break;
            }
        }
        for (auto& pair: runBoundaries) {
            if ((pulses[i].end >= pair.second.first) && (pulses[i].end < pair.second.second)) {
                pulses[i].endRun = pair.first;
                break;
            }
        }
        if (!pulses[i].startRun.size()) {
            for (int j=0; j<runBoundaries.size()-1; ++j) {
                if ((pulses[i].start >= runBoundaries[j].second.second) && (pulses[i].start < runBoundaries[j+1].second.first)) {
                    pulses[i].startRun = runBoundaries[j+1].first;
                }
            }

        }
        if (!pulses[i].endRun.size()) {
            for (int j=0; j<runBoundaries.size()-1; ++j) {
                if ((pulses[i].end >= runBoundaries[j].second.second) && (pulses[i].end < runBoundaries[j+1].second.first)) {
                    pulses[i].endRun = runBoundaries[j].first;
                }
            }
        }
    }

    return true;
}