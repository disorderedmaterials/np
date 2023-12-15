add_library(nexusprocess
  nexusFile.cpp
  processForwardsSummed.cpp
  window.cpp
  nexusFile.h
  processors.h
  window.h
)

target_include_directories(nexusprocess PRIVATE ${PROJECT_SOURCE_DIR}/src ${CONAN_INCLUDE_DIRS})

