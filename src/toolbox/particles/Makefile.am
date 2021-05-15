AUTOMAKE_OPTIONS = subdir-objects


lib_LIBRARIES = libToolboxParticles.a libToolboxParticles_debug.a libToolboxParticles_trace.a libToolboxParticles_stats.a libToolboxParticles_asserts.a


ToolboxParticles_SOURCES = \
  Tracer.cpp \
  particles.cpp
  

libToolboxParticles_a_CXXFLAGS         = -DDimensions=2 -DPeanoDebug=0 
libToolboxParticles_trace_a_CXXFLAGS   = -DDimensions=2 -DPeanoDebug=1 -g3 
libToolboxParticles_asserts_a_CXXFLAGS = -DDimensions=2 -DPeanoDebug=2 -g3 
libToolboxParticles_debug_a_CXXFLAGS   = -DDimensions=2 -DPeanoDebug=4 -g3 -O0 
libToolboxParticles_stats_a_CXXFLAGS   = -DDimensions=2 -DPeanoDebug=0 -g3 -DTrackStatistics

libToolboxParticles_a_SOURCES         = $(ToolboxParticles_SOURCES)
libToolboxParticles_stats_a_SOURCES   = $(ToolboxParticles_SOURCES)
libToolboxParticles_trace_a_SOURCES   = $(ToolboxParticles_SOURCES)
libToolboxParticles_debug_a_SOURCES   = $(ToolboxParticles_SOURCES)
libToolboxParticles_asserts_a_SOURCES = $(ToolboxParticles_SOURCES)

