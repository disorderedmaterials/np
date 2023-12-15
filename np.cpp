#include "processors.h"
#include "window.h"
#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>
#include <fmt/core.h>
#include <vector>

int main(int argc, char **argv)
{
    // Paths to NeXuS files to process
    std::vector<std::string> inputFiles_ = {"/home/tris/src/ModEx/test/NIMROD00077656.nxs"};
    // Output directory
    std::string outputDirectory_ = "/home/tris/src/ModEx/test/";
    // Processing mode
    Processors::ProcessingMode processingMode_ = Processors::ProcessingMode::FORWARDS_SUMMED;
    // Window definition
    std::string windowName_;
    double windowStartTime_{0.0};
    double windowWidth_{0.0};
    bool absoluteStartTime_{false};
    //    Pulse pulse_{"OX", 1664630000, 440.0};

    // Define and parse CLI arguments
    CLI::App app("NeXus Processor (np), Copyright (C) 2023 Jared Swift and Tristan Youngs.");
    // -- Window Definition
    app.add_option("--name", windowName_, "Name of the window, used as a prefix to all output files")->group("Window Definition");
    app.add_option("--start", windowStartTime_, "Start time of the window (relative to first input file start time unless --absolute-start is given)")->group("Window Definition");
    app.add_option("--width", windowWidth_, "Window width in seconds)")->group("Window Definition");
    app.add_flag("--absolute-start", absoluteStartTime_,
                 "Flag that the given window start time is absolute, not relative")->group("Window Definition");
    // -- Input Files
    // TODO
    // -- Output Files
    app.add_option("--output-dir", outputDirectory_, "Output directory for generated NeXuS files.")->group("Output Files");

    // Construct the master window definition
    if (absoluteStartTime_)
    {
        // Need to query first NeXuS file to get its starttime
        NeXuSFile firstFile = XXX
    }
    Window window(windowName_, windowStartTime_ , windowWidth_);

    // Perform processing
    if (processingMode_ == Processors::ProcessingMode::FORWARDS_SUMMED)
    {
        Processors::processForwardsSummed(inputFiles_, outputDirectory_, pulse_, 1, 440.0);
    }

    return 0;
}
