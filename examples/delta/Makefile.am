if UseDelta
  bin_PROGRAMS = Delta_debug Delta Delta_demo_2d_debug
else
  bin_PROGRAMS = Delta_demo_2d_debug
endif



#
# I recommend 
#

DeltaSources = \
  main.cpp \
  CellData.cpp \
  InitData.cpp \
  MyObserver.cpp \
  CompositeMapping.cpp \
  PeanoFormatCellDataPlotter.cpp


Delta_CXXFLAGS  = -I$(top_builddir)/src -DPeanoDebug=0 -DDimensions=3
Delta_LDADD     = -lPeano4Core3d -lTarch
Delta_LDFLAGS   = -L$(top_builddir)/src -lDelta
Delta_SOURCES   = $(DeltaSources)


Delta_debug_CXXFLAGS  = -O0 -I$(top_builddir)/src   -g3 -DPeanoDebug=4 -DDimensions=3
Delta_debug_LDADD     = -lPeano4Core3d_debug -lTarch_debug
Delta_debug_LDFLAGS   = -L$(top_builddir)/src -lDelta_debug
Delta_debug_SOURCES   = $(DeltaSources)


Delta_demo_2d_debug_CXXFLAGS  = -I$(top_builddir)/src   -g3 -O0 -DPeanoDebug=4 -DDimensions=2
Delta_demo_2d_debug_LDADD     = -lPeano4Core2d_debug -lTarch_debug 
Delta_demo_2d_debug_LDFLAGS   = -L$(top_builddir)/src 
Delta_demo_2d_debug_SOURCES   = $(DeltaSources)

