AUTOMAKE_OPTIONS = subdir-objects


if UseSmartMPI
lib_LIBRARIES = libsmartmpi.a libsmartmpi_debug.a
endif


SmartMPI_SOURCES = \
  smartmpi.cpp \
  GlobalStatistics.cpp
  

libsmartmpi_a_CXXFLAGS        = -DSmartMPIDebug=1
libsmartmpi_debug_a_CXXFLAGS  = -DSmartMPIDebug=4

libsmartmpi_a_SOURCES          = $(SmartMPI_SOURCES)
libsmartmpi_debug_a_SOURCES    = $(SmartMPI_SOURCES)
