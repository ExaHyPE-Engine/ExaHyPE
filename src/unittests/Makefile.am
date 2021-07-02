bin_PROGRAMS = UnitTests2d UnitTests3d

UnitTestsSources = main.cpp

UnitTests2d_SOURCES   = $(UnitTestsSources)
UnitTests2d_CXXFLAGS  = $(CXXFLAGS_PEANO_2D_DEBUG)
UnitTests2d_LDFLAGS   = $(LDFLAGS_PEANO_DEBUG) $(LDFLAGS)
UnitTests2d_LDADD     = $(LDADD_PEANO_2D_DEBUG)

if OpenMPGPUOffloading
UnitTests2d_LDADD     += $(top_builddir)/src/tarch/multicore/omp/libTarch_a-multicore.o
UnitTests2d_LDADD     += $(top_builddir)/src/exahype2/fv/libExaHyPE2Core2d_a-Generic.o
UnitTests2d_LDADD     += $(top_builddir)/src/exahype2/fv/libExaHyPE2Core2d_a-Rusanov.o
UnitTests2d_LDADD     += $(top_builddir)/src/exahype2/libExaHyPE2Core2d_a-PatchUtils.o
endif


UnitTests3d_SOURCES   = $(UnitTestsSources)
UnitTests3d_CXXFLAGS  = $(CXXFLAGS_PEANO_3D_DEBUG)
UnitTests3d_LDFLAGS   = $(LDFLAGS_PEANO_DEBUG) $(LDFLAGS)
UnitTests3d_LDADD     = $(LDADD_PEANO_3D_DEBUG)

if OpenMPGPUOffloading
UnitTests3d_LDADD     += $(top_builddir)/src/tarch/multicore/omp/libTarch_a-multicore.o
UnitTests3d_LDADD     += $(top_builddir)/src/exahype2/fv/libExaHyPE2Core3d_a-Generic.o
UnitTests3d_LDADD     += $(top_builddir)/src/exahype2/fv/libExaHyPE2Core3d_a-Rusanov.o
UnitTests3d_LDADD     += $(top_builddir)/src/exahype2/libExaHyPE2Core3d_a-PatchUtils.o
endif

