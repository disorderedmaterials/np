#include <iostream>

#include "nexus.h"

int main(int argc, char **argv)
{

    if (argc < 5)
    {
        std::cout << "Usage: partition_events {Nexus file} {lower spectrum} "
                     "{higher spectrum} {output file}"
                  << std::endl;
        return -1;
    }

    Nexus nxs(argv[1], argv[4]);
    if (!nxs.load(true))
        return -1;
    if (!nxs.writePartitionsWithRelativeTimes(atoi(argv[2]), atoi(argv[3])))
        return -1;
    return 0;
}
