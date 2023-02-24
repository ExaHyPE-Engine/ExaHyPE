AUTOMAKE_OPTIONS = subdir-objects

if UseExaHyPE
  lib_LIBRARIES = libExaHyPE2Core2d.a  libExaHyPE2Core2d_debug.a  libExaHyPE2Core2d_stats.a  libExaHyPE2Core2d_asserts.a  libExaHyPE2Core2d_trace.a \
                  libExaHyPE2Core3d.a  libExaHyPE2Core3d_debug.a  libExaHyPE2Core3d_stats.a  libExaHyPE2Core3d_asserts.a  libExaHyPE2Core3d_trace.a
endif



ExaHyPE2Sources = \
  CellData.cpp \
  EnclaveBookkeeping.cpp \
  EnclaveTask.cpp \
  LoadBalancingConfiguration.cpp \
  NonCriticalAssertions.cpp \
  PlotFilter.cpp \
  RefinementControl.cpp \
  TimeStepping.cpp \
  UserInterface.cpp \
  UnitTests.cpp \
  aderdg/CorrectorAoS.cpp \
  aderdg/Generic.cpp \
  aderdg/KernelUtils.cpp \
  aderdg/PredictorAoS.cpp \
  aderdg/RiemannAoS.cpp \
  aderdg/RusanovNonlinearAoS.cpp \
  aderdg/tests/ADERDGTest.cpp \
  aderdg_kernels/Basis/GaussLegendreBasis.cpp \
  aderdg_kernels/Basis/GaussLobattoBasis.cpp \
  aderdg_kernels/AderUtils.cpp \
  dg/BoundaryConditions.cpp \
  dg/DGUtils.cpp \
  dg/CellIntegral.cpp \
  dg/Riemann.cpp \
  dg/rusanov/Rusanov.cpp \
  dg/tests/CellIntegralTest.cpp \
  dg/tests/DGUtilsTest.cpp \
  dg/tests/RiemannTest.cpp \
  dg/tests/TestUtils.cpp \
  dg/rusanov/tests/RiemannTest.cpp \
  enumerator/AoSLexicographicEnumerator.cpp \
  enumerator/SoALexicographicEnumerator.cpp \
  enumerator/SingleDoFEnumerator.cpp \
  enumerator/AoSoALexicographicEnumerator.cpp \
  enumerator/FaceAoSLexicographicEnumerator.cpp \
  fd/BoundaryConditions.cpp \
  fd/PatchUtils.cpp \
  fd/LoopBody.cpp \
  fd/fd4/FD4_patchwise_functors.cpp \
  fd/fd4/LoopBody.cpp \
  fv/BoundaryConditions.cpp \
  fv/PatchUtils.cpp \
  fv/PatchAccess.cpp \
  fv/PostprocessingKernels.cpp \
  fv/Tracer.cpp \
  fv/tests/InterpolationRestrictionTest.cpp \
  fd/fd4/FD4_patchwise_functors.cpp \
  fv/rusanov/sycl/GPUCellData.cpp \
  fv/rusanov/tests/CopyPatchTest.cpp \
  fv/rusanov/tests/ApplySplit1DRiemannToPatchTest.cpp


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


