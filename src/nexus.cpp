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
        std::cout << path << std::endl;
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

        auto rawFramesTemp = new int[(long int) rawFramesDims[0]];
        H5Dread(rawFrames_.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, rawFramesTemp);
        rawFrames = rawFramesTemp[0];

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
        return false;
    }
    return true;

}

bool Nexus::createHistogram(Pulse &pulse, int epochOffset) {
    int start, end, id;
    double frameZero, event;
    for (auto spec: spectra) {
        histogram[spec] = gsl_histogram_alloc(ranges.size()-1);
        gsl_histogram_set_ranges(histogram[spec], ranges.data(), ranges.size());
    }

    goodFrames = 0;

    for (int i=0; i<frameIndices.size()-1; ++i) {
        start = frameIndices[i];
        end = frameIndices[i+1];
        frameZero = frameOffsets[i];
        if ((frameZero >= (pulse.start-epochOffset)) && (frameZero < (pulse.end-epochOffset))) {
            for (int k=start; k<end; ++k) {
                id = eventIndices[k];
                event = events[k];
                if (id > 0)
                    gsl_histogram_increment(histogram[id], event);
            }
            ++goodFrames;
        }
    }

    std::cout << "There are " << goodFrames << " goodframes!" << std::endl;
    return true;
}

bool Nexus::createHistogram(Pulse &pulse, std::map<unsigned int, gsl_histogram*> &mask, int epochOffset) {

    histogram = mask;
    return createHistogram(pulse, epochOffset);
    
}

void Nexus::binPulseEvents(Pulse &pulse, int epochOffset, Nexus &destination)
{
    // Bin events from this Nexus / pulse into the destination histogram bins
    for (int i=0; i<frameIndices.size()-1; ++i) {
        auto start = frameIndices[i];
        auto end = frameIndices[i+1];
        auto frameZero = frameOffsets[i];
        if ((frameZero >= (pulse.start-epochOffset)) && (frameZero < (pulse.end-epochOffset))) {
            for (int k=start; k<end; ++k) {
                auto id = eventIndices[k];
                auto event = events[k];
                if (id > 0)
                    gsl_histogram_increment(destination.histogram[id], event);
            }
            ++destination.goodFrames;
        }
    }
}

void Nexus::addMonitors(double scale, Nexus &destination)
{
    auto destIt = destination.monitors.begin();
    for (const auto &pair : monitors) {
        auto &destCounts = destIt->second;
        for (int i=0; i<pair.second.size(); ++i) {
            destCounts[i] += (int) (pair.second[i] * scale);
        }
        ++destIt;
    }
}

bool Nexus::output(std::vector<std::string> paths) {

    try {
        // // Open Nexus file in read only mode.
        H5::H5File input = H5::H5File(path, H5F_ACC_RDONLY);
        
        // Create new Nexus file for output.
        H5::H5File output = H5::H5File(outpath, H5F_ACC_TRUNC);

        // Perform copying.
        if (!Nexus::copy(input, output, paths))
            return false;

        input.close();

        writeCounts(output);
        writeMonitors(output, monitors);
        writeGoodFrames(output, goodFrames);
        output.close();
        return true;

    } catch (...) {
        return false;
    }

}

bool Nexus::copy() {
    std::ifstream src(path, std::ios::binary);
    std::ofstream dest(outpath, std::ios::binary);
    dest  << src.rdbuf();
    src.close();
    dest.close();
    return true;
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
        if (H5Ocopy(input.getId(), p.c_str(), output.getId(), p.c_str(), ocpl_id, lcpl_id) < 0)
            return false;
    }

    H5Pclose(ocpl_id);
    H5Pclose(lcpl_id);
    
    return true;
}


bool Nexus::createEmpty(std::vector<std::string> paths)
{
    // Create an empty, minimal Nexus file to sum data into
    try {
        // Open input Nexus file in read only mode.
        H5::H5File input = H5::H5File(path, H5F_ACC_RDONLY);
        
        // Create new Nexus file for output.
        H5::H5File output = H5::H5File(outpath, H5F_ACC_TRUNC);

        printf("Copying source Nexus file to template...\n");
        // Perform copying.
        if (!Nexus::copy(input, output, paths))
            return false;

        // Read in spectra
        H5::DataSet spectra_;
        if (!Nexus::getLeafDataset(input, std::vector<H5std_string> {"raw_data_1", "detector_1"}, "spectrum_index", spectra_))
            return false;
        H5::DataSpace spectraSpace = spectra_.getSpace();
        hsize_t spectraNDims = spectraSpace.getSimpleExtentNdims();
        hsize_t* spectraDims = new hsize_t[spectraNDims];
        spectraSpace.getSimpleExtentDims(spectraDims);
        spectra.resize(spectraDims[0]);
        H5Dread(spectra_.getId(), H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, spectra.data());
        printf("... got spectra.\n");
    
        // Read in bin information.
        H5::DataSet bins_;
        if (!Nexus::getLeafDataset(input, std::vector<H5std_string> {"raw_data_1", "monitor_1"}, "time_of_flight", bins_))
            return false;
        H5::DataSpace binsSpace = bins_.getSpace();
        hsize_t binsNDims = binsSpace.getSimpleExtentNdims();
        hsize_t* binsDims = new hsize_t[binsNDims];
        binsSpace.getSimpleExtentDims(binsDims);
        ranges.resize(binsDims[0]);
        H5Dread(bins_.getId(), H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, ranges.data());
        printf("... got ranges.\n");

        // Read in monitor data.
        int i = 1;
        bool result = true;
        while (true) {

            H5::DataSet monitor;
            if (!Nexus::getLeafDataset(input, std::vector<H5std_string> {"raw_data_1", "monitor_" + std::to_string(i)}, "data", monitor)) {
                break;
            }
            H5::DataSpace monitorSpace = monitor.getSpace();
            hsize_t monitorNDims = monitorSpace.getSimpleExtentNdims();
            hsize_t* monitorDims = new hsize_t[monitorNDims];
            monitorSpace.getSimpleExtentDims(monitorDims);

            std::vector<int> monitorVec;
            monitorVec.resize(monitorDims[2], 0);
            monitors[i++] = monitorVec;
        }

        // Set up 
        for (auto spec: spectra) {
            histogram[spec] = gsl_histogram_alloc(ranges.size()-1);
            gsl_histogram_set_ranges(histogram[spec], ranges.data(), ranges.size());
        }

        goodFrames = 0;

        input.close();
        //output.close();

    } catch (...) {
        return false;
    }

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

bool Nexus::writeTotalFrames(H5::H5File output, int frames) {

    int* buf = new int[1];
    buf[0] = frames;

    H5::DataSet frames_;
    Nexus::getLeafDataset(output, std::vector<H5std_string> {"raw_data_1"}, "total_frames", frames_);

    frames_.write(buf, H5::PredType::STD_I32LE);
    
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

bool Nexus::reduceMonitors(double scale) {
    for (auto &pair : monitors) {
        for (int i=0; i<pair.second.size(); ++i) {
            pair.second[i]= (int) (pair.second[i] * scale);
        }
    }
    return true;
}

bool Nexus::writeMonitors(H5::H5File output, std::map<int, std::vector<int>> monitors) {

    for (auto& pair : monitors) {

        int* buf = new int[1*1*pair.second.size()];
        for (int i=0; i<1; ++i)
            for (int j=0; j<1; ++j)
                for (int k=0; k<pair.second.size(); ++k)
                    buf[(i*1+j)*1+k] = pair.second[k];

        H5::DataSet monitor;
        Nexus::getLeafDataset(output, std::vector<H5std_string> {"raw_data_1", "monitor_" + std::to_string(pair.first)}, "data", monitor);
        monitor.write(buf, H5::PredType::STD_I32LE);
    }
    return true;

}

bool Nexus::writePartitionsWithRelativeTimes(unsigned int lowerSpec, unsigned int higherSpec) {

    if (lowerSpec > higherSpec) {
        std::cerr << "Lower spectrum (" << lowerSpec << ") > higher spectrum (" << higherSpec << ") did you get them the wrong way round?" << std::endl;
        return false;
    }

    double event;
    int idx;
    std::map<unsigned int, std::vector<double>> partitions;
    for (int i=0; i<events.size(); ++i) {
        event = events[i];
        idx = eventIndices[i];
        if (!idx || idx < lowerSpec || idx > higherSpec)
            continue;
        for (int j=0; j<frameIndices.size()-1; ++j) {
            if ((i>= frameIndices[j]) && (i<frameIndices[j+1])) {
                partitions[idx].push_back((event*0.000001) + frameOffsets[j]);
            }
        }
    }

    H5::H5File file(outpath, H5F_ACC_TRUNC);

    for (auto pair : partitions) {
        const int rank = 1;
        hsize_t dims[1];
        dims[0] = pair.second.size();
        H5::DataSpace dataspace(rank, dims);
        H5::DataSet dataset =  file.createDataSet(std::to_string(pair.first).c_str(), H5::PredType::NATIVE_DOUBLE, dataspace);
        dataset.write(pair.second.data(), H5::PredType::NATIVE_DOUBLE);
    }
    file.close();

    return true; 
}
