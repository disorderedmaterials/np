#include <iostream>

#include "nexus.hpp"

int main(int argc, char **argv) {

    if (argc < 4) {
        std::cout << "Target Nexus file not provided." << std::endl;
        std::cout << "Usage: partition_events {Nexus file} {lower spectrum} {higher spectrum}" << std::endl;
        return -1;
    }

    Nexus nxs(argv[1]);
    nxs.loadBasicData();
    nxs.loadEventModeData();
    nxs.writePartitionsWithRelativeTimes(atoi(argv[2]), atoi(argv[3]));
    return 0;
}