#include "processors.h"
#include "pulse.h"
#include <vector>

int main(int argc, char **argv)
{
    // Paths to NeXuS files to process
    std::vector<std::string> inputFiles_ = {"/home/tris/src/ModEx/test/NIMROD00077656.nxs"};
    // Output directory
    std::string outputDirectory_ = "/home/tris/src/ModEx/test/";
    // Processing mode
    Processors::ProcessingMode processingMode_ = Processors::ProcessingMode::FORWARDS_SUMMED;
    // Pulse definitions
    std::vector<Pulse> pulses_;
    //    pulses_.emplace_back("OX", 1664637204, 440.0);
    pulses_.emplace_back("OX", 1664630000, 440.0);

    // Parse CLI arguments
    // TODO

    if (processingMode_ == Processors::ProcessingMode::FORWARDS_SUMMED)
    {
        Processors::processForwardsSummed(inputFiles_, outputDirectory_, pulses_.front(), 1, 440.0);
    }

    return 0;
}
