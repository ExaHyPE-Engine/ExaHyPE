# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

AUTOMAKE_OPTIONS = subdir-objects

if UseToolboxCurvi
lib_LIBRARIES = libToolboxCurvi.a libToolboxCurvi_stats.a  libToolboxCurvi_debug.a libToolboxCurvi_trace.a  libToolboxCurvi_asserts.a
endif

ToolboxCurvi_SOURCES = \
  geometry/Block.cpp \
  geometry/BlockBoundary.cpp \
  geometry/Edge.cpp \
  geometry/Face.cpp \
  geometry/FaceBoundary.cpp \
  input/YAMLInput.cpp \
  interface/Interface.cpp \
  kdTree/InnerNode.cpp \
  kdTree/Leaf.cpp \
  kdTree/Root.cpp \
  netcdf/NetCDFWriter.cpp \
  transformation/Transformation.cpp \
  Coordinate.cpp

libToolboxCurvi_a_SOURCES          = $(ToolboxCurvi_SOURCES)
libToolboxCurvi_trace_a_SOURCES    = $(ToolboxCurvi_SOURCES)
libToolboxCurvi_asserts_a_SOURCES  = $(ToolboxCurvi_SOURCES)
libToolboxCurvi_debug_a_SOURCES    = $(ToolboxCurvi_SOURCES)
libToolboxCurvi_stats_a_SOURCES    = $(ToolboxCurvi_SOURCES)

libToolboxCurvi_a_CXXFLAGS         = $(CXXFLAGS_PEANO_2D_RELEASE)
libToolboxCurvi_trace_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_TRACE)
libToolboxCurvi_asserts_a_CXXFLAGS = $(CXXFLAGS_PEANO_2D_ASSERTS)
libToolboxCurvi_debug_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_DEBUG)
libToolboxCurvi_stats_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_STATS)
