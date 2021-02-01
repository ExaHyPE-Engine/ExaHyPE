AUTOMAKE_OPTIONS = subdir-objects


lib_LIBRARIES = libsmartmpi.a libsmartmpi_debug.a


SmartMPI_SOURCES = \
  smartmpi.cpp \
  GlobalStatistics.cpp
  

libsmartmpi_a_CXXFLAGS        = -DSmartMPIDebug=0 
libsmartmpi_debug_a_CXXFLAGS  = -DSmartMPIDebug=4

libsmartmpi_a_SOURCES          = $(SmartMPI_SOURCES)
libsmartmpi_debug_a_SOURCES    = $(SmartMPI_SOURCES)
