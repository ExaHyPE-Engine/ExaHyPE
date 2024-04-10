AUTOMAKE_OPTIONS = subdir-objects


if UseToolboxParticles
lib_LIBRARIES = libToolboxParticles2d.a libToolboxParticles2d_debug.a libToolboxParticles2d_trace.a libToolboxParticles2d_stats.a libToolboxParticles2d_asserts.a \
                libToolboxParticles3d.a libToolboxParticles3d_debug.a libToolboxParticles3d_trace.a libToolboxParticles3d_stats.a libToolboxParticles3d_asserts.a
endif


ToolboxParticles_SOURCES = \
  FileReader.cpp \
  FileReaderHDF5.cpp \
  MultiscaleTransitions.cpp \
  ParticleSet.cpp \
  TrajectoryDatabase.cpp \
  UnitTests.cpp \
  particles.cpp \
  assignmentchecks/TracingAPI.cpp \
  potentials/Springs.cpp \
  tests/MultiscaleTransitionsTest.cpp \
  tests/TestHelpers.cpp \
  tests/TestParticle.cpp
  

libToolboxParticles2d_a_CXXFLAGS         = $(CXXFLAGS_PEANO_2D_RELEASE) 
libToolboxParticles2d_trace_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_TRACE) 
libToolboxParticles2d_asserts_a_CXXFLAGS = $(CXXFLAGS_PEANO_2D_ASSERTS) 
libToolboxParticles2d_debug_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_DEBUG) 
libToolboxParticles2d_stats_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_STATS)

libToolboxParticles3d_a_CXXFLAGS         = $(CXXFLAGS_PEANO_3D_RELEASE) 
libToolboxParticles3d_trace_a_CXXFLAGS   = $(CXXFLAGS_PEANO_3D_TRACE) 
libToolboxParticles3d_asserts_a_CXXFLAGS = $(CXXFLAGS_PEANO_3D_ASSERTS) 
libToolboxParticles3d_debug_a_CXXFLAGS   = $(CXXFLAGS_PEANO_3D_DEBUG) 
libToolboxParticles3d_stats_a_CXXFLAGS   = $(CXXFLAGS_PEANO_3D_STATS)

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

