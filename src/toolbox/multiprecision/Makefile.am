AUTOMAKE_OPTIONS = subdir-objects


lib_LIBRARIES = libToolboxMultiprecision.a libToolboxMultiprecision_debug.a libToolboxMultiprecision_trace.a


ToolboxMultiprecision_SOURCES = \
  CompressedFloatingPointNumbers.cpp \
  tests/CompressedFloatingPointNumbersTest.cpp
  

libToolboxMultiprecision_a_CXXFLAGS       = -DDimensions=2 -DPeanoDebug=0 
libToolboxMultiprecision_trace_a_CXXFLAGS = -DDimensions=2 -DPeanoDebug=1 -g3 -O0 
libToolboxMultiprecision_debug_a_CXXFLAGS = -DDimensions=2 -DPeanoDebug=4 -g3 -O0 

libToolboxMultiprecision_a_SOURCES       = $(ToolboxMultiprecision_SOURCES)
libToolboxMultiprecision_trace_a_SOURCES = $(ToolboxMultiprecision_SOURCES)
libToolboxMultiprecision_debug_a_SOURCES = $(ToolboxMultiprecision_SOURCES)

