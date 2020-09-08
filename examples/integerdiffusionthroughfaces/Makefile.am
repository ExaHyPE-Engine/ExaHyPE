bin_PROGRAMS = PeanoIntegerDiffusionThroughFaces2d_debug PeanoIntegerDiffusionThroughFaces2d PeanoIntegerDiffusionThroughFaces3d_debug PeanoIntegerDiffusionThroughFaces3d

PeanoIntegerDiffusionThroughFacesSources = \
  main.cpp \
  VTUCellDataPlotter.cpp \
  PeanoFormatCellDataPlotter.cpp \
  CompositeMapping.cpp \
  MyMapping.cpp \
  MyObserver.cpp

PeanoIntegerDiffusionThroughFaces2d_debug_CXXFLAGS  =  -I$(top_builddir)/src -DPeanoDebug=4 -DDimensions=2
PeanoIntegerDiffusionThroughFaces2d_debug_LDADD     = -lPeano4Core2d_debug -lTarch_debug
PeanoIntegerDiffusionThroughFaces2d_debug_LDFLAGS   = -L$(top_builddir)/src
PeanoIntegerDiffusionThroughFaces2d_debug_SOURCES   = $(PeanoIntegerDiffusionThroughFacesSources)

PeanoIntegerDiffusionThroughFaces2d_CXXFLAGS  =  -I$(top_builddir)/src  -DPeanoDebug=1 -DDimensions=2
PeanoIntegerDiffusionThroughFaces2d_LDADD     = -lPeano4Core2d -lTarch
PeanoIntegerDiffusionThroughFaces2d_LDFLAGS   = -L$(top_builddir)/src
PeanoIntegerDiffusionThroughFaces2d_SOURCES   = $(PeanoIntegerDiffusionThroughFacesSources)

PeanoIntegerDiffusionThroughFaces3d_debug_CXXFLAGS  =  -I$(top_builddir)/src -DPeanoDebug=4 -DDimensions=3
PeanoIntegerDiffusionThroughFaces3d_debug_LDADD     = -lPeano4Core3d_debug -lTarch_debug
PeanoIntegerDiffusionThroughFaces3d_debug_LDFLAGS   = -L$(top_builddir)/src
PeanoIntegerDiffusionThroughFaces3d_debug_SOURCES   = $(PeanoIntegerDiffusionThroughFacesSources)

PeanoIntegerDiffusionThroughFaces3d_CXXFLAGS  =  -I$(top_builddir)/src  -DPeanoDebug=1 -DDimensions=3
PeanoIntegerDiffusionThroughFaces3d_LDADD     = -lPeano4Core3d -lTarch
PeanoIntegerDiffusionThroughFaces3d_LDFLAGS   = -L$(top_builddir)/src
PeanoIntegerDiffusionThroughFaces3d_SOURCES   = $(PeanoIntegerDiffusionThroughFacesSources)
