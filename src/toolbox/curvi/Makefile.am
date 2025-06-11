# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

AUTOMAKE_OPTIONS = subdir-objects

if UseToolboxCurvi
lib_LIBRARIES = libToolboxCurvi2d.a libToolboxCurvi2d_stats.a  libToolboxCurvi2d_debug.a libToolboxCurvi2d_trace.a  libToolboxCurvi2d_asserts.a \
                libToolboxCurvi3d.a libToolboxCurvi3d_stats.a  libToolboxCurvi3d_debug.a libToolboxCurvi3d_trace.a  libToolboxCurvi3d_asserts.a
endif

ToolboxCurvi_SOURCES = \
  geometry/face.cpp \
  geometry/faceBoundary.cpp \
  geometry/edge.cpp \
  geometry/block.cpp \
  input/yaml_input.cpp \
  coordinate.cpp

libToolboxCurvi2d_a_CXXFLAGS         = $(CXXFLAGS_PEANO_2D_RELEASE)
libToolboxCurvi2d_trace_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_TRACE)
libToolboxCurvi2d_asserts_a_CXXFLAGS = $(CXXFLAGS_PEANO_2D_ASSERTS)
libToolboxCurvi2d_debug_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_DEBUG)
libToolboxCurvi2d_stats_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_STATS)

libToolboxCurvi2d_a_SOURCES          = $(ToolboxCurvi_SOURCES)
libToolboxCurvi2d_trace_a_SOURCES    = $(ToolboxCurvi_SOURCES)
libToolboxCurvi2d_asserts_a_SOURCES  = $(ToolboxCurvi_SOURCES)
libToolboxCurvi2d_debug_a_SOURCES    = $(ToolboxCurvi_SOURCES)
libToolboxCurvi2d_stats_a_SOURCES    = $(ToolboxCurvi_SOURCES)

libToolboxCurvi3d_a_CXXFLAGS         = $(CXXFLAGS_PEANO_3D_RELEASE)
libToolboxCurvi3d_trace_a_CXXFLAGS   = $(CXXFLAGS_PEANO_3D_TRACE)
libToolboxCurvi3d_asserts_a_CXXFLAGS = $(CXXFLAGS_PEANO_3D_ASSERTS)
libToolboxCurvi3d_debug_a_CXXFLAGS   = $(CXXFLAGS_PEANO_3D_DEBUG)
libToolboxCurvi3d_stats_a_CXXFLAGS   = $(CXXFLAGS_PEANO_3D_STATS)

libToolboxCurvi3d_a_SOURCES          = $(ToolboxCurvi_SOURCES)
libToolboxCurvi3d_trace_a_SOURCES    = $(ToolboxCurvi_SOURCES)
libToolboxCurvi3d_asserts_a_SOURCES  = $(ToolboxCurvi_SOURCES)
libToolboxCurvi3d_debug_a_SOURCES    = $(ToolboxCurvi_SOURCES)
libToolboxCurvi3d_stats_a_SOURCES    = $(ToolboxCurvi_SOURCES)
