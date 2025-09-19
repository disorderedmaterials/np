add_library(
  nexusProcess
  countDetector.cpp
  countMonitor.cpp
  dumpDetector.cpp
  dumpMonitor.cpp
  eventChunker.cpp
  getEvents.cpp
  histogram.cpp
  nexusFile.cpp
  partitionEventsIndividual.cpp
  partitionEventsSummed.cpp
  processCommon.cpp
  resizeDetectors.cpp
  window.cpp
  eventChunker.h
  histogram.h
  nexusFile.h
  processors.h
  window.h)

target_include_directories(nexusProcess PRIVATE ${PROJECT_SOURCE_DIR} / src
                                                ${CONAN_INCLUDE_DIRS})

if(CONAN)
  target_link_libraries(nexusProcess PUBLIC CONAN_PKG::fmt)
else(CONAN)
  target_link_libraries(nexusProcess PUBLIC fmt::fmt)
endif(CONAN)
