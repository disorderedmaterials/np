#include "config.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>

bool Config::parse()
{

    int nRuns, nPulses;     // Number of run files and pulses.
    std::string line, item; // Auxilliary variable to read into.
    struct stat info;       // For statting files.

    // Check if file exists.
    if (stat(path.c_str(), &info) != 0)
    {
        std::cerr << "ERROR: couldn't stat input file " << path << " does it exist?" << std::endl;
        return false;
    }

    std::ifstream ifs(path); // Input file stream.

    // Read in the output directory.
    if (!std::getline(ifs, outputDir))
    {
        std::cerr << "ERROR: couldn't retrieve output directory." << std::endl;
        return false;
    }
    std::cout << "Output dir: " << outputDir << std::endl;

    // Check if the file exists.
    if (stat(outputDir.c_str(), &info) != 0)
    {
        std::cerr << "ERROR: couldn't stat output directory " << outputDir << " does it exist?" << std::endl;
        return false;
    }

    // Read in paths from Nexus definitions file.
    std::string nxsDefinitionPath;
    if (!std::getline(ifs, nxsDefinitionPath))
    {
        std::cerr << "ERROR: couldn't retrieve Nexus definitions file " << nxsDefinitionPath << std::endl;
        return false;
    }

    // Check if file exists.
    if (stat(nxsDefinitionPath.c_str(), &info) != 0)
    {
        std::cerr << "ERROR: couldn't stat Nexus definitions file " << nxsDefinitionPath << " does it exist?" << std::endl;
        return false;
    }

    std::ifstream nxsifs(nxsDefinitionPath); // Nexus definitions file stream.

    // Parse the Nexus definitions file.
    while (std::getline(nxsifs, line))
    {
        std::string path;
        if (!line.size())
            continue;
        size_t pos = line.find("= ") + sizeof("= ") - 1;
        line = line.substr(pos);
        std::stringstream ss(line);
        std::getline(ss, path, ' ');
        if (path.rfind("/", 0) == 0)
        {
            nxsDefinitionPaths.push_back("/raw_data_1" + path);
        }
    }

    // Read in nRuns.
    if (!std::getline(ifs, line))
    {
        std::cerr << "ERROR: couldn't retrieve number of runs." << std::endl;
        return false;
    }
    nRuns = atoi(line.c_str());
    std::cout << "There are " << nRuns << " runs" << std::endl;

    // Read in runs.
    for (int i = 0; i < nRuns; ++i)
    {
        if (!std::getline(ifs, line))
        {
            std::cerr << "ERROR: couldn't retrieve run " << i << "." << std::endl;
            return false;
        }
        runs.push_back(line);
        std::cout << line << std::endl;
    }

    // Read in extrapolation mode.
    if (!std::getline(ifs, line))
    {
        std::cerr << "ERROR: couldn't retrieve extrapolation mode." << std::endl;
        return false;
    }
    if (line == "BACKWARDS")
        extrapolationMode = BACKWARDS;
    else if (line == "FORWARDS")
        extrapolationMode = FORWARDS;
    else if (line == "BI_DIRECTIONAL")
        extrapolationMode = BI_DIRECTIONAL;
    else if (line == "FORWARDS_SUMMED")
        extrapolationMode = FORWARDS_SUMMED;
    else if (line == "NONE")
        extrapolationMode = NONE;
    else
    {
        std::cerr << "ERROR: unrecognised extrapolation mode " << line << "." << std::endl;
    }
    std::cout << "Extrapolation Mode: " << extrapolationMode << std::endl;

    // If extrapolation mode is NONE, then expect pulses.
    if (extrapolationMode == NONE)
    {

        // Read in number of pulses.
        if (!std::getline(ifs, line))
        {
            std::cerr << "ERROR: couldn't retrieve number of pulses." << std::endl;
            return false;
        }
        nPulses = atoi(line.c_str());

        // Read in pulses.
        for (int i = 0; i < nPulses; ++i)
        {
            if (!std::getline(ifs, line))
            {
                std::cerr << "ERROR: couldn't retrieve pulse " << i << "." << std::endl;
                return false;
            }

            std::stringstream ss(line); // String stream for parsing.
            double pulseStart, pulseEnd;

            // Read in pulse start time.
            if (!std::getline(ss, item, ' '))
            {
                std::cerr << "ERROR: couldn't retrieve pulse start from pulse " << i << "." << std::endl;
                return false;
            }
            pulseStart = atof(item.c_str());

            // Read in pulse end time.
            if (!std::getline(ss, item, ' '))
            {
                std::cerr << "ERROR: couldn't retrieve pulse end from pulse " << i << "." << std::endl;
                return false;
            }
            pulseEnd = atof(item.c_str());

            // Append pulse.
            rawPulses.push_back(Pulse(pulseStart, pulseEnd));
        }
    }
    else
    { // Otherwise expect period definition.

        // Read in period duration.
        if (!std::getline(ifs, line))
        {
            std::cerr << "ERROR: couldn't retrieve period duration." << std::endl;
            return false;
        }
        double periodDuration = atof(line.c_str());

        // Read in start time of first period.
        if (!std::getline(ifs, line))
        {
            std::cerr << "ERROR: couldn't retrieve time of first period." << std::endl;
            return false;
        }
        periodBegin = atof(line.c_str());

        // Read in number of pulses / slices
        if (!std::getline(ifs, line))
        {
            std::cerr << "ERROR: couldn't retrieve number of pulses / slices." << std::endl;
            return false;
        }
        std::stringstream pulseSliceStream(line); // String stream for parsing.
        if (!std::getline(pulseSliceStream, item, ' '))
        {
            std::cerr << "ERROR: couldn't retrieve number of pulses" << std::endl;
            return false;
        }
        nPulses = atoi(item.c_str());
        if (!std::getline(pulseSliceStream, item, ' '))
        {
            std::cerr << "ERROR: couldn't retrieve number of slices" << std::endl;
            return false;
        }
        summedNSlices = atoi(item.c_str());

        // For the FORWARDS_SUMMED option we expect exactly one pulse definition
        if (extrapolationMode == FORWARDS_SUMMED && nPulses != 1)
        {
            std::cerr << "ERROR: for the FORWARDS_SUMMED mode exactly one pulse must "
                         "be provided."
                      << std::endl;
            return false;
        }

        std::vector<PulseDefinition> pulseDefs; // Vector of defined pulses.

        // Read in pulse definitions.
        for (int i = 0; i < nPulses; ++i)
        {

            if (!std::getline(ifs, line))
            {
                std::cerr << "ERROR: couldn't retrieve pulse " << i << "." << std::endl;
                return false;
            }

            std::stringstream ss(line); // String stream for parsing.
            std::string label;
            double periodOffset, duration;

            // Read in label.
            if (!std::getline(ss, label, ' '))
            {
                std::cerr << "ERROR: couldn't retrieve label from pulse definition " << i << "." << std::endl;
                return false;
            }

            // Read in period offset.
            if (!std::getline(ss, item, ' '))
            {
                std::cerr << "ERROR: couldn't retrieve period offset from pulse definition " << i << "." << std::endl;
                return false;
            }
            periodOffset = atof(item.c_str());

            // Read in pulse duration.
            if (!std::getline(ss, item, ' '))
            {
                std::cerr << "ERROR: couldn't retrieve pulse duration from pulse definition " << i << "." << std::endl;
                return false;
            }
            duration = atof(item.c_str());

            // Append pulse definition.
            pulseDefs.push_back(PulseDefinition(label, periodOffset, duration));
        }
        periodDefinition = PeriodDefinition(periodDuration, pulseDefs);
        if (!periodDefinition.isValid())
        {
            return false;
        }
    }
    return true;
}
