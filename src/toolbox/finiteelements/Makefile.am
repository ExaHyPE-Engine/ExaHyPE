AUTOMAKE_OPTIONS = subdir-objects


if UseToolboxFiniteElements
lib_LIBRARIES = libToolboxFiniteElements2d.a libToolboxFiniteElements2d_asserts.a libToolboxFiniteElements2d_stats.a libToolboxFiniteElements2d_debug.a libToolboxFiniteElements2d_trace.a \
                libToolboxFiniteElements3d.a libToolboxFiniteElements3d_asserts.a libToolboxFiniteElements3d_stats.a libToolboxFiniteElements3d_debug.a libToolboxFiniteElements3d_trace.a
endif


ToolboxFiniteElements_SOURCES = \
  BSplinesStencilFactory.cpp \
  ElementMatrix.cpp \
  Stencil.cpp \
  StencilFactory.cpp \
  tests/StencilFactoryTest.cpp
  

libToolboxFiniteElements2d_a_CXXFLAGS         = -DDimensions=2 -DPeanoDebug=0 
libToolboxFiniteElements2d_trace_a_CXXFLAGS   = -DDimensions=2 -DPeanoDebug=1 -g3 
libToolboxFiniteElements2d_debug_a_CXXFLAGS   = -DDimensions=2 -DPeanoDebug=4 -g3 -O0 
libToolboxFiniteElements2d_stats_a_CXXFLAGS   = -DDimensions=2 -DPeanoDebug=0 -g3 -DTrackStatistics
libToolboxFiniteElements2d_asserts_a_CXXFLAGS = -DDimensions=2 -DPeanoDebug=2 -g3 

libToolboxFiniteElements3d_a_CXXFLAGS         = -DDimensions=3 -DPeanoDebug=0 
libToolboxFiniteElements3d_trace_a_CXXFLAGS   = -DDimensions=3 -DPeanoDebug=1 -g3 
libToolboxFiniteElements3d_debug_a_CXXFLAGS   = -DDimensions=3 -DPeanoDebug=4 -g3 -O0 
libToolboxFiniteElements3d_asserts_a_CXXFLAGS = -DDimensions=3 -DPeanoDebug=2 -g3
libToolboxFiniteElements3d_stats_a_CXXFLAGS   = -DDimensions=3 -DPeanoDebug=0 -g3 -DTrackStatistics

libToolboxFiniteElements2d_a_SOURCES         = $(ToolboxFiniteElements_SOURCES)
libToolboxFiniteElements2d_trace_a_SOURCES   = $(ToolboxFiniteElements_SOURCES)
libToolboxFiniteElements2d_debug_a_SOURCES   = $(ToolboxFiniteElements_SOURCES)
libToolboxFiniteElements2d_asserts_a_SOURCES = $(ToolboxFiniteElements_SOURCES)
libToolboxFiniteElements2d_stats_a_SOURCES   = $(ToolboxFiniteElements_SOURCES)

libToolboxFiniteElements3d_a_SOURCES         = $(ToolboxFiniteElements_SOURCES)
libToolboxFiniteElements3d_trace_a_SOURCES   = $(ToolboxFiniteElements_SOURCES)
libToolboxFiniteElements3d_debug_a_SOURCES   = $(ToolboxFiniteElements_SOURCES)
libToolboxFiniteElements3d_asserts_a_SOURCES = $(ToolboxFiniteElements_SOURCES)
libToolboxFiniteElements3d_stats_a_SOURCES   = $(ToolboxFiniteElements_SOURCES)

