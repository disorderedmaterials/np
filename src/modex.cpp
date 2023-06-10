#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <memory>

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
        // Extrapolate defined pulse forwards in time to generate all pulses we need to bin from over the entire run
        Period superPeriod;
        if (!createSuperPeriod(superPeriod, cfg.summedNSlices))
            return false;
    
        // Template our output file(s) from the first run
        std::map<int, Nexus> outputFiles;
        for (auto n = 0; n< cfg.summedNSlices; ++n)
        {
            // std::string outpath = cfg.outputDir + "/sum-" + std::to_string((int) cfg.periodBegin);
            std::stringstream ss;
            ss << cfg.outputDir << "/sum-" << std::to_string((int) cfg.periodBegin);
            if (!cfg.periodDefinition.pulseDefinitions.front().label.empty())
                ss << "-" << cfg.periodDefinition.pulseDefinitions.front().label;
            if (cfg.summedNSlices > 1)
                ss << "-" << std::setw(3) << std::setfill('0') << (n+1);
            ss << ".nxs";
            auto data = outputFiles.emplace(n, Nexus(cfg.runs[0], ss.str()));
            if (!data.first->second.createEmpty(cfg.nxsDefinitionPaths))
                return false;
        }

        // Loop over input Nexus files
        for (auto &nxsFileName : cfg.runs)
        {
            // Open the Nexus file ready for use
            Nexus nxs(nxsFileName);
            nxs.load(true);
            printf("Nexus file has %i goodframes...\n", nxs.goodFrames);

            // Zero frame counters in all pulses
            for (auto &p : superPeriod.pulses)
                p.frameCounter = 0;

            // Get first and last pulses which this file might contribute to
            auto beginPulseIt = std::find_if(superPeriod.pulses.begin(), superPeriod.pulses.end(), [&nxs](const auto &p) { return p.end > nxs.startSinceEpoch && p.end < nxs.endSinceEpoch; });
            if (beginPulseIt == superPeriod.pulses.end())
            {
                printf("!!! No pulses fall into the time range of this file - moving on to the next...\n");
                continue;
            }
            auto endPulseIt = std::find_if(superPeriod.pulses.begin(), superPeriod.pulses.end(), [&nxs](const auto &p) { return p.start > nxs.endSinceEpoch; });

            // Loop over frames in the Nexus file
            auto pulseIt = beginPulseIt;
            for (int i=0; i<nxs.frameIndices.size()-1; ++i)
            {
                // Get start, end, and zero for frame
                auto eventStart = nxs.frameIndices[i];
                auto eventEnd = nxs.frameIndices[i+1];
                auto frameZero = nxs.frameOffsets[i] + nxs.startSinceEpoch;

                // If the frame zero is less than the start time of the current pulse, move on
                if (frameZero < pulseIt->start)
                    continue;

                // If the frame zero is less than the end time of the current pulse, bin the events. Otherwise, increase the pulse iterator
                if (frameZero < pulseIt->end)
                {
                    // Grab the destination datafile for this pulse and bin events
                    auto &destinationNexus = outputFiles[pulseIt->sliceIndex];
                    for (int k=eventStart; k<eventEnd; ++k)
                    {
                        auto id = nxs.eventIndices[k];
                        auto event = nxs.events[k];
                        if (id > 0)
                            gsl_histogram_increment(destinationNexus.histogram[id], event);
                    }

                    // Increment the goodframes counter for this pulse
                    ++(pulseIt->frameCounter);
                }
                else
                    ++pulseIt;
                
                // If we have no more pulses, we can stop processing frames
                if (pulseIt == endPulseIt)
                        break;
            }

            // For each pulse we just added to, increase the goodFrames, and monitors by the correct fractional amount
            for (auto it = beginPulseIt; it < endPulseIt; ++it)
            {
                if (it->frameCounter == 0)
                    continue;
                auto &destinationNexus = outputFiles[it->sliceIndex];
                destinationNexus.goodFrames += it->frameCounter;
                nxs.addMonitors((double) it->frameCounter / nxs.goodFrames, destinationNexus);
            }

            // If we have no more pulses, we can stop processing nexus files
            if (pulseIt == endPulseIt)
                break;
        }

        // Save output files
        for (auto &data : outputFiles)
        {
            auto sliceIndex = data.first + 1;
            auto &nexus = data.second;

            printf("Output nexus for slice %i has %li good frames in total.\n", sliceIndex, nexus.goodFrames);

            if (!nexus.output(cfg.nxsDefinitionPaths))
                return false;
            diagnosticFile << nexus.getOutpath() << " " << nexus.goodFrames << std::endl;
        }
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


bool ModEx::createSuperPeriod(Period &period, int nSlices)
{
  std::cout << "Extrapolating periods (in seconds since epoch)\n";
    Nexus firstRunNXS(cfg.runs[0]);
    if (!firstRunNXS.load())
        return false;
    Nexus lastRunNXS(cfg.runs[cfg.runs.size()-1]);
    if (!lastRunNXS.load())
        return false;

    // Get limiting times of experiment (seconds since epoch values)
    const int expStart = firstRunNXS.startSinceEpoch;
    const int expEnd = lastRunNXS.endSinceEpoch;

    // Set absolute start time of first period, equal to the experiment start plus first defined pulse time
    double periodStart = double(expStart) + cfg.periodBegin;

    // We have enforced that only a single master PulseDefinition exists - grab it
    auto &masterPulseDef = cfg.periodDefinition.pulseDefinitions.front();

    // Create copies of the master pulse extrapolating forwards in time by the period duration
    std::vector<Pulse> pulses;
    auto nWholePulses = 0;
    while (periodStart < expEnd) {
        // Check that the actual start of the pulse is before the experimental end time
        if ((periodStart + masterPulseDef.periodOffset) >= expEnd)
            break;
        
        // Take the pulse and split into the number of slices
        auto sliceDelta = (double) masterPulseDef.duration / (double) nSlices;
        auto sliceStart = periodStart + masterPulseDef.periodOffset;
        for (auto n = 0; n < nSlices; ++n)
        {
            pulses.push_back(Pulse(masterPulseDef, sliceStart, sliceStart + sliceDelta, n));
            sliceStart += sliceDelta;   
        }

        ++nWholePulses;
        periodStart += cfg.periodDefinition.duration;
    }

    // Create the superperiod
    auto superPeriodStart = double(expStart) + cfg.periodBegin;
    auto superPeriodEnd = superPeriodStart + cfg.periodDefinition.duration * pulses.size();
    period = Period(cfg.periodDefinition, superPeriodStart, superPeriodEnd, pulses);

    printf("Extrapolated %i pulses into superperiod (from %i whole pulses split into %i slices each):\n", period.pulses.size(), nWholePulses, nSlices);
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
        if (!nxs.reduceMonitors((double) nxs.goodFrames / (double) nxs.rawFrames))
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
        int totalFrames = startNxs.rawFrames + endNxs.rawFrames;
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