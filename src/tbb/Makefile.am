AUTOMAKE_OPTIONS = subdir-objects


if UseTBB
lib_LIBRARIES = libtbb_extension.a libtbb_extension_debug.a libtbb_extension_trace.a
endif


TBB_SOURCES = \
	dynamic_task_graph.cpp \
	dynamic_task_graph_node.cpp \
	dynamic_task_graph_spawned_node.cpp \
	dynamic_task_graph_node_repository.cpp \
	dynamic_task_graph_utils.cpp 
	  

libtbb_extension_a_CXXFLAGS         = $(CXXFLAGS_PEANO_2D_RELEASE) 
libtbb_extension_trace_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_TRACE) -DTBB_USE_DEBUG
libtbb_extension_debug_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_DEBUG) 

libtbb_extension_a_SOURCES          = $(TBB_SOURCES)
libtbb_extension_trace_a_SOURCES    = $(TBB_SOURCES)
libtbb_extension_debug_a_SOURCES    = $(TBB_SOURCES)
