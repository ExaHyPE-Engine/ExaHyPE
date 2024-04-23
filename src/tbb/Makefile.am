AUTOMAKE_OPTIONS = subdir-objects


if UseTBB
lib_LIBRARIES = libTBB.a libTBB_debug.a libTBB_trace.a
endif


TBB_SOURCES = \
	dynamic_task_graph.cpp \
	dynamic_task_graph_node.cpp \
	dynamic_task_graph_spawned_node.cpp \
	dynamic_task_graph_node_repository.cpp \
	dynamic_task_graph_utils.cpp 
	  

libTBB_a_CXXFLAGS         = $(CXXFLAGS_PEANO_2D_RELEASE) 
libTBB_trace_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_TRACE) -DTBB_USE_DEBUG
libTBB_debug_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_DEBUG) 

libTBB_a_SOURCES          = $(TBB_SOURCES)
libTBB_trace_a_SOURCES    = $(TBB_SOURCES)
libTBB_debug_a_SOURCES    = $(TBB_SOURCES)
