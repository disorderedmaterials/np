add_library(nexusprocess
  nexusFile.cpp
  processForwardsSummed.cpp
  pulse.cpp
  nexusFile.h
  processors.h
  pulse.h
)

target_include_directories(nexusprocess PRIVATE ${PROJECT_SOURCE_DIR}/src)
