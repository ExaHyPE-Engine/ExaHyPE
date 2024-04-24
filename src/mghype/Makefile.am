AUTOMAKE_OPTIONS = subdir-objects

if UseMGHyPE
  lib_LIBRARIES = libMultigrid2d.a  libMultigrid2d_debug.a  libMultigrid2d_stats.a  libMultigrid2d_asserts.a  libMultigrid2d_trace.a \
                  libMultigrid3d.a  libMultigrid3d_debug.a  libMultigrid3d_stats.a  libMultigrid3d_asserts.a  libMultigrid3d_trace.a
endif



MultigridSources = \
  mghype.cpp \
  matrixfree/solvers/DGCGCoupling.cpp \
  matrixfree/solvers/Solver.cpp
  

libMultigrid2d_a_CXXFLAGS          = $(CXXFLAGS_PEANO_2D_RELEASE)
libMultigrid2d_trace_a_CXXFLAGS    = $(CXXFLAGS_PEANO_2D_TRACE)  
libMultigrid2d_asserts_a_CXXFLAGS  = $(CXXFLAGS_PEANO_2D_ASSERTS)  
libMultigrid2d_debug_a_CXXFLAGS    = $(CXXFLAGS_PEANO_2D_DEBUG)  
libMultigrid2d_stats_a_CXXFLAGS    = $(CXXFLAGS_PEANO_2D_STATS)

libMultigrid3d_a_CXXFLAGS          = $(CXXFLAGS_PEANO_3D_RELEASE)
libMultigrid3d_trace_a_CXXFLAGS    = $(CXXFLAGS_PEANO_3D_TRACE)  
libMultigrid3d_asserts_a_CXXFLAGS  = $(CXXFLAGS_PEANO_3D_ASSERTS)  
libMultigrid3d_debug_a_CXXFLAGS    = $(CXXFLAGS_PEANO_3D_DEBUG)  
libMultigrid3d_stats_a_CXXFLAGS    = $(CXXFLAGS_PEANO_3D_STATS)


libMultigrid2d_a_SOURCES         = $(MultigridSources)
libMultigrid2d_trace_a_SOURCES   = $(MultigridSources)
libMultigrid2d_asserts_a_SOURCES = $(MultigridSources)
libMultigrid2d_debug_a_SOURCES   = $(MultigridSources)
libMultigrid2d_stats_a_SOURCES   = $(MultigridSources)

libMultigrid3d_a_SOURCES         = $(MultigridSources)
libMultigrid3d_trace_a_SOURCES   = $(MultigridSources)
libMultigrid3d_asserts_a_SOURCES = $(MultigridSources)
libMultigrid3d_debug_a_SOURCES   = $(MultigridSources)
libMultigrid3d_stats_a_SOURCES   = $(MultigridSources)


