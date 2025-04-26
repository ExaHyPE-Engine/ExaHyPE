# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

AUTOMAKE_OPTIONS = subdir-objects

if UseExaHyPE
  lib_LIBRARIES = libExaHyPE2Core2d.a  libExaHyPE2Core2d_debug.a  libExaHyPE2Core2d_stats.a  libExaHyPE2Core2d_asserts.a  libExaHyPE2Core2d_trace.a \
                  libExaHyPE2Core3d.a  libExaHyPE2Core3d_debug.a  libExaHyPE2Core3d_stats.a  libExaHyPE2Core3d_asserts.a  libExaHyPE2Core3d_trace.a
endif

ExaHyPE2Sources = \
  CellAccess.cpp \
  CellData.cpp \
  EnclaveTask.cpp \
  LoadBalancingConfiguration.cpp \
  PlotFilter.cpp \
  RefinementControl.cpp \
  PrecisionControl.cpp \
  RefinementControlService.cpp \
  TimeStepping.cpp \
  UserInterface.cpp \
  UnitTests.cpp \
  enumerator/AoSLexicographicEnumerator.cpp \
  enumerator/SoALexicographicEnumerator.cpp \
  enumerator/SingleDoFEnumerator.cpp \
  enumerator/AoSoALexicographicEnumerator.cpp \
  enumerator/FaceAoSLexicographicEnumerator.cpp \
  dg/BoundaryConditions.cpp \
  dg/DGUtils.cpp \
  dg/CellIntegral.cpp \
  dg/Riemann.cpp \
  dg/rusanov/Rusanov.cpp \
  dg/rusanov/tests/RiemannTest.cpp \
  dg/tests/CellIntegralTest.cpp \
  dg/tests/DGUtilsTest.cpp \
  dg/tests/RiemannTest.cpp \
  dg/tests/TestUtils.cpp \
  fd/BoundaryConditions.cpp \
  fd/PatchUtils.cpp \
  fd/LoopBody.cpp \
  fd/tests/CCZ4KernelTest.cpp\
  fd/tests/SommerfeldBCTest.cpp\
  fd/fd4/LoopBody.cpp \
  fd/fd4/FD4_patchwise_functors.cpp \
  fv/BoundaryConditions.cpp \
  fv/PatchUtils.cpp \
  fv/PostprocessingKernels.cpp \
  fv/InterpolationRestriction.cpp \
  fv/Tracer.cpp \
  fv/tests/InterpolationRestrictionTest.cpp \
  fv/tests/CopyPatchTest.cpp \
  fv/tests/ApplySplit1DRiemannToPatchTest.cpp

libExaHyPE2Core2d_a_CXXFLAGS          = $(CXXFLAGS_PEANO_2D_RELEASE)
libExaHyPE2Core2d_trace_a_CXXFLAGS    = $(CXXFLAGS_PEANO_2D_TRACE)
libExaHyPE2Core2d_asserts_a_CXXFLAGS  = $(CXXFLAGS_PEANO_2D_ASSERTS)
libExaHyPE2Core2d_debug_a_CXXFLAGS    = $(CXXFLAGS_PEANO_2D_DEBUG)
libExaHyPE2Core2d_stats_a_CXXFLAGS    = $(CXXFLAGS_PEANO_2D_STATS)

libExaHyPE2Core3d_a_CXXFLAGS          = $(CXXFLAGS_PEANO_3D_RELEASE)
libExaHyPE2Core3d_trace_a_CXXFLAGS    = $(CXXFLAGS_PEANO_3D_TRACE)
libExaHyPE2Core3d_asserts_a_CXXFLAGS  = $(CXXFLAGS_PEANO_3D_ASSERTS)
libExaHyPE2Core3d_debug_a_CXXFLAGS    = $(CXXFLAGS_PEANO_3D_DEBUG)
libExaHyPE2Core3d_stats_a_CXXFLAGS    = $(CXXFLAGS_PEANO_3D_STATS)

libExaHyPE2Core2d_a_SOURCES         = $(ExaHyPE2Sources)
libExaHyPE2Core2d_trace_a_SOURCES   = $(ExaHyPE2Sources)
libExaHyPE2Core2d_asserts_a_SOURCES = $(ExaHyPE2Sources)
libExaHyPE2Core2d_debug_a_SOURCES   = $(ExaHyPE2Sources)
libExaHyPE2Core2d_stats_a_SOURCES   = $(ExaHyPE2Sources)

libExaHyPE2Core3d_a_SOURCES         = $(ExaHyPE2Sources)
libExaHyPE2Core3d_trace_a_SOURCES   = $(ExaHyPE2Sources)
libExaHyPE2Core3d_asserts_a_SOURCES = $(ExaHyPE2Sources)
libExaHyPE2Core3d_debug_a_SOURCES   = $(ExaHyPE2Sources)
libExaHyPE2Core3d_stats_a_SOURCES   = $(ExaHyPE2Sources)
