#include <algorithm>
#include <iostream>

#include <modex.hpp>
#include <nexus.hpp>


ModEx::ModEx(Config cfg_) : cfg(cfg_) {
    diagnosticFile = std::ofstream(diagnosticPath, std::ofstream::out | std::ofstream::trunc);
}

ModEx::~ModEx() {
    if (diagnosticFile)
        diagnosticFile.close();
}

bool ModEx::process() {
    if (cfg.extrapolationMode == NONE) {
        epochPulses(cfg.rawPulses);
        binPulsesToRuns(cfg.rawPulses);
        totalPulses = cfg.rawPulses.size();
        for (auto &pulse : cfg.rawPulses) {
            printf("   Processing  %f  ->  %f\n", pulse.start, pulse.end);
            processPulse(pulse);
        }
    }
    else if (cfg.extrapolationMode == FORWARDS_SUMMED) {
        // Our period ("sequence of pulses") contains exactly one pulse that we're interested in (enforced in config.cpp)
        // Extrapolate defined period forwards in time to generate all periods we need to bin from
        Period superPeriod;
        createSuperPeriod(superPeriod);
    

    }
    else {
        std::vector<Period> periods;

        extrapolatePeriods(periods);
        binPeriodsToRuns(periods);
        totalPulses = periods.size() * cfg.periodDefinition.pulseDefinitions.size();
        for (auto &period : periods) {
            if (period.isComplete()) {
                for (auto & pulse : period.pulses) {
                    printf("   Processing  %f  ->  %f\n", pulse.start, pulse.end);
                    processPulse(pulse);
                }
            }
            else {
                currentPulse += cfg.periodDefinition.pulseDefinitions.size();
                progress = (double) currentPulse / (double) totalPulses;
                progress*=100;
                diagnosticFile << "Period " << period.start << " to " << period.end << " was ignored (incomplete period)." << std::endl;
            }
        }
    }
    return true;
}

bool ModEx::extrapolatePeriods(std::vector<Period> &periods) {

    std::cout << "Extrapolating periods (in seconds since epoch)\n";
    Nexus firstRunNXS(cfg.runs[0]);
    firstRunNXS.load();
    Nexus lastRunNXS(cfg.runs[cfg.runs.size()-1]);
    lastRunNXS.load();

    const int expStart = firstRunNXS.startSinceEpoch;
    const int expEnd = lastRunNXS.endSinceEpoch;
    double startPeriod = double(expStart) + cfg.periodBegin;
    double periodBegin = 0;

    // First period
    std::vector<Pulse> firstPeriodPulses;
    for (auto &p :cfg.periodDefinition.pulseDefinitions) {
        firstPeriodPulses.push_back(Pulse(p, startPeriod + p.periodOffset, startPeriod + p.periodOffset + p.duration));
    }

    periods.push_back(Period(cfg.periodDefinition, startPeriod, startPeriod + cfg.periodDefinition.duration, firstPeriodPulses));

    if (cfg.extrapolationMode == BACKWARDS || cfg.extrapolationMode == BI_DIRECTIONAL) {
        periodBegin = startPeriod - cfg.periodDefinition.duration;
        while (periodBegin > expStart) {
            std::vector<Pulse> pulses;
            for (auto& p: cfg.periodDefinition.pulseDefinitions) {
                pulses.push_back(Pulse(p, periodBegin + p.periodOffset, periodBegin + p.periodOffset + p.duration));
            }
            periods.push_back(Period(cfg.periodDefinition, periodBegin, periodBegin + cfg.periodDefinition.duration, pulses));
            periodBegin -= cfg.periodDefinition.duration;
        }
    
    }
    if (cfg.extrapolationMode == FORWARDS || cfg.extrapolationMode == BI_DIRECTIONAL) {
        periodBegin  = startPeriod + cfg.periodDefinition.duration;
        while (periodBegin < expEnd) {
            std::vector<Pulse> pulses;
            for (auto& p: cfg.periodDefinition.pulseDefinitions) {
                pulses.push_back(Pulse(p, periodBegin + p.periodOffset, periodBegin + p.periodOffset + p.duration));
            }
            periods.push_back(Period(cfg.periodDefinition, periodBegin, periodBegin + cfg.periodDefinition.duration, pulses));
            periodBegin += cfg.periodDefinition.duration;
        }
    }

    std::sort(
        periods.begin(), periods.end(),
        [](
            const Period a,
            const Period b
        ){
            return a.start < b.end;
        }
    );

    printf("Extrapolated %i periods:\n", periods.size());
    for (const auto &p : periods)
        printf("  %f  ->  %f\n", p.start, p.end);
    return true;
}


bool ModEx::createSuperPeriod(Period &period)
{
  std::cout << "Extrapolating periods (in seconds since epoch)\n";
    Nexus firstRunNXS(cfg.runs[0]);
    firstRunNXS.load();
    Nexus lastRunNXS(cfg.runs[cfg.runs.size()-1]);
    lastRunNXS.load();

    // Get limiting times of experiment (seconds since epoch values)
    const int expStart = firstRunNXS.startSinceEpoch;
    const int expEnd = lastRunNXS.endSinceEpoch;

    // Set absolute start time of first period, equal to the experiment start plus first defined pulse time
    double periodStart = double(expStart) + cfg.periodBegin;

    // We have enforced that only a single master PulseDefinition exists - grab it
    auto &masterPulseDef = cfg.periodDefinition.pulseDefinitions.front();

    // Create copies of the master pulse extrapolating forwards in time by the period duration
    std::vector<Pulse> pulses;
    while (periodStart < expEnd) {
        // Check that the actual start of the pulse is before the experimental end time
        if ((periodStart + masterPulseDef.periodOffset) >= expEnd)
            break;
        pulses.push_back(Pulse(masterPulseDef, periodStart + masterPulseDef.periodOffset, periodStart + masterPulseDef.periodOffset + masterPulseDef.duration));
        periodStart += cfg.periodDefinition.duration;
    }

    // Create the superperiod
    auto superPeriodStart = double(expStart) + cfg.periodBegin;
    auto superPeriodEnd = superPeriodStart + cfg.periodDefinition.duration * pulses.size();
    period = Period(cfg.periodDefinition, superPeriodStart, superPeriodEnd, pulses);

    printf("Extrapolated %i pulses into superperiod:\n", period.pulses.size());
    for (const auto &p : period.pulses)
        printf("  %f  ->  %f\n", p.start, p.end);
    return true;
}

bool ModEx::binPeriodsToRuns(std::vector<Period> &periods) {


    for (auto &period : periods) {
        binPulsesToRuns(period.pulses);
    }

    return true;
}

bool ModEx::binPulsesToRuns(std::vector<Pulse> &pulses) {

    std::vector<std::pair<std::string, std::pair<int, int>>> runBoundaries;

    for (const auto &run : cfg.runs) {
        Nexus nxs(run);
        nxs.load();
        runBoundaries.push_back(std::make_pair(run, std::make_pair(nxs.startSinceEpoch, nxs.endSinceEpoch)));
    }

        for (auto &pulse : pulses) {
            for (int i=0; i<runBoundaries.size(); ++i) {
                if ((pulse.start >= runBoundaries[i].second.first) && (pulse.start < runBoundaries[i].second.second)) {
                    pulse.startRun = runBoundaries[i].first;
                    if (i < runBoundaries.size()-1) {
                        pulse.endRun = runBoundaries[i+1].first;
                    }
                    else {
                        pulse.endRun = pulse.startRun;
                    }
                }
                if ((pulse.end >= runBoundaries[i].second.first) && (pulse.end < runBoundaries[i].second.second)) {
                    pulse.endRun = runBoundaries[i].first;
                    if (pulse.startRun.empty()) {
                        if (i > 0) {
                            pulse.startRun = runBoundaries[i-1].first;
                        }
                        else {
                            pulse.startRun = pulse.endRun;
                        }
                    }
                    break;
                }
            }
        }
        return true;
}

bool ModEx::processPulse(Pulse &pulse) {
    if (pulse.startRun == pulse.endRun) {
        std::string outpath;
        if (!pulse.definition.label.empty())
            outpath = cfg.outputDir + "/" + std::to_string((int) pulse.start) + "-" + pulse.definition.label + ".nxs";
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
        if (!pulse.definition.label.empty())
            outpath = cfg.outputDir + "/" + std::to_string((int) pulse.start) + "-" + pulse.definition.label + ".nxs";
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
        Pulse firstPulse(pulse.start, startNxs.endSinceEpoch);
        Pulse secondPulse(endNxs.startSinceEpoch, pulse.end);
        std::cout << "Pulse duration: " << secondPulse.end - firstPulse.start << std::endl;
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