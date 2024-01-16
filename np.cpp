#include "nexusFile.h"
#include "processors.h"
#include "window.h"
#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>
#include <fmt/core.h>
#include <optional>
#include <vector>

int main(int argc, char **argv)
{
    // Paths to NeXuS files to process
    std::vector<std::string> inputFiles_;
    // Output directory
    std::string outputDirectory_;
    // Processing mode
    Processors::ProcessingMode processingMode_ = Processors::ProcessingMode::None;
    // Window definition
    std::string windowName_;
    double windowStartTime_{0.0};
    double windowWidth_{0.0};
    bool relativeStartTime_{false};
    // Time between windows
    double windowDelta_{0.0};
    // Number of slices to partition window in to
    int windowSlices_{1};
    // Target spectrum for event get (optional)
    std::optional<int> spectrumId_;

    // Define and parse CLI arguments
    CLI::App app("NeXuS Processor (np), Copyright (C) 2024 Jared Swift and Tristan Youngs.");
    // -- Window Definition
    app.add_option("-n,--name", windowName_, "Name of the window, used as a prefix to all output files")
        ->group("Window Definition")
        ->required();
    app.add_option("-s,--start", windowStartTime_,
                   "Start time of the window (relative to first input file start time unless --absolute-start is given)")
        ->group("Window Definition");
    app.add_option("-w,--width", windowWidth_, "Window width in seconds)")->group("Window Definition")->required();
    app.add_flag(
           "--relative-start", relativeStartTime_,
           "Flag that the given window start time is relative to the first run start time, not absolute (seconds since epoch)")
        ->group("Window Definition");
    app.add_option("-d,--delta", windowDelta_, "Time between window occurrences, in seconds)")
        ->group("Window Definition")
        ->required();
    // -- Input Files
    app.add_option("-f,--files", inputFiles_, "List of NeXuS files to process")->group("Input Files");
    // -- Output Files
    app.add_option("--output-dir", outputDirectory_, "Output directory for generated NeXuS files.")->group("Output Files");
    // -- Pre Processing
    app.add_option("-g,--get", spectrumId_, "Get all events from specified spectrum index")->group("Pre-Processing");
    // -- Processing Modes
    app.add_flag_callback(
           "--summed", [&]() { processingMode_ = Processors::ProcessingMode::Summed; },
           "Sum window occurrences instead of treating them individually")
        ->group("Processing");
    app.add_option("-l,--slices", windowSlices_, "Number of slices to split window definition in to (default = 1, no slicing)")
        ->group("Processing");
    // -- Post Processing
    app.add_flag_callback(
           "--scale-monitors", [&]() { Processors::postProcessingMode_ = Processors::PostProcessingMode::ScaleMonitors; },
           "Scale monitor counts in final output to match the number of frames processed for detectors")
        ->group("Post-Processing");
    app.add_flag_callback(
           "--scale-detectors", [&]() { Processors::postProcessingMode_ = Processors::PostProcessingMode::ScaleDetectors; },
           "Scale detector counts in final output to match the number of frames used for monitor counts")
        ->group("Post-Processing");

    CLI11_PARSE(app, argc, argv);

    // Perform pre-processing if requested
    if (spectrumId_)
    {
        Processors::getEvents(inputFiles_, *spectrumId_);
    }

    // Construct the master window definition
    if (relativeStartTime_)
    {
        // Need to query first NeXuS file to get its start time
        if (inputFiles_.empty())
        {
            fmt::print("Error: Need at least one input NeXuS file.");
            return 1;
        }
        NeXuSFile firstFile(inputFiles_.front());
        firstFile.loadTimes();
        fmt::print("Window start time converted from relative to absolute time: {} => {}\n", windowStartTime_,
                   windowStartTime_ + firstFile.startSinceEpoch());
        windowStartTime_ += firstFile.startSinceEpoch();
    }
    Window window(windowName_, windowStartTime_, windowWidth_);

    // Perform processing
    switch (processingMode_)
    {
        case (Processors::ProcessingMode::None):
            fmt::print("No processing mode specified. We are done.\n");
            break;
        case (Processors::ProcessingMode::Summed):
            Processors::processSummed(inputFiles_, outputDirectory_, window, windowSlices_, windowDelta_);
            break;
        default:
            throw(std::runtime_error("Unhandled processing mode.\n"));
    }

    return 0;
}
