#include "nexus.hpp"
#include <iostream>
#include <fstream>
#include <ctime>
#include <algorithm>

bool Nexus::getLeafDataset(H5::H5File file, std::vector<H5std_string> terminals, H5std_string dataset, H5::DataSet &out) {

    H5::Group group;

    try {
        bool root = true;
        for (const auto terminal : terminals) {
            if (root) {
                root = false;
                group = file.openGroup(terminal);
            }
            else {
                group = group.openGroup(terminal);
            }
        }
        out = group.openDataSet(dataset);
        return true;
    } catch (...) {
        return false;
    }
}

bool Nexus::loadBasicData() {

    H5::H5File file = H5::H5File(path, H5F_ACC_RDONLY);

    // First get spectra

    std::cout << "Getting spectra info.." << std::endl;
    H5::DataSet spectra_;
    Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1", "detector_1"}, "spectrum_index", spectra_);
    H5::DataSpace spectraSpace = spectra_.getSpace();
    hsize_t spectraNDims = spectraSpace.getSimpleExtentNdims();
    hsize_t* spectraDims = new hsize_t[spectraNDims];
    spectraSpace.getSimpleExtentDims(spectraDims);

    // data.spectra = new int[(long int) spectraDims[0]];
    spectra.resize(spectraDims[0]);
    H5Dread(spectra_.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, spectra.data());
    std::cout << "Got spectra info!" << std::endl;

    // Now get rawframes

    std::cout << "Getting frame info.." << std::endl;
    H5::DataSet rawFrames_;
    Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1"}, "raw_frames", rawFrames_);
    H5::DataSpace rawFramesSpace = rawFrames_.getSpace();
    hsize_t rawFramesNDims = rawFramesSpace.getSimpleExtentNdims();
    hsize_t* rawFramesDims = new hsize_t[rawFramesNDims];
    rawFramesSpace.getSimpleExtentDims(rawFramesDims);

    rawFrames = new int[(long int) rawFramesDims[0]];
    H5Dread(rawFrames_.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, rawFrames);
    std::cout << "Got frame info!" << std::endl;

    // Start time

    hid_t memType = H5Tcopy(H5T_C_S1);
    H5Tset_size(memType, UCHAR_MAX);

    std::cout << "Getting start time.." << std::endl;
    H5::DataSet startTime_;
    Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1"}, "start_time", startTime_);

    char startTimeBuffer[UCHAR_MAX];
    H5Dread(startTime_.getId(), memType, H5S_ALL, H5S_ALL, H5P_DEFAULT, startTimeBuffer);

    int y=0, M=0, d=0, h=0, m=0, s=0;
    sscanf(startTimeBuffer, "%d-%d-%dT%d:%d:%d", &y, &M, &d, &h, &m, &s);

    std::tm stime = {0};
    stime.tm_year = y - 1900;
    stime.tm_mon = M - 1;
    stime.tm_mday = d;
    stime.tm_hour = h;
    stime.tm_min = m;
    stime.tm_sec = s;

    startSinceEpoch = (int) mktime(&stime);

    std::cout << "Got start time!" << " " << startSinceEpoch << std::endl;

    // End time

    std::cout << "Getting end time.." << std::endl;
    H5::DataSet endTime_;
    Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1"}, "end_time", endTime_);

    char endTimeBuffer[UCHAR_MAX];
    H5Dread(endTime_.getId(), memType, H5S_ALL, H5S_ALL, H5P_DEFAULT, endTimeBuffer);

    sscanf(endTimeBuffer, "%d-%d-%dT%d:%d:%d", &y, &M, &d, &h, &m, &s);

    std::tm etime = {0};
    etime.tm_year = y - 1900;
    etime.tm_mon = M - 1;
    etime.tm_mday = d;
    etime.tm_hour = h;
    etime.tm_min = m;
    etime.tm_sec = s;

    endSinceEpoch = (int) mktime(&etime);

    std::cout << "Got end time!" << " " << endSinceEpoch << std::endl;

    file.close();

    return true;
}

bool Nexus::loadEventModeData() {

    H5::H5File file = H5::H5File(path, H5F_ACC_RDONLY);


    // Now get event indices, this is where stuff gets dangerous

    std::cout << "Getting event indices info.." << std::endl;
    H5::DataSet eventIndices_;
    Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1", "detector_1_events"}, "event_id", eventIndices_);
    H5::DataSpace eventIndicesSpace = eventIndices_.getSpace();
    hsize_t eventIndicesNDims = eventIndicesSpace.getSimpleExtentNdims();
    hsize_t* eventIndicesDims = new hsize_t[eventIndicesNDims];
    eventIndicesSpace.getSimpleExtentDims(eventIndicesDims);

    eventIndices.resize(eventIndicesDims[0]);
    H5Dread(eventIndices_.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, eventIndices.data());
    std::cout << "Got event indices info!" << std::endl;


    // Now get events

    std::cout << "Getting event info.." << std::endl;
    H5::DataSet events_;
    Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1", "detector_1_events"}, "event_time_offset", events_);
    H5::DataSpace eventsSpace = events_.getSpace();
    hsize_t eventsNDims = eventsSpace.getSimpleExtentNdims();
    hsize_t* eventsDims = new hsize_t[eventsNDims];
    eventsSpace.getSimpleExtentDims(eventsDims);

    events.resize(eventsDims[0]);
    H5Dread(events_.getId(), H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, events.data());
    std::cout << "Got event info!" << std::endl;

    // Frame indices

    std::cout << "Getting frame indices info.." << std::endl;
    H5::DataSet frameIndices_;
    Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1", "detector_1_events"}, "event_index", frameIndices_);
    H5::DataSpace frameIndicesSpace = frameIndices_.getSpace();
    hsize_t frameIndicesNDims = frameIndicesSpace.getSimpleExtentNdims();
    hsize_t* frameIndicesDims = new hsize_t[frameIndicesNDims];
    frameIndicesSpace.getSimpleExtentDims(frameIndicesDims);

    frameIndices.resize(frameIndicesDims[0]);
    H5Dread(frameIndices_.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, frameIndices.data());
    std::cout << "Got frame indices info!" << std::endl;

    // Frame offsets

    std::cout << "Getting frame offsets info.." << std::endl;
    H5::DataSet frameOffsets_;
    Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1", "detector_1_events"}, "event_time_zero", frameOffsets_);
    H5::DataSpace frameOffsetsSpace = frameOffsets_.getSpace();
    hsize_t frameOffsetsNDims = frameOffsetsSpace.getSimpleExtentNdims();
    hsize_t* frameOffsetsDims = new hsize_t[frameOffsetsNDims];
    frameOffsetsSpace.getSimpleExtentDims(frameOffsetsDims);

    frameOffsets.resize(frameOffsetsDims[0]);
    H5Dread(frameOffsets_.getId(), H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, frameOffsets.data());
    std::cout << "Got frame offsets info!" << std::endl;

    // bins

    std::cout << "Getting bins info.." << std::endl;
    H5::DataSet bins_;
    Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1", "monitor_1"}, "time_of_flight", bins_);
    H5::DataSpace binsSpace = bins_.getSpace();
    hsize_t binsNDims = binsSpace.getSimpleExtentNdims();
    hsize_t* binsDims = new hsize_t[binsNDims];
    binsSpace.getSimpleExtentDims(binsDims);

    std::vector<double> binBoundaries;
    binBoundaries.resize(binsDims[0]);
    H5Dread(bins_.getId(), H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, binBoundaries.data());
    // std::cout << "Got bins info!" << std::endl;

    for (int i=0; i<binBoundaries.size()-1; ++i) {
        bins.push_back(std::make_pair(binBoundaries[i], binBoundaries[i+1]));
    }

    file.close();

    return true;

}

bool Nexus::parseNexusFile() {

    H5::H5File file = H5::H5File(path, H5F_ACC_RDONLY);

    // First get spectra

    std::cout << "Getting spectra info.." << std::endl;
    H5::DataSet spectra_;
    Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1", "detector_1"}, "spectrum_index", spectra_);
    H5::DataSpace spectraSpace = spectra_.getSpace();
    hsize_t spectraNDims = spectraSpace.getSimpleExtentNdims();
    hsize_t* spectraDims = new hsize_t[spectraNDims];
    spectraSpace.getSimpleExtentDims(spectraDims);

    // data.spectra = new int[(long int) spectraDims[0]];
    spectra.resize(spectraDims[0]);
    H5Dread(spectra_.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, spectra.data());
    std::cout << "Got spectra info!" << std::endl;

    // Now get rawframes

    std::cout << "Getting frame info.." << std::endl;
    H5::DataSet rawFrames_;
    Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1"}, "raw_frames", rawFrames_);
    H5::DataSpace rawFramesSpace = rawFrames_.getSpace();
    hsize_t rawFramesNDims = rawFramesSpace.getSimpleExtentNdims();
    hsize_t* rawFramesDims = new hsize_t[rawFramesNDims];
    rawFramesSpace.getSimpleExtentDims(rawFramesDims);

    rawFrames = new int[(long int) rawFramesDims[0]];
    H5Dread(rawFrames_.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, rawFrames);
    std::cout << "Got frame info!" << std::endl;


    // Now get event indices, this is where stuff gets dangerous

    std::cout << "Getting event indices info.." << std::endl;
    H5::DataSet eventIndices_;
    Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1", "detector_1_events"}, "event_id", eventIndices_);
    H5::DataSpace eventIndicesSpace = eventIndices_.getSpace();
    hsize_t eventIndicesNDims = eventIndicesSpace.getSimpleExtentNdims();
    hsize_t* eventIndicesDims = new hsize_t[eventIndicesNDims];
    eventIndicesSpace.getSimpleExtentDims(eventIndicesDims);

    eventIndices.resize(eventIndicesDims[0]);
    H5Dread(eventIndices_.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, eventIndices.data());
    std::cout << "Got event indices info!" << std::endl;


    // Now get events

    std::cout << "Getting event info.." << std::endl;
    H5::DataSet events_;
    Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1", "detector_1_events"}, "event_time_offset", events_);
    H5::DataSpace eventsSpace = events_.getSpace();
    hsize_t eventsNDims = eventsSpace.getSimpleExtentNdims();
    hsize_t* eventsDims = new hsize_t[eventsNDims];
    eventsSpace.getSimpleExtentDims(eventsDims);

    events.resize(eventsDims[0]);
    H5Dread(events_.getId(), H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, events.data());
    std::cout << "Got event info!" << std::endl;

    // Frame indices

    std::cout << "Getting frame indices info.." << std::endl;
    H5::DataSet frameIndices_;
    Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1", "detector_1_events"}, "event_index", frameIndices_);
    H5::DataSpace frameIndicesSpace = frameIndices_.getSpace();
    hsize_t frameIndicesNDims = frameIndicesSpace.getSimpleExtentNdims();
    hsize_t* frameIndicesDims = new hsize_t[frameIndicesNDims];
    frameIndicesSpace.getSimpleExtentDims(frameIndicesDims);

    frameIndices.resize(frameIndicesDims[0]);
    H5Dread(frameIndices_.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, frameIndices.data());
    std::cout << "Got frame indices info!" << std::endl;

    // Frame offsets

    std::cout << "Getting frame offsets info.." << std::endl;
    H5::DataSet frameOffsets_;
    Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1", "detector_1_events"}, "event_time_zero", frameOffsets_);
    H5::DataSpace frameOffsetsSpace = frameOffsets_.getSpace();
    hsize_t frameOffsetsNDims = frameOffsetsSpace.getSimpleExtentNdims();
    hsize_t* frameOffsetsDims = new hsize_t[frameOffsetsNDims];
    frameOffsetsSpace.getSimpleExtentDims(frameOffsetsDims);

    frameOffsets.resize(frameOffsetsDims[0]);
    H5Dread(frameOffsets_.getId(), H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, frameOffsets.data());
    std::cout << "Got frame offsets info!" << std::endl;

    // bins

    std::cout << "Getting bins info.." << std::endl;
    H5::DataSet bins_;
    Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1", "monitor_1"}, "time_of_flight", bins_);
    H5::DataSpace binsSpace = bins_.getSpace();
    hsize_t binsNDims = binsSpace.getSimpleExtentNdims();
    hsize_t* binsDims = new hsize_t[binsNDims];
    binsSpace.getSimpleExtentDims(binsDims);

    std::vector<double> binBoundaries;
    binBoundaries.resize(binsDims[0]);
    H5Dread(bins_.getId(), H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, binBoundaries.data());
    // std::cout << "Got bins info!" << std::endl;

    for (int i=0; i<binBoundaries.size()-1; ++i) {
        bins.push_back(std::make_pair(binBoundaries[i], binBoundaries[i+1]));
    }

    file.close();

    return true;
}

bool Nexus::createHistogram() {
    std::cout << "Setting up histogram.." << std::endl;
    for (auto spec: spectra) {
        std::vector<unsigned int> spectraVec(bins.size(), 0);
        histogram[spec] = spectraVec;
    }
    std::vector<unsigned int> spectraVec(bins.size(), 0);
    histogram[0] = spectraVec;
    std::cout << "Empty histogram created!" << std::endl;

    std::cout << "Starting to bin events." << std::endl;
    for (int i=0; i<events.size(); ++i) {
        for (int j=0; j<bins.size(); ++j) {
            if ((events[i] >= bins[j].first) && (events[i] < bins[j].second)) {
                histogram[eventIndices[i]][j]++;
                break;
            }
        }
    }
    std::cout << "Finished binning events." << std::endl;
    return true;
}

bool Nexus::createHistogram(std::vector<std::pair<double, double>> bounds) {

    std::cout << "Setting up histogram.." << std::endl;
    for (auto spec: spectra) {
        std::vector<unsigned int> spectraVec(bins.size(), 0);
        histogram[spec] = spectraVec;
    }
    std::vector<unsigned int> spectraVec(bins.size(), 0);
    histogram[0] = spectraVec;
    std::cout << "Empty histogram created!" << std::endl;

    std::cout << "Starting to bin events." << std::endl;
    for (int i=0; i<events.size(); ++i) {
        bool match= false;
        for (int j=0; j<bounds.size(); ++j) {
            if ((events[i] >= bounds[j].first) && (events[i] < bounds[j].second)) {
                match = true;
                break;
            }
        }
        if (!match) continue;
        for (int j=0; j<bins.size(); ++j) {
            if ((events[i] >= bins[j].first) && (events[i] < bins[j].second)) {
                histogram[eventIndices[i]][j]++;
                break;
            }
        }
    }
    std::cout << "Finished binning events." << std::endl;
    return true;

}

bool Nexus::createHistogram(std::pair<double, double> bounds) {

    std::cout << "Setting up histogram.." << std::endl;
    for (auto spec: spectra) {
        std::vector<unsigned int> spectraVec(bins.size(), 0);
        histogram[spec] = spectraVec;
    }
    std::vector<unsigned int> spectraVec(bins.size(), 0);
    histogram[0] = spectraVec;
    std::cout << "Empty histogram created!" << std::endl;

    std::cout << "Starting to bin events." << std::endl;
    for (int i=0; i<events.size(); ++i) {
        bool match= false;
        if ((events[i] >= bounds.first) && (events[i] < bounds.second)) {
            match = true;
        }
        if (!match) continue;
        for (int j=0; j<bins.size(); ++j) {
            if ((events[i] >= bins[j].first) && (events[i] < bins[j].second)) {
                histogram[eventIndices[i]][j]++;
                break;
            }
        }
    }
    std::cout << "Finished binning events." << std::endl;
    return true;

}

bool Nexus::writeCountsHistogram() {

    const int nSpec = spectra.size();
    const int nBin = bins.size()-1;

    int* buf = new int[1*nSpec*nBin]; // HDF5 expects contiguous memory. This is a pain.

    for (int i=0; i<1; ++i)
        for (int j=0; j<nSpec; ++j)
            for (int k=0; k<nBin; ++k)
                buf[(i*nSpec+j)*nBin+k] = histogram[spectra[j]][k]; //buf[i][j][k] = buf[(i*Y+j)*Z+k]

    Nexus::copy();

    H5::H5File file = H5::H5File(outpath, H5F_ACC_RDWR);

    H5::DataSet counts;
    Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1", "detector_1"}, "counts", counts);

    counts.write(buf, H5::PredType::STD_I32LE);
    file.close();
    return true;
}



bool Nexus::extractPulseTimes(int spectrum, std::vector<double> &pulses) {

    for (int i=0; i<events.size(); ++i) {
        double event = events[i];
        int idx = eventIndices[i];
        if (idx != spectrum)
            continue;
        for(int j=0; j<frameIndices.size()-1; j++) {
            if ((i>= frameIndices[j]) && (i< frameIndices[j+1])) {
                pulses.push_back((event*0.000001) + frameOffsets[j]);
                break;
            }
        }
    }
    return true;

}

bool Nexus::extrapolatePulseTimes(double start, bool backwards, bool forwards, double step, double duration, std::vector<std::pair<double, double>> &pulses) {

    double pulse;
    pulses.push_back(std::make_pair(start, start+step));

    if (backwards) {
        pulse = start - step;
        while (pulse > 0) {
            pulses.push_back(std::make_pair(pulse, pulse+duration));
            pulse-=step;
        }
    }
    if (forwards) {
        pulse = start + step;
        while (pulse < (endSinceEpoch-startSinceEpoch)) {
            pulses.push_back(std::make_pair(pulse, pulse+duration));
            pulse+=step;
        }
    }

    std::sort(
        pulses.begin(), pulses.end(),
        [](
            const std::pair<double, double> a,
            const std::pair<double, double> b
            ){
                return a.first < b.first;
            }
    );

    return true;

}

bool Nexus::copy() {
    std::ifstream src(path, std::ios::binary);
    std::ofstream dest(outpath, std::ios::binary);
    dest  << src.rdbuf();
    src.close();
    dest.close();
    return true;
}

