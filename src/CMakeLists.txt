add_library(nexusprocess
  config.cpp
  modex.cpp
  nexus.cpp
  period.cpp
  period_definition.cpp
  config.h
  modex.h
  nexus.h
  period_definition.h
  period.h
  pulse_definition.h
  pulse.h
)

target_include_directories(nexusprocess PRIVATE ${PROJECT_SOURCE_DIR}/src)

