AUTOMAKE_OPTIONS = subdir-objects


if UseToolboxBlockstructured
lib_LIBRARIES = libToolboxBlockstructured2d.a libToolboxBlockstructured2d_debug.a libToolboxBlockstructured2d_trace.a libToolboxBlockstructured2d_stats.a libToolboxBlockstructured2d_asserts.a \
                libToolboxBlockstructured3d.a libToolboxBlockstructured3d_debug.a libToolboxBlockstructured3d_trace.a libToolboxBlockstructured3d_stats.a libToolboxBlockstructured3d_asserts.a
endif


ToolboxBlockstructured_SOURCES = \
  Enumeration.cpp \
  Interpolation.cpp
  

libToolboxBlockstructured2d_a_CXXFLAGS         = $(CXXFLAGS_PEANO_2D_RELEASE) 
libToolboxBlockstructured2d_trace_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_TRACE) 
libToolboxBlockstructured2d_debug_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_DEBUG) 
libToolboxBlockstructured2d_stats_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_STATS) 
libToolboxBlockstructured2d_asserts_a_CXXFLAGS = $(CXXFLAGS_PEANO_2D_ASSERTS) 

libToolboxBlockstructured3d_a_CXXFLAGS         = $(CXXFLAGS_PEANO_3D_RELEASE) 
libToolboxBlockstructured3d_trace_a_CXXFLAGS   = $(CXXFLAGS_PEANO_3D_TRACE) 
libToolboxBlockstructured3d_debug_a_CXXFLAGS   = $(CXXFLAGS_PEANO_3D_DEBUG) 
libToolboxBlockstructured3d_stats_a_CXXFLAGS   = $(CXXFLAGS_PEANO_3D_STATS) 
libToolboxBlockstructured3d_asserts_a_CXXFLAGS = $(CXXFLAGS_PEANO_3D_ASSERTS) 

libToolboxBlockstructured2d_a_SOURCES         = $(ToolboxBlockstructured_SOURCES)
libToolboxBlockstructured2d_trace_a_SOURCES   = $(ToolboxBlockstructured_SOURCES)
libToolboxBlockstructured2d_debug_a_SOURCES   = $(ToolboxBlockstructured_SOURCES)
libToolboxBlockstructured2d_stats_a_SOURCES   = $(ToolboxBlockstructured_SOURCES)
libToolboxBlockstructured2d_asserts_a_SOURCES = $(ToolboxBlockstructured_SOURCES)

libToolboxBlockstructured3d_a_SOURCES         = $(ToolboxBlockstructured_SOURCES)
libToolboxBlockstructured3d_trace_a_SOURCES   = $(ToolboxBlockstructured_SOURCES)
libToolboxBlockstructured3d_debug_a_SOURCES   = $(ToolboxBlockstructured_SOURCES)
libToolboxBlockstructured3d_stats_a_SOURCES   = $(ToolboxBlockstructured_SOURCES)
libToolboxBlockstructured3d_asserts_a_SOURCES = $(ToolboxBlockstructured_SOURCES)


