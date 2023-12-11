#include <iostream>

#include "config.h"
#include "modex.h"

int main(int argc, char **argv)
{

    if (argc < 2)
    {
        std::cout << "Configuration file not provided." << std::endl;
        std::cout << "Usage: modulation_excitation {config.cfg}" << std::endl;
        return -1;
    }

    Config config(argv[1]);
    if (!config.parse())
    {
        std::cout << "Could not parse configuration file." << std::endl;
        return -1;
    }

    ModEx modex(config);
    modex.process();

    return 0;
}
