AUTOMAKE_OPTIONS = subdir-objects

if UseExaHyPE
  lib_LIBRARIES = libExaHyPE2Core2d.a  libExaHyPE2Core2d_debug.a  libExaHyPE2Core2d_stats.a  libExaHyPE2Core2d_asserts.a  libExaHyPE2Core2d_trace.a \
                  libExaHyPE2Core3d.a  libExaHyPE2Core3d_debug.a  libExaHyPE2Core3d_stats.a  libExaHyPE2Core3d_asserts.a  libExaHyPE2Core3d_trace.a
endif

ExaHyPE2Sources = \
  CellData.cpp \
  CellAccess.cpp \
  EnclaveBookkeeping.cpp \
  EnclaveTask.cpp \
  LoadBalancingConfiguration.cpp \
  PlotFilter.cpp \
  RefinementControl.cpp \
  PrecisionControl.cpp \
  RefinementControlService.cpp \
  TimeStepping.cpp \
  UserInterface.cpp \
  UnitTests.cpp \
  aderdg/kernels/Basis/GaussLegendreBasis.cpp \
  aderdg/kernels/Basis/GaussLobattoBasis.cpp \
  aderdg/kernels/AderUtils.cpp \
  dg/BoundaryConditions.cpp \
  dg/DGUtils.cpp \
  dg/CellIntegral.cpp \
  dg/Riemann.cpp \
  dg/rusanov/Rusanov.cpp \
  dg/tests/CellIntegralTest.cpp \
  dg/tests/DGUtilsTest.cpp \
  dg/tests/RiemannTest.cpp \
  dg/tests/TestUtils.cpp \
  enumerator/AoSLexicographicEnumerator.cpp \
  enumerator/SoALexicographicEnumerator.cpp \
  enumerator/SingleDoFEnumerator.cpp \
  enumerator/AoSoALexicographicEnumerator.cpp \
  enumerator/FaceAoSLexicographicEnumerator.cpp \
  fd/BoundaryConditions.cpp \
  fd/PatchUtils.cpp \
  fd/LoopBody.cpp \
  fv/BoundaryConditions.cpp \
  fv/PatchUtils.cpp \
  fv/PostprocessingKernels.cpp \
  fv/Tracer.cpp \
  fd/tests/SommerfeldBCTest.cpp\
  fd/tests/CCZ4KernelTest.cpp\
  fv/tests/InterpolationRestrictionTest.cpp \
  fd/fd4/LoopBody.cpp \
  fd/fd4/FD4_patchwise_functors.cpp \
  fv/riemann/Riemann.cpp \
  fv/rusanov/rusanov.cpp \
  fv/rusanov/sycl/GPUCellData.cpp \
  dg/rusanov/tests/RiemannTest.cpp \
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

if GPUOffloadingCUDA
ExaHyPE2_CUDA_OBJS_2D					= tests/libExaHyPE2Core2d_a-KernelLaunchTest.o
ExaHyPE2_trace_CUDA_OBJS_2D		= tests/libExaHyPE2Core2d_trace_a-KernelLaunchTest.o
ExaHyPE2_asserts_CUDA_OBJS_2D	= tests/libExaHyPE2Core2d_asserts_a-KernelLaunchTest.o
ExaHyPE2_debug_CUDA_OBJS_2D		= tests/libExaHyPE2Core2d_debug_a-KernelLaunchTest.o
ExaHyPE2_stats_CUDA_OBJS_2D		= tests/libExaHyPE2Core2d_stats_a-KernelLaunchTest.o

$(ExaHyPE2_CUDA_OBJS_2D): tests/libExaHyPE2Core2d_a-%.o: tests/%.cu
	$(NVCC) $(NVCC_FLAGS_RELEASE) $(CXXFLAGS_PEANO_2D_RELEASE) -c $< -o $@

$(ExaHyPE2_trace_CUDA_OBJS_2D): tests/libExaHyPE2Core2d_trace_a-%.o: tests/%.cu
	$(NVCC) $(NVCC_FLAGS_TRACE) $(CXXFLAGS_PEANO_2D_TRACE) -c $< -o $@

$(ExaHyPE2_asserts_CUDA_OBJS_2D): tests/libExaHyPE2Core2d_asserts_a-%.o: tests/%.cu
	$(NVCC) $(NVCC_FLAGS_ASSERTS) $(CXXFLAGS_PEANO_2D_ASSERTS) -c $< -o $@

$(ExaHyPE2_debug_CUDA_OBJS_2D): tests/libExaHyPE2Core2d_debug_a-%.o: tests/%.cu
	$(NVCC) $(NVCC_FLAGS_DEBUG) $(CXXFLAGS_PEANO_2D_DEBUG) -c $< -o $@

$(ExaHyPE2_stats_CUDA_OBJS_2D): tests/libExaHyPE2Core2d_stats_a-%.o: tests/%.cu
	$(NVCC) $(NVCC_FLAGS_STATS) $(CXXFLAGS_PEANO_2D_STATS) -c $< -o $@

libExaHyPE2Core2d_a_LIBADD          = $(ExaHyPE2_CUDA_OBJS_2D)
libExaHyPE2Core2d_trace_a_LIBADD    = $(ExaHyPE2_trace_CUDA_OBJS_2D)
libExaHyPE2Core2d_asserts_a_LIBADD  = $(ExaHyPE2_asserts_CUDA_OBJS_2D)
libExaHyPE2Core2d_debug_a_LIBADD    = $(ExaHyPE2_debug_CUDA_OBJS_2D)
libExaHyPE2Core2d_stats_a_LIBADD    = $(ExaHyPE2_stats_CUDA_OBJS_2D)

ExaHyPE2_CUDA_OBJS_3D					= tests/libExaHyPE2Core3d_a-KernelLaunchTest.o
ExaHyPE2_trace_CUDA_OBJS_3D		= tests/libExaHyPE2Core3d_trace_a-KernelLaunchTest.o
ExaHyPE2_asserts_CUDA_OBJS_3D	= tests/libExaHyPE2Core3d_asserts_a-KernelLaunchTest.o
ExaHyPE2_debug_CUDA_OBJS_3D		= tests/libExaHyPE2Core3d_debug_a-KernelLaunchTest.o
ExaHyPE2_stats_CUDA_OBJS_3D		= tests/libExaHyPE2Core3d_stats_a-KernelLaunchTest.o

$(ExaHyPE2_CUDA_OBJS_3D): tests/libExaHyPE2Core3d_a-%.o: tests/%.cu
	$(NVCC) $(NVCC_FLAGS_RELEASE) $(CXXFLAGS_PEANO_3D_RELEASE) -c $< -o $@

$(ExaHyPE2_trace_CUDA_OBJS_3D): tests/libExaHyPE2Core3d_trace_a-%.o: tests/%.cu
	$(NVCC) $(NVCC_FLAGS_TRACE) $(CXXFLAGS_PEANO_3D_TRACE) -c $< -o $@

$(ExaHyPE2_asserts_CUDA_OBJS_3D): tests/libExaHyPE2Core3d_asserts_a-%.o: tests/%.cu
	$(NVCC) $(NVCC_FLAGS_ASSERTS) $(CXXFLAGS_PEANO_3D_ASSERTS) -c $< -o $@

$(ExaHyPE2_debug_CUDA_OBJS_3D): tests/libExaHyPE2Core3d_debug_a-%.o: tests/%.cu
	$(NVCC) $(NVCC_FLAGS_DEBUG) $(CXXFLAGS_PEANO_3D_DEBUG) -c $< -o $@

$(ExaHyPE2_stats_CUDA_OBJS_3D): tests/libExaHyPE2Core3d_stats_a-%.o: tests/%.cu
	$(NVCC) $(NVCC_FLAGS_STATS) $(CXXFLAGS_PEANO_3D_STATS) -c $< -o $@

libExaHyPE2Core3d_a_LIBADD          = $(ExaHyPE2_CUDA_OBJS_3D)
libExaHyPE2Core3d_trace_a_LIBADD    = $(ExaHyPE2_trace_CUDA_OBJS_3D)
libExaHyPE2Core3d_asserts_a_LIBADD  = $(ExaHyPE2_asserts_CUDA_OBJS_3D)
libExaHyPE2Core3d_debug_a_LIBADD    = $(ExaHyPE2_debug_CUDA_OBJS_3D)
libExaHyPE2Core3d_stats_a_LIBADD    = $(ExaHyPE2_stats_CUDA_OBJS_3D)
endif
