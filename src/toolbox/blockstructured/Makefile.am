AUTOMAKE_OPTIONS = subdir-objects


if UseToolboxBlockstructured
lib_LIBRARIES = libToolboxBlockstructured.a libToolboxBlockstructured_debug.a libToolboxBlockstructured_trace.a libToolboxBlockstructured_stats.a libToolboxBlockstructured_asserts.a
endif


ToolboxBlockstructured_SOURCES = \
  Interpolation.cpp
  

libToolboxBlockstructured_a_CXXFLAGS       = $(CXXFLAGS_PEANO_2D_RELEASE) 
libToolboxBlockstructured_trace_a_CXXFLAGS = $(CXXFLAGS_PEANO_2D_TRACE) 
libToolboxBlockstructured_debug_a_CXXFLAGS = $(CXXFLAGS_PEANO_2D_DEBUG) 

libToolboxBlockstructured_a_SOURCES       = $(ToolboxBlockstructured_SOURCES)
libToolboxBlockstructured_trace_a_SOURCES = $(ToolboxBlockstructured_SOURCES)
libToolboxBlockstructured_debug_a_SOURCES = $(ToolboxBlockstructured_SOURCES)

