#include <iostream>

#include "nexus.hpp"

int main(int argc, char **argv) {

    if (argc < 2) {
        std::cout << "Target Nexus file not provided." << std::endl;
        std::cout << "Usage: partition_events {Nexus file}" << std::endl;
        return -1;
    }

    Nexus nxs(argv[1]);
    nxs.loadBasicData();
    nxs.loadEventModeData();
    nxs.writePartitions();
    return 0;
}