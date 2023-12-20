add_library(nexusProcess
  nexusFile.cpp
  processSummed.cpp
  window.cpp
  nexusFile.h
  processors.h
  window.h
)

target_include_directories(nexusProcess PRIVATE ${PROJECT_SOURCE_DIR}/src ${CONAN_INCLUDE_DIRS})

if(CONAN)
  target_link_libraries(nexusProcess PUBLIC CONAN_PKG::fmt)
else(CONAN)
  target_link_libraries(nexusProcess PUBLIC fmt::fmt)
endif(CONAN)
