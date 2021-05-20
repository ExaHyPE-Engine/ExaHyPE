AUTOMAKE_OPTIONS = subdir-objects


if UseToolboxMultiprecision
lib_LIBRARIES = libToolboxMultiprecision.a libToolboxMultiprecision_debug.a libToolboxMultiprecision_trace.a
endif


ToolboxMultiprecision_SOURCES = \
  CompressedFloatingPointNumbers.cpp \
  tests/CompressedFloatingPointNumbersTest.cpp
  

libToolboxMultiprecision_a_CXXFLAGS       = $(CXXFLAGS_PEANO_2D_RELEASE) 
libToolboxMultiprecision_trace_a_CXXFLAGS = $(CXXFLAGS_PEANO_2D_TRACE) 
libToolboxMultiprecision_debug_a_CXXFLAGS = $(CXXFLAGS_PEANO_2D_DEBUG) 

libToolboxMultiprecision_a_SOURCES       = $(ToolboxMultiprecision_SOURCES)
libToolboxMultiprecision_trace_a_SOURCES = $(ToolboxMultiprecision_SOURCES)
libToolboxMultiprecision_debug_a_SOURCES = $(ToolboxMultiprecision_SOURCES)

