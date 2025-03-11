# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

AUTOMAKE_OPTIONS = subdir-objects

#
# I want Peano to be as fast as possible and thus build it as static library
# such that a linker/compiler can ipo. One of the other reasons for this is
# that I have a couple of singletons and I did struggle with singletons and
# shared libraries.
#
lib_LIBRARIES = libPeano4Core2d.a libPeano4Core2d_debug.a libPeano4Core2d_asserts.a libPeano4Core2d_trace.a libPeano4Core2d_stats.a \
								libPeano4Core3d.a libPeano4Core3d_debug.a libPeano4Core3d_asserts.a libPeano4Core3d_trace.a libPeano4Core3d_stats.a

Peano4Core_SOURCES = \
	peano4.cpp \
	UnitTests.cpp \
	datamanagement/CellMarker.cpp \
	datamanagement/FaceMarker.cpp \
	datamanagement/VertexMarker.cpp \
	grid/AutomatonState.cpp \
	grid/EmptyTraversalObserver.cpp \
	grid/grid.cpp \
	grid/GridControlEvent.cpp \
	grid/GridVertex.cpp \
	grid/GridStatistics.cpp \
	grid/GridTraversalEvent.cpp \
	grid/GridTraversalEventGenerator.cpp \
	grid/LoadStoreComputeFlag.cpp \
	grid/PeanoCurve.cpp \
	grid/Spacetree.cpp \
	grid/TraversalObserver.cpp \
	grid/TraversalVTKPlotter.cpp \
	grid/tests/GridControlEventTest.cpp \
	grid/tests/SpacetreeTest.cpp \
	grid/tests/GridTraversalEventGeneratorTest.cpp \
	parallel/Node.cpp \
	parallel/parallel.cpp \
	parallel/SpacetreeSet.cpp \
	parallel/StartTraversalMessage.cpp \
	parallel/TreeEntry.cpp \
	parallel/TreeManagementMessage.cpp \
	parallel/tests/PingPongTest.cpp \
	parallel/tests/NodeTest.cpp \
	stacks/stacks.cpp \
	stacks/STDVectorStack.cpp \
	utils/Loop.cpp

libPeano4Core2d_a_SOURCES         = $(Peano4Core_SOURCES)
libPeano4Core2d_trace_a_SOURCES   = $(Peano4Core_SOURCES)
libPeano4Core2d_asserts_a_SOURCES = $(Peano4Core_SOURCES)
libPeano4Core2d_debug_a_SOURCES   = $(Peano4Core_SOURCES)
libPeano4Core2d_stats_a_SOURCES   = $(Peano4Core_SOURCES)

libPeano4Core3d_a_SOURCES         = $(Peano4Core_SOURCES)
libPeano4Core3d_trace_a_SOURCES   = $(Peano4Core_SOURCES)
libPeano4Core3d_asserts_a_SOURCES = $(Peano4Core_SOURCES)
libPeano4Core3d_debug_a_SOURCES   = $(Peano4Core_SOURCES)
libPeano4Core3d_stats_a_SOURCES   = $(Peano4Core_SOURCES)

libPeano4Core2d_a_CXXFLAGS          = $(CXXFLAGS_PEANO_2D_RELEASE)
libPeano4Core2d_stats_a_CXXFLAGS    = $(CXXFLAGS_PEANO_2D_STATS)
libPeano4Core2d_trace_a_CXXFLAGS    = $(CXXFLAGS_PEANO_2D_TRACE)
libPeano4Core2d_asserts_a_CXXFLAGS  = $(CXXFLAGS_PEANO_2D_ASSERTS)
libPeano4Core2d_debug_a_CXXFLAGS    = $(CXXFLAGS_PEANO_2D_DEBUG)

libPeano4Core3d_a_CXXFLAGS          = $(CXXFLAGS_PEANO_3D_RELEASE)
libPeano4Core3d_stats_a_CXXFLAGS    = $(CXXFLAGS_PEANO_3D_STATS)
libPeano4Core3d_trace_a_CXXFLAGS    = $(CXXFLAGS_PEANO_3D_TRACE)
libPeano4Core3d_asserts_a_CXXFLAGS  = $(CXXFLAGS_PEANO_3D_ASSERTS)
libPeano4Core3d_debug_a_CXXFLAGS    = $(CXXFLAGS_PEANO_3D_DEBUG)
