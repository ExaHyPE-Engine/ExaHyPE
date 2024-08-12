AUTOMAKE_OPTIONS = subdir-objects

#
# By default, automake does dfs. We need the libs to be built first however.
# So we add the . explicitly. Please note that it is not enough to add your
# directory here. You also have to add any new directory to the root's
# configure.ac script and rerun the whole autotool pipeline (see comment
# within configure.ac as well as its very last line).
#
SUBDIRS = . \
	toolbox/blockstructured \
	toolbox/finiteelements \
	toolbox/loadbalancing \
	toolbox/multiprecision \
	toolbox/particles \
	convert \
	exahype2 \
	mghype \
	petsc \
	swift2 \
	tbb \
	unittests

#
# I want Peano to be as fast as possible and thus build it as static library
# such that a linker/compiler can ipo. One of the other reasons for this is
# that I have a couple of singletons and I did struggle with singletons and
# shared libraries.
#
lib_LIBRARIES = libTarch.a        libTarch_debug.a        libTarch_asserts.a        libTarch_trace.a        libTarch_stats.a \
								libPeano4Core2d.a libPeano4Core2d_debug.a libPeano4Core2d_asserts.a libPeano4Core2d_trace.a libPeano4Core2d_stats.a \
								libPeano4Core3d.a libPeano4Core3d_debug.a libPeano4Core3d_asserts.a libPeano4Core3d_trace.a libPeano4Core3d_stats.a

Tarch_SOURCES = tarch/tarch.cpp \
	tarch/Enumerator.cpp \
	tarch/NonCriticalAssertions.cpp \
	tarch/UnitTests.cpp \
	tarch/la/tests/DynamicMatrixTest.cpp \
	tarch/la/tests/MatrixVectorTest.cpp \
	tarch/la/tests/MatrixTest.cpp \
	tarch/la/tests/ScalarTest.cpp \
	tarch/la/tests/GramSchmidtTest.cpp \
	tarch/la/tests/LUDecompositionTest.cpp \
	tarch/la/tests/VectorTest.cpp \
	tarch/la/ScalarOperations.cpp \
	tarch/la/Scalar.cpp \
	tarch/la/DynamicMatrix.cpp \
	tarch/accelerator/accelerator.cpp \
	tarch/accelerator/Device.cpp \
	tarch/accelerator/cpp/accelerator.cpp \
	tarch/accelerator/cpp/Device.cpp \
	tarch/accelerator/omp/accelerator.cpp \
	tarch/accelerator/omp/Device.cpp \
	tarch/accelerator/omp/GPUMemoryManager.cpp \
	tarch/accelerator/sycl/accelerator.cpp \
	tarch/accelerator/sycl/Device.cpp \
	tarch/accelerator/sycl/GPUMemoryManager.cpp \
	tarch/accelerator/cuda/ErrorCheck.cpp \
	tarch/accelerator/cuda/Device.cpp \
	tarch/accelerator/cuda/accelerator.cpp \
	tarch/accelerator/hip/accelerator.cpp \
	tarch/multicore/BooleanSemaphore.cpp \
	tarch/multicore/multicore.cpp \
	tarch/multicore/Core.cpp \
	tarch/multicore/Lock.cpp \
	tarch/multicore/MultiReadSingleWriteSemaphore.cpp \
	tarch/multicore/MultiReadSingleWriteLock.cpp \
	tarch/multicore/RecursiveLock.cpp \
	tarch/multicore/RecursiveSemaphore.cpp \
	tarch/multicore/smartScheduling.cpp \
	tarch/multicore/Tasks.cpp \
	tarch/multicore/cpp/multicore.cpp \
	tarch/multicore/cpp/BooleanSemaphore.cpp \
	tarch/multicore/cpp/Core.cpp \
	tarch/multicore/cpp/RecursiveSemaphore.cpp \
	tarch/multicore/cpp/Tasks.cpp \
	tarch/multicore/omp/multicore.cpp \
	tarch/multicore/omp/BooleanSemaphore.cpp \
	tarch/multicore/omp/Core.cpp \
	tarch/multicore/omp/RecursiveSemaphore.cpp \
	tarch/multicore/omp/Tasks.cpp \
	tarch/multicore/tbb/multicore.cpp \
	tarch/multicore/tbb/BooleanSemaphore.cpp \
	tarch/multicore/tbb/Core.cpp \
	tarch/multicore/tbb/RecursiveSemaphore.cpp \
	tarch/multicore/tbb/Tasks.cpp \
	tarch/multicore/sycl/multicore.cpp \
	tarch/multicore/sycl/BooleanSemaphore.cpp \
	tarch/multicore/sycl/Core.cpp \
	tarch/multicore/sycl/RecursiveSemaphore.cpp \
	tarch/multicore/sycl/Tasks.cpp \
	tarch/multicore/orchestration/AllOnGPU.cpp \
	tarch/multicore/orchestration/BackfillAndDeployRoundRobin.cpp \
	tarch/multicore/orchestration/Hardcoded.cpp \
	tarch/multicore/orchestration/StrategyFactory.cpp \
	tarch/tests/TestCaseCollection.cpp \
	tarch/tests/TreeTestCaseCollection.cpp \
	tarch/tests/TestCase.cpp \
	tarch/plotter/griddata/unstructured/vtk/VTUTextFileWriter_CellWriter.cpp \
	tarch/plotter/griddata/unstructured/vtk/VTKTextFileWriter_VertexDataWriter.cpp \
	tarch/plotter/griddata/unstructured/vtk/VTKTextFileWriter_VertexWriter.cpp \
	tarch/plotter/griddata/unstructured/vtk/tests/VTKBinaryFileTestCase.cpp \
	tarch/plotter/griddata/unstructured/vtk/VTKBinaryFileWriter_CellWriter.cpp \
	tarch/plotter/griddata/unstructured/vtk/VTUTextFileWriter_CellDataWriter.cpp \
	tarch/plotter/griddata/unstructured/vtk/VTKTextFileWriter_CellWriter.cpp \
	tarch/plotter/griddata/unstructured/vtk/VTUTextFileWriter_VertexDataWriter.cpp \
	tarch/plotter/griddata/unstructured/vtk/VTKBinaryFileWriter.cpp \
	tarch/plotter/griddata/unstructured/vtk/VTUTextFileWriter.cpp \
	tarch/plotter/griddata/unstructured/vtk/VTKBinaryFileWriter_VertexWriter.cpp \
	tarch/plotter/griddata/unstructured/vtk/VTKBinaryFileWriter_CellDataWriter.cpp \
	tarch/plotter/griddata/unstructured/vtk/VTKTextFileWriter_CellDataWriter.cpp \
	tarch/plotter/griddata/unstructured/vtk/VTKBinaryFileWriter_VertexDataWriter.cpp \
	tarch/plotter/griddata/unstructured/vtk/VTUTextFileWriter_VertexWriter.cpp \
	tarch/plotter/griddata/unstructured/vtk/VTKTextFileWriter.cpp \
	tarch/plotter/PVDTimeSeriesWriter.cpp \
	tarch/plotter/griddata/blockstructured/PeanoTextPatchFileWriter.cpp \
	tarch/plotter/griddata/blockstructured/PeanoTextPatchFileWriter_VertexDataWriter.cpp \
	tarch/plotter/griddata/blockstructured/PeanoHDF5PatchFileWriter.cpp \
	tarch/plotter/griddata/blockstructured/PeanoHDF5PatchFileWriter_CellDataWriter.cpp \
	tarch/plotter/griddata/blockstructured/PeanoHDF5PatchFileWriter_VertexDataWriter.cpp \
	tarch/plotter/griddata/blockstructured/PeanoTextPatchFileWriter_CellDataWriter.cpp \
	tarch/plotter/pointdata/vtk/VTKWriter.cpp \
	tarch/mpi/BooleanSemaphore.cpp \
	tarch/mpi/Lock.cpp \
	tarch/mpi/Rank.cpp \
	tarch/mpi/mpi.cpp \
	tarch/mpi/DoubleMessage.cpp \
	tarch/mpi/IntegerMessage.cpp \
	tarch/mpi/StringMessage.cpp \
	tarch/mpi/tests/StringTest.cpp \
	tarch/logging/CommandLineLogger.cpp \
	tarch/logging/ChromeTraceFileLogger.cpp \
	tarch/logging/ITACLogger.cpp \
	tarch/logging/ITTLogger.cpp \
	tarch/logging/Log.cpp \
	tarch/logging/LogFilterFileReader.cpp \
	tarch/logging/NVTXLogger.cpp \
	tarch/logging/ScorePLogger.cpp \
	tarch/logging/LogFilter.cpp \
	tarch/logging/Statistics.cpp \
	tarch/timing/Watch.cpp \
	tarch/timing/Measurement.cpp \
	tarch/timing/GlidingAverageMeasurement.cpp \
	tarch/services/Service.cpp \
	tarch/services/ServiceRepository.cpp

libTarch_a_SOURCES         = $(Tarch_SOURCES)
libTarch_stats_a_SOURCES   = $(Tarch_SOURCES)
libTarch_trace_a_SOURCES   = $(Tarch_SOURCES)
libTarch_asserts_a_SOURCES = $(Tarch_SOURCES)
libTarch_debug_a_SOURCES   = $(Tarch_SOURCES)

libTarch_a_CXXFLAGS         = $(CXXFLAGS_PEANO_2D_RELEASE)
libTarch_trace_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_TRACE)
libTarch_asserts_a_CXXFLAGS = $(CXXFLAGS_PEANO_2D_ASSERTS)
libTarch_debug_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_DEBUG)
libTarch_stats_a_CXXFLAGS   = $(CXXFLAGS_PEANO_2D_STATS)

Peano4Core_SOURCES = \
	peano4/datamanagement/CellMarker.cpp \
	peano4/datamanagement/FaceMarker.cpp \
	peano4/datamanagement/VertexMarker.cpp \
	peano4/peano.cpp \
	peano4/UnitTests.cpp \
	peano4/grid/AutomatonState.cpp \
	peano4/grid/EmptyTraversalObserver.cpp \
	peano4/grid/grid.cpp \
	peano4/grid/GridControlEvent.cpp \
	peano4/grid/GridVertex.cpp \
	peano4/grid/GridStatistics.cpp \
	peano4/grid/GridTraversalEvent.cpp \
	peano4/grid/GridTraversalEventGenerator.cpp \
	peano4/grid/LoadStoreComputeFlag.cpp \
	peano4/grid/PeanoCurve.cpp \
	peano4/grid/Spacetree.cpp \
	peano4/grid/TraversalObserver.cpp \
	peano4/grid/TraversalVTKPlotter.cpp \
	peano4/grid/tests/GridControlEventTest.cpp \
	peano4/grid/tests/SpacetreeTest.cpp \
	peano4/grid/tests/GridTraversalEventGeneratorTest.cpp \
	peano4/parallel/Node.cpp \
	peano4/parallel/parallel.cpp \
	peano4/parallel/SpacetreeSet.cpp \
	peano4/parallel/StartTraversalMessage.cpp \
	peano4/parallel/TreeEntry.cpp \
	peano4/parallel/TreeManagementMessage.cpp \
	peano4/parallel/tests/PingPongTest.cpp \
	peano4/parallel/tests/NodeTest.cpp \
	peano4/stacks/stacks.cpp \
	peano4/stacks/STDVectorStack.cpp \
	peano4/utils/Loop.cpp \
	peano4/utils/tests/ParallelDForTest.cpp
	

libPeano4Core2d_a_SOURCES         = $(Peano4Core_SOURCES)
libPeano4Core2d_trace_a_SOURCES   = $(Peano4Core_SOURCES)
libPeano4Core2d_asserts_a_SOURCES = $(Peano4Core_SOURCES)
libPeano4Core2d_debug_a_SOURCES   = $(Peano4Core_SOURCES)
libPeano4Core2d_stats_a_SOURCES   = $(Peano4Core_SOURCES)

libPeano4Core3d_a_SOURCES         = $(Peano4Core_SOURCES)
libPeano4Core3d_trace_a_SOURCES   = $(Peano4Core_SOURCES)
libPeano4Core3d_asserts_a_SOURCES = $(Peano4Core_SOURCES)
libPeano4Core3d_debug_a_SOURCES   = $(Peano4Core_SOURCES)
libPeano4Core3d_stats_a_SOURCES   = $(Peano4Core_SOURCES)

libPeano4Core2d_a_CXXFLAGS          = $(CXXFLAGS_PEANO_2D_RELEASE)
libPeano4Core2d_stats_a_CXXFLAGS    = $(CXXFLAGS_PEANO_2D_STATS)
libPeano4Core2d_trace_a_CXXFLAGS    = $(CXXFLAGS_PEANO_2D_TRACE)
libPeano4Core2d_asserts_a_CXXFLAGS  = $(CXXFLAGS_PEANO_2D_ASSERTS)
libPeano4Core2d_debug_a_CXXFLAGS    = $(CXXFLAGS_PEANO_2D_DEBUG)

libPeano4Core3d_a_CXXFLAGS          = $(CXXFLAGS_PEANO_3D_RELEASE)
libPeano4Core3d_stats_a_CXXFLAGS    = $(CXXFLAGS_PEANO_3D_STATS)
libPeano4Core3d_trace_a_CXXFLAGS    = $(CXXFLAGS_PEANO_3D_TRACE)
libPeano4Core3d_asserts_a_CXXFLAGS  = $(CXXFLAGS_PEANO_3D_ASSERTS)
libPeano4Core3d_debug_a_CXXFLAGS    = $(CXXFLAGS_PEANO_3D_DEBUG)

if GPUOffloadingCUDA
# The pattern syntax is necessary to compile CUDA source files to multiple targets.
# Pattern syntax is a GNU extension therefore we can't use it (due to treat-warnings-as-error).
# Therefore, we need to statistically list all of the object files.
# Any kind of for-loops are GNU extensions too, therefore one has to do it per hand and
# due to the limitations of the pattern rule, the rules have to be defined per directory.
# I will give some examples for GNU extensions, as one will surely ask, why do not use a for loop?
# CUDA_OBJS_RELEASE = $(addprefix libTarch_a-, $(CUDA_OBJS))
# $(foreach item,$(ITEMS),libTarch_stats_a-$(item)) is a GNU extension
# CUDA_OBJS_RELEASE = $(patsubst %.cu,libTarch_a-%.o, $(ITEMS))
# libTarch_debug_a-%.o: %.cu again a GNU extension (% is a GNU Extension) unless
# $(CUDA_OBJS_RELEASE): libTarch_a-%.o: %.cu (static pattern)
# also see here:
# https://stackoverflow.com/questions/27975545/how-to-deal-with-autoconf-warning-style-pattern-rules-are-a-gnu-make-extens

# The CUDA objects added with LDADD are also tracked as dependencies
# Therefore adding the sources to the <target>_Sources not necessary
Tarch_CUDA_OBJS					= tarch/accelerator/cuda/libTarch_a-TestKernels.o
Tarch_stats_CUDA_OBJS		= tarch/accelerator/cuda/libTarch_stats_a-TestKernels.o
Tarch_trace_CUDA_OBJS		= tarch/accelerator/cuda/libTarch_trace_a-TestKernels.o
Tarch_asserts_CUDA_OBJS	= tarch/accelerator/cuda/libTarch_asserts_a-TestKernels.o
Tarch_debug_CUDA_OBJS		= tarch/accelerator/cuda/libTarch_debug_a-TestKernels.o

# Common flags given to nvcc is used in both device and host code.
# These should be optimisation flags (e.g., -O3), warning flags (e.g. -Wall),
# debugging flags (e.g., -g), std flags (e.g., -std=c++20), definitions (e.g., -DPEANO_DEBUG=4),
# include paths (e.g., -I/path/...) or linker flags (e.g., -L/path/...).
# In case these do not work, you might need to do $(FLAGS) -Xcompiler="$(FLAGS)" but then also
# split the $(CXX) to compiler=CXX[0] and args=CXX[1:] and to $(args) -Xcompiler="$(args) if not
# using a common flag that needs to be replicated.
$(Tarch_CUDA_OBJS): tarch/accelerator/cuda/libTarch_a-%.o: tarch/accelerator/cuda/%.cu
	$(NVCC) $(NVCC_FLAGS_RELEASE) $(CXXFLAGS_PEANO_2D_RELEASE) -c $< -o $@

$(Tarch_stats_CUDA_OBJS): tarch/accelerator/cuda/libTarch_stats_a-%.o: tarch/accelerator/cuda/%.cu
	$(NVCC) $(NVCC_FLAGS_STATS) $(CXXFLAGS_PEANO_2D_STATS) -c $< -o $@

$(Tarch_trace_CUDA_OBJS): tarch/accelerator/cuda/libTarch_trace_a-%.o: tarch/accelerator/cuda/%.cu
	$(NVCC) $(NVCC_FLAGS_TRACE) $(CXXFLAGS_PEANO_2D_TRACE) -c $< -o $@

$(Tarch_asserts_CUDA_OBJS): tarch/accelerator/cuda/libTarch_asserts_a-%.o: tarch/accelerator/cuda/%.cu
	$(NVCC) $(NVCC_FLAGS_ASSERTS) $(CXXFLAGS_PEANO_2D_ASSERTS) -c $< -o $@

$(Tarch_debug_CUDA_OBJS): tarch/accelerator/cuda/libTarch_debug_a-%.o: tarch/accelerator/cuda/%.cu
	$(NVCC) $(NVCC_FLAGS_DEBUG) $(CXXFLAGS_PEANO_2D_DEBUG) -c $< -o $@

# Adding $(CUDA_LIBS) to libTarch is not necessary as it is only needed for executables and it is already added
# to PEANO_LDADD; having it twice might cause issues.
libTarch_a_LIBADD         = $(Tarch_CUDA_OBJS)
libTarch_stats_a_LIBADD   = $(Tarch_stats_CUDA_OBJS)
libTarch_trace_a_LIBADD   = $(Tarch_trace_CUDA_OBJS)
libTarch_asserts_a_LIBADD = $(Tarch_asserts_CUDA_OBJS)
libTarch_debug_a_LIBADD   = $(Tarch_debug_CUDA_OBJS)
endif
