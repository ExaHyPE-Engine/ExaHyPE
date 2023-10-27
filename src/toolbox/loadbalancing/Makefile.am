AUTOMAKE_OPTIONS = subdir-objects


if UseToolboxLoadBalancing
lib_LIBRARIES = libToolboxLoadBalancing2d.a libToolboxLoadBalancing2d_stats.a  libToolboxLoadBalancing2d_debug.a libToolboxLoadBalancing2d_trace.a  libToolboxLoadBalancing2d_asserts.a \
                libToolboxLoadBalancing3d.a libToolboxLoadBalancing3d_stats.a  libToolboxLoadBalancing3d_debug.a libToolboxLoadBalancing3d_trace.a  libToolboxLoadBalancing3d_asserts.a
endif


ToolboxLoadBalancing_SOURCES = \
  AbstractLoadBalancing.cpp \
  Blacklist.cpp \
  Configuration.cpp \
  CostMetrics.cpp \
  loadbalancing.cpp \
  State.cpp \
  Statistics.cpp \
  metrics/CellCount.cpp \
  metrics/CustomCellWeight.cpp \
  strategies/Hardcoded.cpp \
  strategies/NoLoadBalancing.cpp \
  strategies/RecursiveBipartition.cpp \
  strategies/SplitOversizedTree.cpp \
  strategies/SpreadOut.cpp \
  strategies/SpreadOutHierarchically.cpp \
  strategies/SpreadOutOnceGridStagnates.cpp
  

libToolboxLoadBalancing2d_a_CXXFLAGS         = $(CXXFLAGS_PEANO_2D_RELEASE) 
libToolboxLoadBalancing2d_trace_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_TRACE)  
libToolboxLoadBalancing2d_asserts_a_CXXFLAGS = $(CXXFLAGS_PEANO_2D_ASSERTS) 
libToolboxLoadBalancing2d_debug_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_DEBUG) 
libToolboxLoadBalancing2d_stats_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_STATS)

libToolboxLoadBalancing2d_a_SOURCES          = $(ToolboxLoadBalancing_SOURCES)
libToolboxLoadBalancing2d_trace_a_SOURCES    = $(ToolboxLoadBalancing_SOURCES)
libToolboxLoadBalancing2d_asserts_a_SOURCES  = $(ToolboxLoadBalancing_SOURCES)
libToolboxLoadBalancing2d_debug_a_SOURCES    = $(ToolboxLoadBalancing_SOURCES)
libToolboxLoadBalancing2d_stats_a_SOURCES    = $(ToolboxLoadBalancing_SOURCES)


libToolboxLoadBalancing3d_a_CXXFLAGS         = $(CXXFLAGS_PEANO_3D_RELEASE)
libToolboxLoadBalancing3d_trace_a_CXXFLAGS   = $(CXXFLAGS_PEANO_3D_TRACE) 
libToolboxLoadBalancing3d_asserts_a_CXXFLAGS = $(CXXFLAGS_PEANO_3D_ASSERTS) 
libToolboxLoadBalancing3d_debug_a_CXXFLAGS   = $(CXXFLAGS_PEANO_3D_DEBUG) 
libToolboxLoadBalancing3d_stats_a_CXXFLAGS   = $(CXXFLAGS_PEANO_3D_STATS)

libToolboxLoadBalancing3d_a_SOURCES          = $(ToolboxLoadBalancing_SOURCES)
libToolboxLoadBalancing3d_trace_a_SOURCES    = $(ToolboxLoadBalancing_SOURCES)
libToolboxLoadBalancing3d_asserts_a_SOURCES  = $(ToolboxLoadBalancing_SOURCES)
libToolboxLoadBalancing3d_debug_a_SOURCES    = $(ToolboxLoadBalancing_SOURCES)
libToolboxLoadBalancing3d_stats_a_SOURCES    = $(ToolboxLoadBalancing_SOURCES)

