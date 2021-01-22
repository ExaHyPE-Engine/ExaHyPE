bin_PROGRAMS = UnitTests2d UnitTests3d

UnitTestsSources = main.cpp

UnitTests2d_CXXFLAGS  = -I$(top_builddir)/src  -DPeanoDebug=4 -DDimensions=2 -g3 -O0
UnitTests2d_LDFLAGS   = -L$(top_builddir)/src -g3
UnitTests2d_LDADD     =
if UseExaHyPE
UnitTests2d_LDADD     += -lExaHyPE2Core2d_debug
UnitTests2d_LDFLAGS   += -L$(top_builddir)/src/exahype2
if OpenMPGPUOffloading
UnitTests2d_LDADD     += $(top_builddir)/src/tarch/multicore/omp/libTarch_a-multicore.o
UnitTests2d_LDADD     += $(top_builddir)/src/exahype2/fv/libExaHyPE2Core2d_a-Generic.o
UnitTests2d_LDADD     += $(top_builddir)/src/exahype2/fv/libExaHyPE2Core2d_a-Rusanov.o
UnitTests2d_LDADD     += $(top_builddir)/src/exahype2/libExaHyPE2Core2d_a-PatchUtils.o
endif
endif
UnitTests2d_LDADD     += -lPeano4Core2d_debug -lTarch_debug
UnitTests2d_SOURCES   = $(UnitTestsSources)

UnitTests3d_CXXFLAGS  = -I$(top_builddir)/src  -DPeanoDebug=4 -DDimensions=3 -g3 -O0
UnitTests3d_LDFLAGS   = -L$(top_builddir)/src -g3
UnitTests3d_LDADD     =
if UseExaHyPE
UnitTests3d_LDADD     += -lExaHyPE2Core3d_debug
UnitTests3d_LDFLAGS   += -L$(top_builddir)/src/exahype2
if OpenMPGPUOffloading
UnitTests3d_LDADD     += $(top_builddir)/src/tarch/multicore/omp/libTarch_a-multicore.o
UnitTests3d_LDADD     += $(top_builddir)/src/exahype2/fv/libExaHyPE2Core3d_a-Generic.o
UnitTests3d_LDADD     += $(top_builddir)/src/exahype2/fv/libExaHyPE2Core3d_a-Rusanov.o
UnitTests3d_LDADD     += $(top_builddir)/src/exahype2/libExaHyPE2Core3d_a-PatchUtils.o
endif
endif
UnitTests3d_LDADD     += -lPeano4Core3d_debug -lTarch_debug
UnitTests3d_SOURCES   = $(UnitTestsSources)

