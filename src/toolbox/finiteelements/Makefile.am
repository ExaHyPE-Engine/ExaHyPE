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
  

libToolboxFiniteElements2d_a_CXXFLAGS         = $(CXXFLAGS_PEANO_2D_RELEASE) 
libToolboxFiniteElements2d_trace_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_TRACE) 
libToolboxFiniteElements2d_debug_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_DEBUG) 
libToolboxFiniteElements2d_stats_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_STATS)
libToolboxFiniteElements2d_asserts_a_CXXFLAGS = $(CXXFLAGS_PEANO_2D_ASSERTS) 

libToolboxFiniteElements3d_a_CXXFLAGS         = $(CXXFLAGS_PEANO_3D_RELEASE) 
libToolboxFiniteElements3d_trace_a_CXXFLAGS   = $(CXXFLAGS_PEANO_3D_TRACE) 
libToolboxFiniteElements3d_debug_a_CXXFLAGS   = $(CXXFLAGS_PEANO_3D_DEBUG) 
libToolboxFiniteElements3d_asserts_a_CXXFLAGS = $(CXXFLAGS_PEANO_3D_ASSERTS)
libToolboxFiniteElements3d_stats_a_CXXFLAGS   = $(CXXFLAGS_PEANO_3D_STATS)

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

