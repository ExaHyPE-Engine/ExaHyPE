AUTOMAKE_OPTIONS = subdir-objects

if UseSWIFT
  lib_LIBRARIES = libSWIFT2Core2d.a  libSWIFT2Core2d_debug.a  libSWIFT2Core2d_stats.a  libSWIFT2Core2d_asserts.a  libSWIFT2Core2d_trace.a \
                  libSWIFT2Core3d.a  libSWIFT2Core3d_debug.a  libSWIFT2Core3d_stats.a  libSWIFT2Core3d_asserts.a  libSWIFT2Core3d_trace.a
endif



SWIFT2Sources = \
  GridControlEvents.cpp \
  ParticleSpecies.cpp \
  swift2.cpp \
  TaskEnumerator.cpp \
  TaskNumber.cpp \
  UnitTests.cpp \
  UserInterface.cpp \
  boundaryconditions/FixedBoundary.cpp \
  boundaryconditions/Utils.cpp \
  kernels/ParticleSetIterators.cpp \
  kernels/legacy/Density.cpp \
  kernels/legacy/HydroForce.cpp \
  kernels/legacy/SmoothingLength.cpp \
  kernels/legacy/Swift.cpp \
  timestepping/TimeStepping.cpp \
  dependencychecks/DependencyChecks.cpp

libSWIFT2Core2d_a_CXXFLAGS          = $(CXXFLAGS_PEANO_2D_RELEASE)
libSWIFT2Core2d_trace_a_CXXFLAGS    = $(CXXFLAGS_PEANO_2D_TRACE)  
libSWIFT2Core2d_asserts_a_CXXFLAGS  = $(CXXFLAGS_PEANO_2D_ASSERTS)  
libSWIFT2Core2d_debug_a_CXXFLAGS    = $(CXXFLAGS_PEANO_2D_DEBUG)  
libSWIFT2Core2d_stats_a_CXXFLAGS    = $(CXXFLAGS_PEANO_2D_STATS)

libSWIFT2Core3d_a_CXXFLAGS          = $(CXXFLAGS_PEANO_3D_RELEASE)
libSWIFT2Core3d_trace_a_CXXFLAGS    = $(CXXFLAGS_PEANO_3D_TRACE)  
libSWIFT2Core3d_asserts_a_CXXFLAGS  = $(CXXFLAGS_PEANO_3D_ASSERTS)  
libSWIFT2Core3d_debug_a_CXXFLAGS    = $(CXXFLAGS_PEANO_3D_DEBUG)  
libSWIFT2Core3d_stats_a_CXXFLAGS    = $(CXXFLAGS_PEANO_3D_STATS)


libSWIFT2Core2d_a_SOURCES         = $(SWIFT2Sources)
libSWIFT2Core2d_trace_a_SOURCES   = $(SWIFT2Sources)
libSWIFT2Core2d_asserts_a_SOURCES = $(SWIFT2Sources)
libSWIFT2Core2d_debug_a_SOURCES   = $(SWIFT2Sources)
libSWIFT2Core2d_stats_a_SOURCES   = $(SWIFT2Sources)

libSWIFT2Core3d_a_SOURCES         = $(SWIFT2Sources)
libSWIFT2Core3d_trace_a_SOURCES   = $(SWIFT2Sources)
libSWIFT2Core3d_asserts_a_SOURCES = $(SWIFT2Sources)
libSWIFT2Core3d_debug_a_SOURCES   = $(SWIFT2Sources)
libSWIFT2Core3d_stats_a_SOURCES   = $(SWIFT2Sources)


