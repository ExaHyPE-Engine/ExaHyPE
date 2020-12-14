bin_PROGRAMS = UnitTests2d UnitTests3d

UnitTestsSources = main.cpp

UnitTests2d_CXXFLAGS  = -I$(top_builddir)/src  -DPeanoDebug=4 -DDimensions=2 -g3 -O0
UnitTests2d_LDADD     = -lPeano4Core2d_debug -lTarch_debug
UnitTests2d_LDFLAGS   = -L$(top_builddir)/src -g3
if UseExaHyPE
UnitTests2d_LDADD     += -lExaHyPE2Core2d_debug
UnitTests2d_LDFLAGS   += -L$(top_builddir)/src/exahype2
endif
UnitTests2d_SOURCES   = $(UnitTestsSources)


UnitTests3d_CXXFLAGS  = -I$(top_builddir)/src  -DPeanoDebug=4 -DDimensions=3 -g3 -O0
UnitTests3d_LDADD     = -lPeano4Core3d_debug -lTarch_debug
UnitTests3d_LDFLAGS   = -L$(top_builddir)/src -g3
if UseExaHyPE
UnitTests3d_LDADD     += -lExaHyPE2Core3d_debug
UnitTests3d_LDFLAGS   += -L$(top_builddir)/src/exahype2
endif
UnitTests3d_SOURCES   = $(UnitTestsSources)

