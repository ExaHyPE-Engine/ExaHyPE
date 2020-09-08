AUTOMAKE_OPTIONS = subdir-objects


lib_LIBRARIES = libToolboxLoadBalancing2d.a libToolboxLoadBalancing2d_debug.a libToolboxLoadBalancing2d_trace.a  libToolboxLoadBalancing2d_asserts.a \
                libToolboxLoadBalancing3d.a libToolboxLoadBalancing3d_debug.a libToolboxLoadBalancing3d_trace.a  libToolboxLoadBalancing3d_asserts.a


ToolboxLoadBalancing_SOURCES = \
  Hardcoded.cpp \
  loadbalancing.cpp \
  NoLoadBalancing.cpp \
  RecursiveSubdivision.cpp
  

libToolboxLoadBalancing2d_a_CXXFLAGS         = -DDimensions=2 -DPeanoDebug=0 
libToolboxLoadBalancing2d_trace_a_CXXFLAGS   = -DDimensions=2 -DPeanoDebug=1 -g3 -O0 
libToolboxLoadBalancing2d_asserts_a_CXXFLAGS = -DDimensions=2 -DPeanoDebug=2 -g3 -O0 
libToolboxLoadBalancing2d_debug_a_CXXFLAGS   = -DDimensions=2 -DPeanoDebug=4 -g3 -O0 

libToolboxLoadBalancing2d_a_SOURCES          = $(ToolboxLoadBalancing_SOURCES)
libToolboxLoadBalancing2d_trace_a_SOURCES    = $(ToolboxLoadBalancing_SOURCES)
libToolboxLoadBalancing2d_asserts_a_SOURCES  = $(ToolboxLoadBalancing_SOURCES)
libToolboxLoadBalancing2d_debug_a_SOURCES    = $(ToolboxLoadBalancing_SOURCES)



libToolboxLoadBalancing3d_a_CXXFLAGS         = -DDimensions=3 -DPeanoDebug=0 
libToolboxLoadBalancing3d_trace_a_CXXFLAGS   = -DDimensions=3 -DPeanoDebug=1 -g3 -O0 
libToolboxLoadBalancing3d_asserts_a_CXXFLAGS = -DDimensions=3 -DPeanoDebug=2 -g3 -O0 
libToolboxLoadBalancing3d_debug_a_CXXFLAGS   = -DDimensions=3 -DPeanoDebug=4 -g3 -O0 

libToolboxLoadBalancing3d_a_SOURCES          = $(ToolboxLoadBalancing_SOURCES)
libToolboxLoadBalancing3d_trace_a_SOURCES    = $(ToolboxLoadBalancing_SOURCES)
libToolboxLoadBalancing3d_asserts_a_SOURCES  = $(ToolboxLoadBalancing_SOURCES)
libToolboxLoadBalancing3d_debug_a_SOURCES    = $(ToolboxLoadBalancing_SOURCES)

