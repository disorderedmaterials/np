#include <config.hpp>

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <sys/stat.h>


bool Config::parse() {
    
    int nRuns, nPulses; // Number of run files and pulses.
    std::string line; // Auxilliary variable to read into.

    std::ifstream ifs(path); // Input file stream.

    // Read in the output directory.
    if (!std::getline(ifs, outputDir))
        return false;
    std::cout << "Output dir: " << outputDir << std::endl;

    // Check if the file exists.
    struct stat info;
    if (stat(outputDir.c_str(), &info) != 0)
        return false;
    
    // Read in paths from Nexus definitions file.
    std::string nxsDefinitionPath;
    if (!std::getline(ifs, nxsDefinitionPath))
        return false;
    std::ifstream nxsifs(nxsDefinitionPath); // Nexus definitions file stream.

    // Parse the Nexus definitions file.
    while (std::getline(nxsifs, line)) {
        std::string path;
        if (!line.size())
            continue;
        size_t pos = line.find("= ") + sizeof("= ")-1;
        line = line.substr(pos);
        std::stringstream ss(line);
        std::getline(ss, path, ' ');
        if (path.rfind("/", 0) == 0) {
            nxsDefinitionPaths.push_back("/raw_data_1" + path);
        }
    }

    // Read in nRuns.
    if (!std::getline(ifs, line))
        return false;
    nRuns = atoi(line.c_str());
    std::cout << "There are " << nRuns << " runs" << std::endl;

    // Read in runs.
    for (int i=0; i<nRuns; ++i) {
        if(!std::getline(ifs, line))
            return false;
        runs.push_back(line);
        std::cout << line << std::endl;
    }

    // Read in extrapolation mode.
    if (!std::getline(ifs, line))
        return false;
    if (line == "BACKWARDS")
        extrapolationMode = BACKWARDS;
    else if (line == "FORWARDS")
        extrapolationMode = FORWARDS;
    else if (line == "BI_DIRECTIONAL")
        extrapolationMode = BI_DIRECTIONAL;
    else
        extrapolationMode = NONE;
    std::cout << "Extrapolation Mode: " << extrapolationMode << std::endl;

    // If extrapolation mode is NONE, then expect pulses.
    if (extrapolationMode == NONE) {

        // Read in number of pulses.
        if (!std::getline(ifs, line))
            return false;
        nPulses = atoi(line.c_str());

        // Read in pulses.
        for (int i=0; i<nPulses; ++i) {
            if (!std::getline(ifs, line))
                return false;
            
            std::stringstream ss(line); // String stream for parsing.
            std::string label;
            double pulseStart, pulseEnd;
            std::string item; // Auxilliary variable to read into.
            
            // Read in label.
            if(!std::getline(ss, label, ' '))
                return false;

            // Read in pulse start time.
            if(!std::getline(ss, item, ' '))
                return false;
            pulseStart = atof(item.c_str());

            // Read in pulse end time.
            if (!std::getline(ss, item, ' '))
                return false;
            pulseEnd = atof(item.c_str());

            // Append pulse.
            pulses.push_back(Pulse(label, pulseStart, pulseEnd));
        }
    } else { // Otherwise expect period definition.

        // Read in period duration.
        if (!std::getline(ifs, line))
            return false;
        double periodDuration = atof(line.c_str());

        // Read in start time of first period.
        if (!std::getline(ifs, line))
            return false;
        periodBegin = atof(line.c_str());

        // Read in nPulses.
        if (!std::getline(ifs, line))
            return false;
        nPulses = atoi(line.c_str());

        std::vector<PulseDefinition> pulses; // Vector of defined pulses.

        // Read in pulse definitions.
        for (int i=0; i<nPulses; ++i) {
            if (!std::getline(ifs, line))
                return false;

            std::stringstream ss(line); // String stream for parsing.
            std::string label;
            double periodOffset, duration;
            std::string item;

            // Read in label.
            if (!std::getline(ss, label, ' '))
                return false;
            
            // Read in period offset.
            if (!std::getline(ss, item, ' '))
                return false;
            periodOffset = atof(item.c_str());

            // Read in pulse duration.
            if (!std::getline(ss, item, ' '))
                return false;
            duration = atof(item.c_str());

            // Append pulse definition.
            pulses.push_back(PulseDefinition(label, periodOffset, duration));
        }
        period = Period(periodDuration, pulses);

    }
    return true;

}