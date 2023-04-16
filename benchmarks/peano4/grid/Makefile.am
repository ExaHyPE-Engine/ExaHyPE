bin_PROGRAMS = PeanoGrid2d_debug PeanoGrid2d PeanoGrid3d_debug PeanoGrid3d

PeanoGridSources = \
  main.cpp \
  MyObserver.cpp

PeanoGrid2d_debug_CXXFLAGS  = -I$(top_builddir)/src  -DPeanoDebug=4 -DDimensions=2
PeanoGrid2d_debug_LDADD     = -lPeano4Core2d_debug -lTarch_debug
PeanoGrid2d_debug_LDFLAGS   = -L$(top_builddir)/src 
PeanoGrid2d_debug_SOURCES   = $(PeanoGridSources)

PeanoGrid2d_CXXFLAGS  = -I$(top_builddir)/src   -DPeanoDebug=1 -DDimensions=2
PeanoGrid2d_LDADD     = -lPeano4Core2d -lTarch
PeanoGrid2d_LDFLAGS   = -L$(top_builddir)/src 
PeanoGrid2d_SOURCES   = $(PeanoGridSources)

PeanoGrid3d_debug_CXXFLAGS  = -I$(top_builddir)/src  -DPeanoDebug=4 -DDimensions=3
PeanoGrid3d_debug_LDADD     = -lPeano4Core3d_debug -lTarch_debug
PeanoGrid3d_debug_LDFLAGS   = -L$(top_builddir)/src 
PeanoGrid3d_debug_SOURCES   = $(PeanoGridSources)

PeanoGrid3d_CXXFLAGS  = -I$(top_builddir)/src   -DPeanoDebug=1 -DDimensions=3
PeanoGrid3d_LDADD     = -lPeano4Core3d -lTarch
PeanoGrid3d_LDFLAGS   = -L$(top_builddir)/src 
PeanoGrid3d_SOURCES   = $(PeanoGridSources)
