#include <iostream>

#include "nexus.hpp"

int main(int argc, char **argv) {

    if (argc < 5) {
        std::cout << "Usage: partition_events {Nexus file} {lower spectrum} {higher spectrum} {output file}" << std::endl;
        return -1;
    }

    Nexus nxs(argv[1], argv[4]);
    nxs.load(true);
    nxs.writePartitionsWithRelativeTimes(atoi(argv[2]), atoi(argv[3]));
    return 0;
}