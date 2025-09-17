#include "nexusFile.h"
#include <algorithm>
#include <array>
#include <ctime>
#include <fmt/core.h>
#include <iostream>

// Basic paths required when copying / creating a NeXuS file
std::vector<std::string> neXuSBasicPaths_ = {"/raw_data_1/title",
                                             "/raw_data_1/user_1/name",
                                             "/raw_data_1/start_time",
                                             "/raw_data_1/end_time",
                                             "/raw_data_1/good_frames",
                                             "/raw_data_1/raw_frames",
                                             "/raw_data_1/monitor_1/data",
                                             "/raw_data_1/monitor_2/data",
                                             "/raw_data_1/monitor_3/data",
                                             "/raw_data_1/monitor_4/data",
                                             "/raw_data_1/monitor_5/data",
                                             "/raw_data_1/monitor_6/data",
                                             "/raw_data_1/monitor_7/data",
                                             "/raw_data_1/monitor_8/data",
                                             "/raw_data_1/monitor_9/data",
                                             "/raw_data_1/detector_1/counts",
                                             "/raw_data_1/detector_1/spectrum_index",
                                             "/raw_data_1/detector_1/time_of_flight"};

NeXuSFile::NeXuSFile(std::string filename, bool printInfo) : filename_(filename)
{
    if (!filename_.empty())
    {
        fmt::print("Opening NeXuS file '{}'...\n", filename_);

        loadBasicData(printInfo);
    }
}

void NeXuSFile::operator=(NeXuSFile &source) { copy(source); }

NeXuSFile::NeXuSFile(const NeXuSFile &source) { copy(source); }

NeXuSFile::NeXuSFile(NeXuSFile &&source)
{
    // Copy data, but don't deep copy histograms (just copy pointers)
    copy(source, false);

    // Clear the detectorHistograms_ map here so we don't try to delete the histograms (which we just copied the pointers for)
    source.detectorHistograms_.clear();

    source.clear();
}

NeXuSFile::~NeXuSFile() { clear(); }

// Copy data from specified source
void NeXuSFile::copy(const NeXuSFile &source, bool deepCopyHistograms)
{
    filename_ = source.filename_;
    detectorSpectrumIndices_ = source.detectorSpectrumIndices_;
    nMonitorSpectra_ = source.nMonitorSpectra_;
    nMonitorFrames_ = source.nMonitorFrames_;
    nGoodFrames_ = source.nGoodFrames_;
    startSinceEpoch_ = source.startSinceEpoch_;
    endSinceEpoch_ = source.endSinceEpoch_;
    eventIndices_ = source.eventIndices_;
    eventTimes_ = source.eventTimes_;
    eventsPerFrame_ = source.eventsPerFrame_;
    frameOffsets_ = source.frameOffsets_;
    tofBoundaries_ = source.tofBoundaries_;
    monitorCounts_ = source.monitorCounts_;
    detectorCounts_ = source.detectorCounts_;
    detectorHistograms_ = source.detectorHistograms_;
}

// Clear all data and arrays
void NeXuSFile::clear()
{
    filename_ = {};
    detectorSpectrumIndices_.clear();
    nMonitorSpectra_ = 0;
    nMonitorFrames_ = 0;
    nGoodFrames_ = 0;
    startSinceEpoch_ = 0;
    endSinceEpoch_ = 0;
    eventIndices_.clear();
    eventTimes_.clear();
    eventsPerFrame_.clear();
    frameOffsets_.clear();
    tofBoundaries_.clear();
    monitorCounts_.clear();
    detectorCounts_.clear();
    detectorHistograms_.clear();
}

/*
 * I/O
 */

// Return handle and (simple) dimension for named leaf dataset
std::pair<H5::DataSet, long int> NeXuSFile::get1DDataset(H5::H5File file, H5std_string groupName, H5std_string datasetName)
{
    if (!file.nameExists(groupName))
    {
        fmt::print("ERROR: No group named '{}' exists.\n", groupName);
        return {};
    }

    H5::Group group = file.openGroup(groupName);
    if (!group.nameExists(datasetName))
    {
        fmt::print("ERROR: No dataset named '{}' exists in the group '{}' exists.\n", datasetName, groupName);
        return {};
    }

    H5::DataSet dataset = group.openDataSet(datasetName);
    H5::DataSpace space = dataset.getSpace();
    hsize_t spaceNDimensions = space.getSimpleExtentNdims();

    std::vector<hsize_t> spaceDimensions(spaceNDimensions);
    space.getSimpleExtentDims(spaceDimensions.data());
    hsize_t nPoints = space.getSimpleExtentNpoints();

    // Create a string of the dimensions
    std::string dims;
    for (auto d : spaceDimensions)
        dims += dims.empty() ? fmt::format("{}", d) : fmt::format(",{}", d);

    fmt::print("Got dataset '{}' in group '{}', rank/dims/N = {}/{}/{}\n", datasetName, groupName, spaceNDimensions, dims,
               nPoints);
    return {dataset, spaceDimensions[0]};
}

// Resize 1D dataset
void NeXuSFile::resize1DDataset(H5::DataSet dataset, std::vector<hsize_t> dimensions)
{
    H5::DataSpace space = dataset.getSpace();
    hsize_t spaceNDimensions = space.getSimpleExtentNdims();
    std::vector<hsize_t> spaceDimensions(spaceNDimensions);
    space.getSimpleExtentDims(spaceDimensions.data());

    // Create strings of the dimensions
    std::string oldDims, newDims;
    for (auto d : spaceDimensions)
        oldDims += oldDims.empty() ? fmt::format("{}", d) : fmt::format(",{}", d);
    for (auto d : dimensions)
        newDims += newDims.empty() ? fmt::format("{}", d) : fmt::format(",{}", d);

    hsize_t existingNPoints = space.getSimpleExtentNpoints();
    fmt::print("Checking dataset size - existing rank/dims = {}/{}, new = {}/{}...\n", spaceNDimensions, oldDims,
               dimensions.size(), newDims);
    if (spaceNDimensions == existingNPoints && spaceDimensions == dimensions)
        fmt::print(" ... Extent has not changed - no change will be made.\n");
    else
    {
        fmt::print(" ... Extent has changed - resizing dataset...\n");
        space.setExtentSimple(dimensions.size(), dimensions.data(), dimensions.data());
    }
}

// Return filename
std::string NeXuSFile::filename() const { return filename_; }

// Load basic information from the NeXuS file
void NeXuSFile::loadBasicData(bool printInfo)
{
    hid_t memType = H5Tcopy(H5T_C_S1);
    H5Tset_size(memType, UCHAR_MAX);
    char charBuffer[UCHAR_MAX];

    // Open input NeXuS file in read only mode.
    H5::H5File input = H5::H5File(filename_, H5F_ACC_RDONLY);

    // Get the run title
    auto &&[titleID, titleDimension] = NeXuSFile::get1DDataset(input, "raw_data_1", "title");
    H5Dread(titleID.getId(), memType, H5S_ALL, H5S_ALL, H5P_DEFAULT, charBuffer);
    if (printInfo)
        fmt::print("... run title was '{}'\n", charBuffer);

    // Read in start time in Unix time.
    int y = 0, M = 0, d = 0, h = 0, m = 0, s = 0;

    auto &&[startTimeID, startTimeDimension] = NeXuSFile::get1DDataset(input, "raw_data_1", "start_time");
    H5Dread(startTimeID.getId(), memType, H5S_ALL, H5S_ALL, H5P_DEFAULT, charBuffer);

    sscanf(charBuffer, "%d-%d-%dT%d:%d:%d", &y, &M, &d, &h, &m, &s);
    std::tm stime = {0};
    stime.tm_year = y - 1900;
    stime.tm_mon = M - 1;
    stime.tm_mday = d;
    stime.tm_hour = h;
    stime.tm_min = m;
    stime.tm_sec = s;
    startSinceEpoch_ = (int)mktime(&stime);
    if (printInfo)
        fmt::print("... run started at {} ({} s since epoch).\n", charBuffer, startSinceEpoch_);

    // Read in end time in Unix time.
    auto &&[endTimeID, endTimeDimension] = NeXuSFile::get1DDataset(input, "raw_data_1", "end_time");
    H5Dread(endTimeID.getId(), memType, H5S_ALL, H5S_ALL, H5P_DEFAULT, charBuffer);

    sscanf(charBuffer, "%d-%d-%dT%d:%d:%d", &y, &M, &d, &h, &m, &s);
    std::tm etime = {0};
    etime.tm_year = y - 1900;
    etime.tm_mon = M - 1;
    etime.tm_mday = d;
    etime.tm_hour = h;
    etime.tm_min = m;
    etime.tm_sec = s;
    endSinceEpoch_ = (int)mktime(&etime);
    if (printInfo)
    {
        fmt::print("... run ended at {} ({} s since epoch).\n", charBuffer, endSinceEpoch_);
        fmt::print("... literal run duration was {} s.\n", endSinceEpoch_ - startSinceEpoch_);
    }

    // Read in good frames
    auto &&[goodFramesID, goodFramesDimension] = NeXuSFile::get1DDataset(input, "raw_data_1", "good_frames");
    auto goodFramesTemp = new int[(long int)goodFramesDimension];
    H5Dread(goodFramesID.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, goodFramesTemp);
    nGoodFrames_ = goodFramesTemp[0];
    if (printInfo)
        fmt::print("... there were {} good frames.\n", nGoodFrames_);

    input.close();
}

// Template a new NeXusFile from that specified
void NeXuSFile::templateTo(std::string sourceFilename, std::string newFilename)
{
    // Open this NeXuS file in read only mode.
    H5::H5File input = H5::H5File(sourceFilename, H5F_ACC_RDONLY);

    // Create new NeXuS file for output.
    H5::H5File output = H5::H5File(newFilename, H5F_ACC_TRUNC);

    printf("Templating file '%s' to '%s'...\n", sourceFilename.c_str(), newFilename.c_str());

    hid_t ocpl_id, lcpl_id;
    ocpl_id = H5Pcreate(H5P_OBJECT_COPY);
    if (ocpl_id < 0)
        throw(std::runtime_error("File templating failed.\n"));
    lcpl_id = H5Pcreate(H5P_LINK_CREATE);
    if (lcpl_id < 0)
        throw(std::runtime_error("File templating failed.\n"));
    if (H5Pset_create_intermediate_group(lcpl_id, 1) < 0)
        throw(std::runtime_error("File templating failed.\n"));

    for (const auto &path : neXuSBasicPaths_)
    {
        if (H5Ocopy(input.getId(), path.c_str(), output.getId(), path.c_str(), ocpl_id, lcpl_id) < 0)
            throw(std::runtime_error("Failed to copy one or more paths.\n"));
    }

    H5Pclose(ocpl_id);
    H5Pclose(lcpl_id);

    input.close();
    output.close();
}

// Prepare spectra storage, including loading TOF boundaries etc.
void NeXuSFile::prepareSpectraSpace(bool printInfo)
{
    // Open input NeXuS file in read only mode.
    H5::H5File input = H5::H5File(filename_, H5F_ACC_RDONLY);

    // Read in detector spectra information
    auto &&[detSpecIndices, spectraDimension] = NeXuSFile::get1DDataset(input, "raw_data_1/detector_1", "spectrum_index");
    detectorSpectrumIndices_.resize(spectraDimension);
    H5Dread(detSpecIndices.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, detectorSpectrumIndices_.data());
    nMonitorSpectra_ = detectorSpectrumIndices_.front() - 1;

    if (printInfo)
    {
        fmt::print("... total number of detector spectra is {}.\n", detectorSpectrumIndices_.size());
        fmt::print("... inferred number of monitor spectra is {}.\n", nMonitorSpectra_);
    }

    // Read in TOF boundary information.
    auto &&[tofBoundariesID, tofBoundariesDimension] =
        NeXuSFile::get1DDataset(input, "raw_data_1/detector_1", "time_of_flight");
    tofBoundaries_.resize(tofBoundariesDimension);
    H5Dread(tofBoundariesID.getId(), H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, tofBoundaries_.data());

    // Set up detector histograms and straight counts vectors
    for (auto spec : detectorSpectrumIndices_)
        appendEmptyDetector(spec);

    input.close();
}

// Load in monitor histograms
void NeXuSFile::loadMonitorCounts()
{
    printf("Load monitor counts...\n");

    // Open our NeXuS file in read only mode.
    H5::H5File input = H5::H5File(filename_, H5F_ACC_RDONLY);

    // Read in monitor data - start from index 1 and end when we fail to find the named dataset with this suffix
    for (auto i = 1; i <= nMonitorSpectra_; ++i)
    {
        auto &&[monitorSpectrum, monitorSpectrumDimension] =
            NeXuSFile::get1DDataset(input, "/raw_data_1/monitor_" + std::to_string(i), "data");

        monitorCounts_[i].resize(tofBoundaries_.size());
        H5Dread(monitorSpectrum.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, monitorCounts_[i].data());
    }

    // Read in number of good frames - this will reflect our current monitor frame count since we copied those histograms in
    // full
    auto &&[goodFramesID, goodFramesDimension] = NeXuSFile::get1DDataset(input, "raw_data_1", "good_frames");
    auto goodFramesTemp = new int[(long int)goodFramesDimension];
    H5Dread(goodFramesID.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, goodFramesTemp);
    nMonitorFrames_ = goodFramesTemp[0];

    input.close();
}

// Load event data
void NeXuSFile::loadEventData()
{
    printf("Loading event data...\n");

    // Open our NeXuS file in read only mode.
    H5::H5File input = H5::H5File(filename_, H5F_ACC_RDONLY);

    // Read in event indices.
    auto &&[eventIndicesID, eventIndicesDimension] = NeXuSFile::get1DDataset(input, "raw_data_1/detector_1_events", "event_id");
    eventIndices_.resize(eventIndicesDimension);
    H5Dread(eventIndicesID.getId(), H5T_STD_I64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, eventIndices_.data());

    // Read in events.
    auto &&[eventTimesID, eventTimesDimension] =
        NeXuSFile::get1DDataset(input, "raw_data_1/detector_1_events", "event_time_offset");
    eventTimes_.resize(eventTimesDimension);
    H5Dread(eventTimesID.getId(), H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, eventTimes_.data());

    // Read in event counts per frame
    auto &&[eventsPerFrameID, eventsPerFrameDimension] =
        NeXuSFile::get1DDataset(input, "raw_data_1/framelog/events_log", "value");
    eventsPerFrame_.resize(eventsPerFrameDimension);
    H5Dread(eventsPerFrameID.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, eventsPerFrame_.data());

    // Read in frame offsets.
    auto &&[frameOffsetsID, frameOffsetsDimension] =
        NeXuSFile::get1DDataset(input, "raw_data_1/detector_1_events", "event_time_zero");
    frameOffsets_.resize(frameOffsetsDimension);
    H5Dread(frameOffsetsID.getId(), H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, frameOffsets_.data());

    fmt::print("... there are {} events...\n", eventTimes_.size());

    input.close();
}

// Load detector counts from the file
void NeXuSFile::loadDetectorCounts()
{
    printf("Load detector counts....\n");

    // Open our NeXuS file in read only mode.
    H5::H5File input = H5::H5File(filename_, H5F_ACC_RDONLY);

    const auto nSpec = detectorSpectrumIndices_.size();
    const auto nTOFBins = tofBoundaries_.size() - 1;

    // Note to Future Me: While you might think that the buffer into which we will read must match the datatype of that stored
    // (a 32-bit int) using a std::vector<long int> completely breaks the read and gives nothing but zeroes.
    std::vector<int> countsBuffer;
    countsBuffer.resize(nSpec * nTOFBins); // Need contiguous memory. This is a pain.
    auto &&[counts, detectorCountsDimension] = NeXuSFile::get1DDataset(input, "raw_data_1/detector_1", "counts");
    printf("LOAD1: %li\n", detectorCountsDimension);
    printf("LOAD2: %li\n", nSpec * nTOFBins);
    H5Dread(counts.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, countsBuffer.data());
    printf("LOAD3: %li %li %li\n", detectorCounts_.size(), detectorSpectrumIndices_.size(), detectorHistograms_.size());
    for (auto i = 0; i < nSpec; ++i)
    {
        auto &detCounts = detectorCounts_[detectorSpectrumIndices_[i]];
        for (auto j = 0; j < nTOFBins; ++j)
            detCounts[j] = countsBuffer[i * nTOFBins + j];
    }

    input.close();
}

// Save key modified data back to the file
bool NeXuSFile::saveModifiedData()
{
    // Open NeXuS file in read/write mode.
    H5::H5File output = H5::H5File(filename_, H5F_ACC_RDWR);

    const auto nSpec = detectorSpectrumIndices_.size();
    const auto nTOFBins = tofBoundaries_.size() - 1;
    printf("111 %li  %li\n", nSpec, nTOFBins);

    // Write good frames
    std::array<int, 1> framesBuffer;
    framesBuffer[0] = nGoodFrames_;
    auto &&[goodFrames, goodFramesDimension] = NeXuSFile::get1DDataset(output, "raw_data_1", "good_frames");
    goodFrames.write(framesBuffer.data(), H5::PredType::STD_I32LE);
    printf("111\n");
    // Write monitors
    for (auto &&[index, counts] : monitorCounts_)
    {
        auto &&[monitorCounts, monitorCountsDimension] =
            NeXuSFile::get1DDataset(output, "raw_data_1/monitor_" + std::to_string(index), "data");
        monitorCounts.write(counts.data(), H5::PredType::STD_I32LE);
    }

    // Write spectrum indices
    auto &&[detSpecIndices, spectraDimension] = NeXuSFile::get1DDataset(output, "raw_data_1/detector_1", "spectrum_index");
    NeXuSFile::resize1DDataset(detSpecIndices, {detectorSpectrumIndices_.size()});
    detSpecIndices.write(detectorSpectrumIndices_.data(), H5::PredType::STD_I32LE);

    // Write detector counts
    std::vector<long int> countsBuffer;
    countsBuffer.resize(nSpec * nTOFBins); // Need contiguous memory. This is a pain.
    for (auto i = 0; i < nSpec; ++i)
    {
        auto specID = detectorSpectrumIndices_[i];
        for (auto j = 0; j < nTOFBins; ++j)
            countsBuffer[i * nTOFBins + j] = detectorHistograms_[specID].value(j);
    }
    auto &&[counts, detectorCountsDimension] = NeXuSFile::get1DDataset(output, "raw_data_1/detector_1", "counts");
    NeXuSFile::resize1DDataset(counts, {1, nSpec, nTOFBins});
    counts.write(countsBuffer.data(), H5::PredType::STD_I32LE);

    output.close();

    return true;
}

/*
 * Data
 */

int NeXuSFile::nGoodFrames() const { return nGoodFrames_; }
void NeXuSFile::zeroGoodFrames() { nGoodFrames_ = 0; }
void NeXuSFile::incrementGoodFrames(int delta) { nGoodFrames_ += delta; }
int NeXuSFile::nMonitorFrames() const { return nMonitorFrames_; }
int NeXuSFile::startSinceEpoch() const { return startSinceEpoch_; }
int NeXuSFile::endSinceEpoch() const { return endSinceEpoch_; }
const std::vector<long long> &NeXuSFile::eventIndices() const { return eventIndices_; }
const std::vector<double> &NeXuSFile::eventTimes() const { return eventTimes_; }
const std::vector<int> &NeXuSFile::eventsPerFrame() const { return eventsPerFrame_; }
const std::vector<double> &NeXuSFile::frameOffsets() const { return frameOffsets_; }
const std::vector<double> &NeXuSFile::tofBoundaries() const { return tofBoundaries_; }
const int NeXuSFile::spectrumForDetector(int detectorId) const { return detectorSpectrumIndices_.at(detectorId - 1); }
const int NeXuSFile::nDetectors() const { return detectorSpectrumIndices_.size(); }
const std::map<int, std::vector<long int>> &NeXuSFile::monitorCounts() const { return monitorCounts_; }
const std::map<unsigned int, std::vector<long int>> &NeXuSFile::detectorCounts() const { return detectorCounts_; }
std::map<unsigned int, IntegerHistogram> &NeXuSFile::detectorHistograms() { return detectorHistograms_; }

/*
 * Manipulation
 */

// Remove the last detector spectrum, returning the index that was removed
int NeXuSFile::removeLastDetector()
{
    // Get target spectrum index
    auto specID = detectorSpectrumIndices_.back();
    detectorCounts_.erase(specID);
    detectorHistograms_.erase(specID);
    detectorSpectrumIndices_.pop_back();

    return specID;
}

// Append an empty detector
int NeXuSFile::appendEmptyDetector(int specID)
{
    // Generate and push target spectrum index if we need to
    if (specID == -1)
    {
        specID = detectorSpectrumIndices_.empty() ? 0 : (detectorSpectrumIndices_.back() + 1);
        detectorSpectrumIndices_.push_back(specID);
    }

    // For event re-binning
    detectorHistograms_[specID].initialise(tofBoundaries_);

    // For histogram manipulation
    detectorCounts_[specID].resize(tofBoundaries_.size() - 1);
    std::fill(detectorCounts_[specID].begin(), detectorCounts_[specID].end(), 0);

    return specID;
}

// Scale monitors by specified factor
void NeXuSFile::scaleMonitors(double factor)
{
    for (auto &&[index, counts] : monitorCounts_)
    {
        for (auto &bin : counts)
            bin *= factor;
    }

    nMonitorFrames_ *= factor;
    fmt::print(" ... New number of effective contributing monitor frames is {}.\n", nMonitorFrames_);
}

// Scale detectors by specified factor
void NeXuSFile::scaleDetectors(double factor)
{
    unsigned long long oldSum = 0, newSum = 0;
    for (auto i : detectorSpectrumIndices_)
    {
        oldSum += detectorHistograms_[i].sum();
        detectorHistograms_[i].scale(factor);
        newSum += detectorHistograms_[i].sum();
    }
    fmt::print(" ... Old counts was {}, now scaled to {} (ratio = {}).\n", oldSum, newSum, double(oldSum) / double(newSum));

    nGoodFrames_ *= factor;
    fmt::print(" ... New number of effective contributing detector frames is {}.\n", nGoodFrames_);
}
