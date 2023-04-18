bin_PROGRAMS = PeanoRegularGridUpscaling2d_debug PeanoRegularGridUpscaling2d PeanoRegularGridUpscaling3d_debug PeanoRegularGridUpscaling3d

PeanoRegularGridUpscalingSources = \
  main.cpp \
  MyObserver.cpp

PeanoRegularGridUpscaling2d_debug_CXXFLAGS  = -g3 -O0 -I$(top_builddir)/src  -DPeanoDebug=4 -DDimensions=2
PeanoRegularGridUpscaling2d_debug_LDADD     = -lPeano4Core2d_debug -lTarch_debug -lToolboxFiniteElements2d_debug
PeanoRegularGridUpscaling2d_debug_LDFLAGS   = -L$(top_builddir)/src -L$(top_builddir)/src/toolbox/finiteelements
PeanoRegularGridUpscaling2d_debug_SOURCES   = $(PeanoRegularGridUpscalingSources)

PeanoRegularGridUpscaling2d_CXXFLAGS  = -I$(top_builddir)/src  -DPeanoDebug=0 -DDimensions=2
PeanoRegularGridUpscaling2d_LDADD     = -lPeano4Core2d  -lTarch -lToolboxFiniteElements2d
PeanoRegularGridUpscaling2d_LDFLAGS   = -L$(top_builddir)/src -L$(top_builddir)/src/toolbox/finiteelements
PeanoRegularGridUpscaling2d_SOURCES   = $(PeanoRegularGridUpscalingSources)

PeanoRegularGridUpscaling3d_debug_CXXFLAGS  = -g3 -O0 -I$(top_builddir)/src  -DPeanoDebug=4 -DDimensions=3
PeanoRegularGridUpscaling3d_debug_LDADD     = -lPeano4Core3d_debug -lTarch_debug -lToolboxFiniteElements3d_debug
PeanoRegularGridUpscaling3d_debug_LDFLAGS   = -L$(top_builddir)/src -L$(top_builddir)/src/toolbox/finiteelements
PeanoRegularGridUpscaling3d_debug_SOURCES   = $(PeanoRegularGridUpscalingSources)

PeanoRegularGridUpscaling3d_CXXFLAGS  = -I$(top_builddir)/src   -DPeanoDebug=0 -DDimensions=3
PeanoRegularGridUpscaling3d_LDADD     = -lPeano4Core3d -lTarch -lToolboxFiniteElements3d
PeanoRegularGridUpscaling3d_LDFLAGS   = -L$(top_builddir)/src -L$(top_builddir)/src/toolbox/finiteelements
PeanoRegularGridUpscaling3d_SOURCES   = $(PeanoRegularGridUpscalingSources)
