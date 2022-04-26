#include <config.hpp>

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <sys/stat.h>


Config::Config(std::string path) {

    int nRuns, nPulses;

    std::string line;
    std::ifstream ifs(path);
    std::getline(ifs, gudrunInputFile);
    std::cout << "Gudrun input file: " << gudrunInputFile << std::endl;
    std::getline(ifs, purgeInputFile);
    std::cout << "Purge file: " << purgeInputFile << std::endl;
    std::getline(ifs, outputDir);
    std::cout << "Output dir: " << outputDir << std::endl;

    // Check if file exists, create if not.
    struct stat info;
    if (stat(outputDir.c_str(), &info) !=0) {
        mkdir(outputDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        std::cout << "Created " << outputDir << std::endl;
    }

    std::getline(ifs, line);
    nRuns = atoi(line.c_str());
    std::cout << "There are " << nRuns << " runs" << std::endl;
    std::getline(ifs, dataFileDir);
    for (int i=0; i<nRuns; ++i) {
        std::getline(ifs, line);
        runs.push_back(line);
        std::cout << line << std::endl;
    }


    std::getline(ifs, line);

    if (line == "BACKWARDS")
        extrapolationMode = BACKWARDS;
    else if (line == "FORWARDS")
        extrapolationMode = FORWARDS;
    else if (line == "BI-DIRECTIONAL")
        extrapolationMode = BI_DIRECTIONAL;
    else
        extrapolationMode = NONE;
    std::cout << "Extrapolate: " << extrapolationMode << std::endl;
    if (extrapolationMode != NONE) {
        std::getline(ifs, line);
        double periodDuration = atof(line.c_str());
        std::getline(ifs, line);
        periodBegin = atof(line.c_str());
        std::getline(ifs, line);
        nPulses = atoi(line.c_str());
        std::vector<PulseDefinition> pulses;

        for (int i=0; i<nPulses; ++i) {
            std::getline(ifs, line);
            std::stringstream ss(line);
            std::string label;
            double periodOffset, duration;
            std::string item;
            std::getline(ss, label, ' ');
            std::getline(ss, item, ' ');
            periodOffset = atof(item.c_str());
            std::getline(ss, item, ' ');
            duration = atof(item.c_str());
            pulses.push_back(PulseDefinition(label, periodOffset, duration));
        }

        period = PeriodDefinition(periodDuration, pulses);
    } else {
        std::getline(ifs, line);
        nPulses = atoi(line.c_str());
        for (int i=0; i<nPulses; ++i) {
            std::getline(ifs, line);
            std::stringstream ss(line);
            std::string label;
            double pulseStart, pulseEnd;
            std::string item;
            std::getline(ss, label, ' ');
            std::getline(ss, item, ' ');
            pulseStart = atof(item.c_str());
            std::getline(ss, item, ' ');
            pulseEnd = atof(item.c_str());
            pulses.push_back(Pulse(label, pulseStart, pulseEnd));
        }

    }
}