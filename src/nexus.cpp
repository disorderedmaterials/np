#include <nexus.hpp>

#include <algorithm>
#include <ctime>
#include <iostream>
#include <fstream>

bool Nexus::getLeafDataset(H5::H5File file, std::vector<H5std_string> terminals, H5std_string dataset, H5::DataSet &out) {

    H5::Group group;

    try {
        bool root = true;
        for (const auto terminal : terminals) {
            if (root) {
                root = false;
                if (!file.nameExists(terminal))
                    return false;
                group = file.openGroup(terminal);
            }
            else {
                if (!group.nameExists(terminal))
                    return false;
                group = group.openGroup(terminal);
            }
        }
        if (!group.nameExists(dataset))
            return false;
        out = group.openDataSet(dataset);
        return true;
    } catch (...) {
        return false;
    }
}

bool Nexus::load(bool advanced) {

    try {

        // Open Nexus file in read only mode.
        H5::H5File file = H5::H5File(path, H5F_ACC_RDONLY);

        // Read in spectra.
        H5::DataSet spectra_;
        if (!Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1", "detector_1"}, "spectrum_index", spectra_))
            return false;
        H5::DataSpace spectraSpace = spectra_.getSpace();
        hsize_t spectraNDims = spectraSpace.getSimpleExtentNdims();
        hsize_t* spectraDims = new hsize_t[spectraNDims];
        spectraSpace.getSimpleExtentDims(spectraDims);

        spectra.resize(spectraDims[0]);
        H5Dread(spectra_.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, spectra.data());

        // Read in raw frames.
        H5::DataSet rawFrames_;
        if (!Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1"}, "raw_frames", rawFrames_))
            return false;
        H5::DataSpace rawFramesSpace = rawFrames_.getSpace();
        hsize_t rawFramesNDims = rawFramesSpace.getSimpleExtentNdims();
        hsize_t* rawFramesDims = new hsize_t[rawFramesNDims];
        rawFramesSpace.getSimpleExtentDims(rawFramesDims);

        rawFrames = new int[(long int) rawFramesDims[0]];
        H5Dread(rawFrames_.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, rawFrames);

        // Read in start time in Unix time.
        hid_t memType = H5Tcopy(H5T_C_S1);
        H5Tset_size(memType, UCHAR_MAX);

        H5::DataSet startTime_;
        if (!Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1"}, "start_time", startTime_))
            return false;

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

        // Read in end time in Unix time.
        H5::DataSet endTime_;
        if (!Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1"}, "end_time", endTime_))
            return false;

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

        if (advanced) {

            // Read in event indices.
            H5::DataSet eventIndices_;
            if (!Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1", "detector_1_events"}, "event_id", eventIndices_))
                return false;
            H5::DataSpace eventIndicesSpace = eventIndices_.getSpace();
            hsize_t eventIndicesNDims = eventIndicesSpace.getSimpleExtentNdims();
            hsize_t* eventIndicesDims = new hsize_t[eventIndicesNDims];
            eventIndicesSpace.getSimpleExtentDims(eventIndicesDims);

            eventIndices.resize(eventIndicesDims[0]);
            H5Dread(eventIndices_.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, eventIndices.data());

            // Read in events.
            H5::DataSet events_;
            if (!Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1", "detector_1_events"}, "event_time_offset", events_))
                return false;
            H5::DataSpace eventsSpace = events_.getSpace();
            hsize_t eventsNDims = eventsSpace.getSimpleExtentNdims();
            hsize_t* eventsDims = new hsize_t[eventsNDims];
            eventsSpace.getSimpleExtentDims(eventsDims);

            events.resize(eventsDims[0]);
            H5Dread(events_.getId(), H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, events.data());

            // Read in frame indices.
            H5::DataSet frameIndices_;
            if (!Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1", "detector_1_events"}, "event_index", frameIndices_))
                return false;
            H5::DataSpace frameIndicesSpace = frameIndices_.getSpace();
            hsize_t frameIndicesNDims = frameIndicesSpace.getSimpleExtentNdims();
            hsize_t* frameIndicesDims = new hsize_t[frameIndicesNDims];
            frameIndicesSpace.getSimpleExtentDims(frameIndicesDims);

            frameIndices.resize(frameIndicesDims[0]);
            H5Dread(frameIndices_.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, frameIndices.data());

            // Read in frame offsets.
            H5::DataSet frameOffsets_;
            if (!Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1", "detector_1_events"}, "event_time_zero", frameOffsets_))
                return false;
            H5::DataSpace frameOffsetsSpace = frameOffsets_.getSpace();
            hsize_t frameOffsetsNDims = frameOffsetsSpace.getSimpleExtentNdims();
            hsize_t* frameOffsetsDims = new hsize_t[frameOffsetsNDims];
            frameOffsetsSpace.getSimpleExtentDims(frameOffsetsDims);

            frameOffsets.resize(frameOffsetsDims[0]);
            H5Dread(frameOffsets_.getId(), H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, frameOffsets.data());

            // Read in bin information.
            H5::DataSet bins_;
            if (!Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1", "monitor_1"}, "time_of_flight", bins_))
                return false;
            H5::DataSpace binsSpace = bins_.getSpace();
            hsize_t binsNDims = binsSpace.getSimpleExtentNdims();
            hsize_t* binsDims = new hsize_t[binsNDims];
            binsSpace.getSimpleExtentDims(binsDims);

            ranges.resize(binsDims[0]);
            H5Dread(bins_.getId(), H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, ranges.data());

            // Read in monitor data.

            int i = 1;
            bool result = true;
            while (true) {

                H5::DataSet monitor;
                if (!Nexus::getLeafDataset(file, std::vector<H5std_string> {"raw_data_1", "monitor_" + std::to_string(i)}, "data", monitor)) {
                    break;
                }
                H5::DataSpace monitorSpace = monitor.getSpace();
                hsize_t monitorNDims = monitorSpace.getSimpleExtentNdims();
                hsize_t* monitorDims = new hsize_t[monitorNDims];
                monitorSpace.getSimpleExtentDims(monitorDims);

                std::vector<int> monitorVec;
                monitorVec.resize(monitorDims[2]);
                H5Dread(monitor.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, monitorVec.data());
                monitors[i++] = monitorVec;
            }

        }

        // Close file.
        file.close();
        return true;

    } catch (...) {
        file.close();
        return false;
    }
    return true;

}

bool Nexus::createHistogram(Pulse &pulse, int epochOffset) {
    for (auto spec: spectra) {
        histogram[spec] = gsl_histogram_alloc(ranges.size()-1);
        gsl_histogram_set_ranges(histogram[spec], ranges.data(), ranges.size());
    }

    for (int i=0; i<events.size(); ++i) {
        if ((events[i] >= pulse.start-epochOffset) && (events[i] < pulse.end-epochOffset)) {
            if (eventIndices[i] > 0) {
                gsl_histogram_increment(histogram[eventIndices[i]], events[i]);
            }
        }
    }

    return true;
}

bool Nexus::createHistogram(Pulse &pulse, std::map<unsigned int, gsl_histogram*> &mask, int epochOffset) {

    histogram = mask;

    for (int i=0; i<events.size(); ++i) {
        if ((events[i] >= pulse.start-epochOffset) && (events[i] < pulse.end-epochOffset)) {
            if (eventIndices[i] > 0) {
                gsl_histogram_increment(histogram[eventIndices[i]], events[i]);
            }
        }
    }

    return true;
}

int Nexus::countGoodFrames(Pulse &pulse, int epochOffset) {
    int goodFrames = 0;

    for (int i =0; i<frameIndices.size()-1; ++i) {
        int frameStart = frameIndices[i];
        int frameEnd = frameIndices[i+1];
        double frameZero = frameOffsets[i];
        if ((frameZero >= pulse.start-epochOffset) && (frameZero < pulse.end-epochOffset))
            goodFrames++;
    }
    return goodFrames;

}

bool Nexus::output(std::vector<std::string> paths) {

    try {
        // Open Nexus file in read only mode.
        H5::H5File input = H5::H5File(path, H5F_ACC_RDONLY);
        
        // Create new Nexus file for output.
        H5::H5File output = H5::H5File(outpath, H5F_ACC_TRUNC);

        // Perform copying.
        if (!Nexus::copy(input, output, paths))
            return false;

        input.close();

        writeCounts(output);
        output.close();
        return true;

    } catch (...) {
        return false;
    }

}

bool Nexus::output(std::vector<std::string> paths, int goodFrames, std::map<int, std::vector<int>> monitors) {

    try {
        // Open Nexus file in read only mode.
        H5::H5File input = H5::H5File(path, H5F_ACC_RDONLY);
        
        // Create new Nexus file for output.
        H5::H5File output = H5::H5File(outpath, H5F_ACC_TRUNC);

        // Perform copying.
        if (!Nexus::copy(input, output, paths))
            return false;

        input.close();

        writeCounts(output);
        writeGoodFrames(output, goodFrames);
        writeMonitors(output, monitors);
        output.close();
        return true;

    } catch (...) {
        return false;
    }

}

bool Nexus::copy(H5::H5File input, H5::H5File output, std::vector<std::string> paths) {
    hid_t ocpl_id, lcpl_id;
    ocpl_id = H5Pcreate(H5P_OBJECT_COPY);
    if (ocpl_id < 0)
        return false;
    lcpl_id = H5Pcreate(H5P_LINK_CREATE);
    if (lcpl_id <0)
        return false;
    if (H5Pset_create_intermediate_group(lcpl_id, 1) < 0)
        return false;

    for (const auto &p : paths) {
        std::cout << p << std::endl;
        if (H5Ocopy(input.getId(), p.c_str(), output.getId(), p.c_str(), ocpl_id, lcpl_id) < 0)
            return false;
    }

    H5Pclose(ocpl_id);
    H5Pclose(lcpl_id);
    
    return true;
}

bool Nexus::writeCounts(H5::H5File output) {

    // Write out histogram.
    const int nSpec = spectra.size();
    const int nBin = ranges.size()-1;

    int* buf = new int[1*nSpec*nBin]; // HDF5 expects contiguous memory. This is a pain.
    int i;
    for (int i=0; i<1; ++i)
        for (int j=0; j<nSpec; ++j)
            for (int k=0; k<nBin; ++k) {
                buf[(i*nSpec+j)*nBin+k] = gsl_histogram_get(histogram[spectra[j]], k);
            }

    H5::DataSet counts;
    Nexus::getLeafDataset(output, std::vector<H5std_string> {"raw_data_1", "detector_1"}, "counts", counts);

    counts.write(buf, H5::PredType::STD_I32LE);
    
    return true;

}

bool Nexus::writeGoodFrames(H5::H5File output, int goodFrames) {

    int* buf = new int[1];
    buf[0] = goodFrames;

    H5::DataSet goodFrames_;
    Nexus::getLeafDataset(output, std::vector<H5std_string> {"raw_data_1"}, "good_frames", goodFrames_);

    goodFrames_.write(buf, H5::PredType::STD_I32LE);
    
    return true;

}

bool Nexus::writeMonitors(H5::H5File output, std::map<int, std::vector<int>> monitors) {

    for (auto pair : monitors) {

        int* buf = new int[1*1*pair.second.size()];
        for (int i=0; i<1; ++i)
            for (int j=0; j<1; ++j)
                for (int k=0; k<pair.second.size(); ++k)
                    buf[i*j*k] = pair.second[k];

        H5::DataSet monitor;
        Nexus::getLeafDataset(output, std::vector<H5std_string> {"raw_data_1", "monitor_" + std::to_string(pair.first)}, "data", monitor);
        monitor.write(buf, H5::PredType::STD_I32LE);
    }
    return true;

}