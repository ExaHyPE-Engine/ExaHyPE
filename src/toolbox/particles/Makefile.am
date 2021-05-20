AUTOMAKE_OPTIONS = subdir-objects


if UseToolboxParticles
lib_LIBRARIES = libToolboxParticles2d.a libToolboxParticles2d_debug.a libToolboxParticles2d_trace.a libToolboxParticles2d_stats.a libToolboxParticles2d_asserts.a \
                libToolboxParticles3d.a libToolboxParticles3d_debug.a libToolboxParticles3d_trace.a libToolboxParticles3d_stats.a libToolboxParticles3d_asserts.a
endif


ToolboxParticles_SOURCES = \
  Tracer.cpp \
  TrajectoryDatabase.cpp \
  particles.cpp \
  potentials/Springs.cpp
  

libToolboxParticles2d_a_CXXFLAGS         = -DDimensions=2 -DPeanoDebug=0 
libToolboxParticles2d_trace_a_CXXFLAGS   = -DDimensions=2 -DPeanoDebug=1 -g3 
libToolboxParticles2d_asserts_a_CXXFLAGS = -DDimensions=2 -DPeanoDebug=2 -g3 
libToolboxParticles2d_debug_a_CXXFLAGS   = -DDimensions=2 -DPeanoDebug=4 -g3 -O0 
libToolboxParticles2d_stats_a_CXXFLAGS   = -DDimensions=2 -DPeanoDebug=0 -g3 -DTrackStatistics

libToolboxParticles3d_a_CXXFLAGS         = -DDimensions=3 -DPeanoDebug=0 
libToolboxParticles3d_trace_a_CXXFLAGS   = -DDimensions=3 -DPeanoDebug=1 -g3 
libToolboxParticles3d_asserts_a_CXXFLAGS = -DDimensions=3 -DPeanoDebug=2 -g3 
libToolboxParticles3d_debug_a_CXXFLAGS   = -DDimensions=3 -DPeanoDebug=4 -g3 -O0 
libToolboxParticles3d_stats_a_CXXFLAGS   = -DDimensions=3 -DPeanoDebug=0 -g3 -DTrackStatistics

libToolboxParticles2d_a_SOURCES         = $(ToolboxParticles_SOURCES)
libToolboxParticles2d_stats_a_SOURCES   = $(ToolboxParticles_SOURCES)
libToolboxParticles2d_trace_a_SOURCES   = $(ToolboxParticles_SOURCES)
libToolboxParticles2d_debug_a_SOURCES   = $(ToolboxParticles_SOURCES)
libToolboxParticles2d_asserts_a_SOURCES = $(ToolboxParticles_SOURCES)

libToolboxParticles3d_a_SOURCES         = $(ToolboxParticles_SOURCES)
libToolboxParticles3d_stats_a_SOURCES   = $(ToolboxParticles_SOURCES)
libToolboxParticles3d_trace_a_SOURCES   = $(ToolboxParticles_SOURCES)
libToolboxParticles3d_debug_a_SOURCES   = $(ToolboxParticles_SOURCES)
libToolboxParticles3d_asserts_a_SOURCES = $(ToolboxParticles_SOURCES)

