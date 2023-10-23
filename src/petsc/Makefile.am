AUTOMAKE_OPTIONS = subdir-objects

if UsePETSc
  lib_LIBRARIES = libPETSc.a  libPETSc_debug.a  libPETSc_stats.a  libPETSc_asserts.a  libPETSc_trace.a
endif

PETScSources = \
  petsc.cpp \
  LinearEquationSystem.cpp \
  LocalToGlobalMap.cpp \
  UnitTests.cpp

libPETSc_a_CXXFLAGS          = $(CXXFLAGS_PEANO_2D_RELEASE)
libPETSc_trace_a_CXXFLAGS    = $(CXXFLAGS_PEANO_2D_TRACE)
libPETSc_asserts_a_CXXFLAGS  = $(CXXFLAGS_PEANO_2D_ASSERTS)
libPETSc_debug_a_CXXFLAGS    = $(CXXFLAGS_PEANO_2D_DEBUG)
libPETSc_stats_a_CXXFLAGS    = $(CXXFLAGS_PEANO_2D_STATS)

libPETSc_a_SOURCES         = $(PETScSources)
libPETSc_trace_a_SOURCES   = $(PETScSources)
libPETSc_asserts_a_SOURCES = $(PETScSources)
libPETSc_debug_a_SOURCES   = $(PETScSources)
libPETSc_stats_a_SOURCES   = $(PETScSources)
