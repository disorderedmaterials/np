#include "nexusFile.h"
#include <algorithm>
#include <ctime>
#include <fmt/core.h>
#include <iostream>

// Basic paths required when copying / creating a NeXuS file
std::vector<std::string> neXuSBasicPaths_ = {"/raw_data_1/title",
                                             "/raw_data_1/user_1/name",
                                             "/raw_data_1/start_time",
                                             "/raw_data_1/good_frames",
                                             "/raw_data_1/raw_frames",
                                             "/raw_data_1/monitor_1/data",
                                             "/raw_data_1/monitor_1/time_of_flight",
                                             "/raw_data_1/monitor_2/data",
                                             "/raw_data_1/monitor_3/data",
                                             "/raw_data_1/monitor_4/data",
                                             "/raw_data_1/monitor_5/data",
                                             "/raw_data_1/monitor_6/data",
                                             "/raw_data_1/monitor_7/data",
                                             "/raw_data_1/monitor_8/data",
                                             "/raw_data_1/monitor_9/data",
                                             "/raw_data_1/detector_1/counts"};

NeXuSFile::NeXuSFile(std::string filename) : filename_(filename) {}

/*
 * I/O
 */

// Return handle and (simple) dimension for named leaf dataset
std::pair<H5::DataSet, long int> NeXuSFile::find1DDataset(H5::H5File file, H5std_string groupName, H5std_string datasetName)
{
    if (!file.nameExists(groupName))
        return {};

    H5::Group group = file.openGroup(groupName);
    if (!group.nameExists(datasetName))
        return {};

    H5::DataSet dataset = group.openDataSet(datasetName);
    H5::DataSpace space = dataset.getSpace();
    hsize_t spaceNDims = space.getSimpleExtentNdims();

    hsize_t *spaceDims = new hsize_t[spaceNDims];
    space.getSimpleExtentDims(spaceDims);

    return {dataset, spaceDims[0]};
}

// Return filename
std::string NeXuSFile::filename() const { return filename_; }

// Template basic paths from the referenceFile, and make ready for histogram binning
void NeXuSFile::templateFile(std::string referenceFile, std::string outputFile)
{
    filename_ = outputFile;

    // Open input Nexus file in read only mode.
    H5::H5File input = H5::H5File(referenceFile, H5F_ACC_RDONLY);

    // Create new Nexus file for output.
    H5::H5File output = H5::H5File(filename_, H5F_ACC_TRUNC);

    printf("Templating file '%s' to '%s'...\n", referenceFile.c_str(), filename_.c_str());
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

    // Read in detector spectra information
    auto &&[spectraID, spectraDimension] = NeXuSFile::find1DDataset(input, "raw_data_1/detector_1", "spectrum_index");
    spectra_.resize(spectraDimension);
    H5Dread(spectraID.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, spectra_.data());

    // Read in TOF bin information.
    auto &&[tofBinsID, tofBinsDimension] = NeXuSFile::find1DDataset(input, "raw_data_1/monitor_1", "time_of_flight");
    tofBins_.resize(tofBinsDimension);
    H5Dread(tofBinsID.getId(), H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, tofBins_.data());

    // Set up detector histograms
    for (auto spec : spectra_)
    {
        detectorHistograms_[spec] = gsl_histogram_alloc(tofBins_.size() - 1);
        gsl_histogram_set_ranges(detectorHistograms_[spec], tofBins_.data(), tofBins_.size());
    }

    // Read in good frames - this will reflect our current monitor frame count since we copied those histograms in full
    auto &&[goodFramesID, goodFramesDimension] = NeXuSFile::find1DDataset(input, "raw_data_1", "good_frames");
    auto goodFramesTemp = new int[(long int)goodFramesDimension];
    H5Dread(goodFramesID.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, goodFramesTemp);
    nMonitorFrames_ = goodFramesTemp[0];

    input.close();
    output.close();
}

// Load frame counts
void NeXuSFile::loadFrameCounts()
{
    printf("Load frame counts...\n");

    // Open our Nexus file in read only mode.
    H5::H5File input = H5::H5File(filename_, H5F_ACC_RDONLY);

    // Read in good frames
    auto &&[goodFramesID, goodFramesDimension] = NeXuSFile::find1DDataset(input, "raw_data_1", "good_frames");
    auto goodFramesTemp = new int[(long int)goodFramesDimension];
    H5Dread(goodFramesID.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, goodFramesTemp);
    nGoodFrames_ = goodFramesTemp[0];

    input.close();
}

// Load event data
void NeXuSFile::loadEventData()
{
    printf("Load event data...\n");

    // Open our Nexus file in read only mode.
    H5::H5File input = H5::H5File(filename_, H5F_ACC_RDONLY);

    // Read in event indices.
    auto &&[eventIndicesID, eventIndicesDimension] =
        NeXuSFile::find1DDataset(input, "raw_data_1/detector_1_events", "event_id");
    eventIndices_.resize(eventIndicesDimension);
    H5Dread(eventIndicesID.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, eventIndices_.data());

    // Read in events.
    auto &&[eventTimesID, eventTimesDimension] =
        NeXuSFile::find1DDataset(input, "raw_data_1/detector_1_events", "event_time_offset");
    eventTimes_.resize(eventTimesDimension);
    H5Dread(eventTimesID.getId(), H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, eventTimes_.data());

    // Read in event counts per frame
    auto &&[eventsPerFrameID, eventsPerFrameDimension] =
        NeXuSFile::find1DDataset(input, "raw_data_1/framelog/events_log", "value");
    eventsPerFrame_.resize(eventsPerFrameDimension);
    H5Dread(eventsPerFrameID.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, eventsPerFrame_.data());

    // Read in frame offsets.
    auto &&[frameOffsetsID, frameOffsetsDimension] =
        NeXuSFile::find1DDataset(input, "raw_data_1/detector_1_events", "event_time_zero");
    frameOffsets_.resize(frameOffsetsDimension);
    H5Dread(frameOffsetsID.getId(), H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, frameOffsets_.data());

    input.close();
}

// Load start/end times
void NeXuSFile::loadTimes()
{
    printf("Load times....\n");

    // Open our Nexus file in read only mode.
    H5::H5File input = H5::H5File(filename_, H5F_ACC_RDONLY);

    // Read in start time in Unix time.
    hid_t memType = H5Tcopy(H5T_C_S1);
    H5Tset_size(memType, UCHAR_MAX);
    char timeBuffer[UCHAR_MAX];
    int y = 0, M = 0, d = 0, h = 0, m = 0, s = 0;

    auto &&[startTimeID, startTimeDimension] = NeXuSFile::find1DDataset(input, "raw_data_1", "start_time");
    H5Dread(startTimeID.getId(), memType, H5S_ALL, H5S_ALL, H5P_DEFAULT, timeBuffer);

    sscanf(timeBuffer, "%d-%d-%dT%d:%d:%d", &y, &M, &d, &h, &m, &s);
    std::tm stime = {0};
    stime.tm_year = y - 1900;
    stime.tm_mon = M - 1;
    stime.tm_mday = d;
    stime.tm_hour = h;
    stime.tm_min = m;
    stime.tm_sec = s;
    startSinceEpoch_ = (int)mktime(&stime);

    // Read in end time in Unix time.
    auto &&[endTimeID, endTimeDimension] = NeXuSFile::find1DDataset(input, "raw_data_1", "end_time");
    H5Dread(endTimeID.getId(), memType, H5S_ALL, H5S_ALL, H5P_DEFAULT, timeBuffer);

    sscanf(timeBuffer, "%d-%d-%dT%d:%d:%d", &y, &M, &d, &h, &m, &s);
    std::tm etime = {0};
    etime.tm_year = y - 1900;
    etime.tm_mon = M - 1;
    etime.tm_mday = d;
    etime.tm_hour = h;
    etime.tm_min = m;
    etime.tm_sec = s;

    endSinceEpoch_ = (int)mktime(&etime);

    input.close();
}

// Save key modified data back to the file
bool NeXuSFile::saveModifiedData()
{
    // Open Nexus file in read/write mode.
    H5::H5File output = H5::H5File(filename_, H5F_ACC_RDWR);

    // Write good frames
    std::array<int, 1> framesBuffer;
    framesBuffer[0] = nDetectorFrames_;
    auto &&[goodFrames, goodFramesDimension] = NeXuSFile::find1DDataset(output, "raw_data_1", "good_frames");
    goodFrames.write(framesBuffer.data(), H5::PredType::STD_I32LE);

    // Write monitors
    for (auto &&[index, counts] : monitorCounts_)
    {
        auto &&[monitorCounts, monitorCountsDimension] =
            NeXuSFile::find1DDataset(output, "raw_data_1/monitor_" + std::to_string(index), "data");
        monitorCounts.write(counts.data(), H5::PredType::STD_I32LE);
    }

    // Write detector counts
    const int nSpec = spectra_.size();
    const int nTofBins = tofBins_.size() - 1;

    auto *countsBuffer = new int[nSpec * nTofBins]; // HDF5 expects contiguous memory. This is a pain.
    for (auto i = 0; i < nSpec; ++i)
        for (auto j = 0; j < nTofBins; ++j)
            countsBuffer[i * nTofBins + j] = gsl_histogram_get(detectorHistograms_[spectra_[i]], j);
    auto &&[counts, detectorCountsDimension] = NeXuSFile::find1DDataset(output, "raw_data_1/detector_1", "counts");
    counts.write(countsBuffer, H5::PredType::STD_I32LE);

    delete[](countsBuffer);

    output.close();

    return true;
}

/*
 * Data
 */

int NeXuSFile::nGoodFrames() const { return nGoodFrames_; }
int NeXuSFile::nMonitorFrames() const { return nMonitorFrames_; }
int NeXuSFile::nDetectorFrames() const { return nDetectorFrames_; }
void NeXuSFile::incrementDetectorFrameCount(int delta) { nDetectorFrames_ += delta; }
int NeXuSFile::startSinceEpoch() const { return startSinceEpoch_; }
int NeXuSFile::endSinceEpoch() const { return endSinceEpoch_; }
const std::vector<int> &NeXuSFile::eventIndices() const { return eventIndices_; }
const std::vector<double> &NeXuSFile::eventTimes() const { return eventTimes_; }
const std::vector<int> &NeXuSFile::eventsPerFrame() const { return eventsPerFrame_; }
const std::vector<double> &NeXuSFile::frameOffsets() const { return frameOffsets_; }
const std::vector<double> &NeXuSFile::tofBins() const { return tofBins_; }
const std::map<int, std::vector<int>> &NeXuSFile::monitorCounts() const { return monitorCounts_; }
std::map<unsigned int, gsl_histogram *> &NeXuSFile::detectorHistograms() { return detectorHistograms_; }
const std::map<unsigned int, std::vector<double>> &NeXuSFile::partitions() const { return partitions_; }

/*
 * Manipulation
 */

// Scale monitors by specified factor
void NeXuSFile::scaleMonitors(double factor)
{
    for (auto &pair : monitorCounts_)
    {
        for (auto &bin : pair.second)
        {
            bin *= factor;
        }
    }

    nMonitorFrames_ *= factor;
    fmt::print(" ... New number of effective contributing monitor frames is {}.\n", nMonitorFrames_);
}

// Scale detectors by specified factor
void NeXuSFile::scaleDetectors(double factor)
{
    for (auto i : spectra_)
        gsl_histogram_scale(detectorHistograms_[i], factor);

    nDetectorFrames_ *= factor;
    fmt::print(" ... New number of effective contributing detector frames is {}.\n", nDetectorFrames_);
}
