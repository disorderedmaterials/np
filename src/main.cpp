#include "nexus.hpp"
#include "modex.hpp"
#include <iostream>
#include <map>

int main(void) {

    std::vector<std::string> runs = {"data/NIMROD00069862.nxs"};
    ModEx m("azobenzene.txt", "purge_det.dat", "testout", runs);
    std::map<std::string, std::vector<std::pair<double, double>>> pulses;
    m.extrapolatePulseTimes(
        runs[0], 12129., false, true, 980., 300., pulses
    );
    m.run(pulses);

}