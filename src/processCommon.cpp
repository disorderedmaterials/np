#include "nexusFile.h"
#include "processors.h"
#include "window.h"
#include <iomanip>
#include <sstream>

namespace Processors
{
// Prepare slices for specified Window
std::vector<std::pair<Window, NeXuSFile>> prepareSlices(const Window &window, int nSlices, std::string templatingSourceFilename,
                                                        std::string_view outputFilePath)
{
    std::vector<std::pair<Window, NeXuSFile>> slices;
    const auto sliceDuration = window.duration() / nSlices;
    auto sliceStartTime = window.startTime();

    for (auto i = 0; i < nSlices; ++i)
    {
        std::stringstream outputFileName;
        outputFileName << outputFilePath << window.id();
        if (nSlices > 1)
            outputFileName << "-" << std::setw(3) << std::setfill('0') << (i + 1);
        outputFileName << ".nxs";

        std::stringstream sliceName;
        sliceName << window.id() << i + 1;

        auto &[newWin, nexus] = slices.emplace_back(Window(sliceName.str(), sliceStartTime, sliceDuration), NeXuSFile());

        nexus.templateFile(templatingSourceFilename, outputFileName.str());

        sliceStartTime += sliceDuration;
    }

    return slices;
}
} // namespace Processors
