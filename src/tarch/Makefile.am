# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

AUTOMAKE_OPTIONS = subdir-objects

lib_LIBRARIES = libTarch.a libTarch_debug.a libTarch_asserts.a libTarch_trace.a libTarch_stats.a

Tarch_SOURCES = \
	tarch.cpp \
	UnitTests.cpp \
	memory/memory.cpp \
	memory/MemoryLocation.cpp \
	memory/MemoryManager.cpp \
	memory/tests/MemoryManagerTest.cpp \
	la/tests/DynamicMatrixTest.cpp \
	la/tests/MatrixVectorTest.cpp \
	la/tests/MatrixTest.cpp \
	la/tests/ScalarTest.cpp \
	la/tests/GramSchmidtTest.cpp \
	la/tests/LUDecompositionTest.cpp \
	la/tests/VectorTest.cpp \
	la/ScalarOperations.cpp \
	la/Scalar.cpp \
	la/DynamicMatrix.cpp \
	accelerator/accelerator.cpp \
	accelerator/Device.cpp \
	accelerator/DeviceInfo.cpp \
	accelerator/cpp/accelerator.cpp \
	accelerator/cpp/Device.cpp \
	accelerator/cpp/DeviceInfo.cpp \
	accelerator/omp/accelerator.cpp \
	accelerator/omp/Device.cpp \
	accelerator/omp/DeviceInfo.cpp \
	accelerator/sycl/accelerator.cpp \
	accelerator/sycl/Device.cpp \
	accelerator/sycl/DeviceInfo.cpp \
	accelerator/cuda/accelerator.cpp \
	accelerator/cuda/ErrorCheck.cpp \
	accelerator/cuda/DeviceInfo.cpp \
	accelerator/cuda/Device.cpp \
	multicore/BooleanSemaphore.cpp \
	multicore/multicore.cpp \
	multicore/Core.cpp \
	multicore/Lock.cpp \
	multicore/RecursiveLock.cpp \
	multicore/RecursiveSemaphore.cpp \
	multicore/smartScheduling.cpp \
	multicore/Tasks.cpp \
	multicore/cpp/multicore.cpp \
	multicore/cpp/BooleanSemaphore.cpp \
	multicore/cpp/Core.cpp \
	multicore/cpp/RecursiveSemaphore.cpp \
	multicore/cpp/Tasks.cpp \
	multicore/omp/multicore.cpp \
	multicore/omp/BooleanSemaphore.cpp \
	multicore/omp/Core.cpp \
	multicore/omp/RecursiveSemaphore.cpp \
	multicore/omp/Tasks.cpp \
	multicore/tbb/multicore.cpp \
	multicore/tbb/BooleanSemaphore.cpp \
	multicore/tbb/Core.cpp \
	multicore/tbb/RecursiveSemaphore.cpp \
	multicore/tbb/Tasks.cpp \
	multicore/sycl/multicore.cpp \
	multicore/sycl/BooleanSemaphore.cpp \
	multicore/sycl/Core.cpp \
	multicore/sycl/RecursiveSemaphore.cpp \
	multicore/sycl/Tasks.cpp \
	multicore/orchestration/AllOnGPU.cpp \
	multicore/orchestration/BackfillAndDeployRoundRobin.cpp \
	multicore/orchestration/Hardcoded.cpp \
	multicore/orchestration/StrategyFactory.cpp \
	tests/TestCaseCollection.cpp \
	tests/TreeTestCaseCollection.cpp \
	tests/TestCase.cpp \
	plotter/griddata/unstructured/vtk/VTUTextFileWriter_CellWriter.cpp \
	plotter/griddata/unstructured/vtk/VTKTextFileWriter_VertexDataWriter.cpp \
	plotter/griddata/unstructured/vtk/VTKTextFileWriter_VertexWriter.cpp \
	plotter/griddata/unstructured/vtk/tests/VTKBinaryFileTestCase.cpp \
	plotter/griddata/unstructured/vtk/VTKBinaryFileWriter_CellWriter.cpp \
	plotter/griddata/unstructured/vtk/VTUTextFileWriter_CellDataWriter.cpp \
	plotter/griddata/unstructured/vtk/VTKTextFileWriter_CellWriter.cpp \
	plotter/griddata/unstructured/vtk/VTUTextFileWriter_VertexDataWriter.cpp \
	plotter/griddata/unstructured/vtk/VTKBinaryFileWriter.cpp \
	plotter/griddata/unstructured/vtk/VTUTextFileWriter.cpp \
	plotter/griddata/unstructured/vtk/VTKBinaryFileWriter_VertexWriter.cpp \
	plotter/griddata/unstructured/vtk/VTKBinaryFileWriter_CellDataWriter.cpp \
	plotter/griddata/unstructured/vtk/VTKTextFileWriter_CellDataWriter.cpp \
	plotter/griddata/unstructured/vtk/VTKBinaryFileWriter_VertexDataWriter.cpp \
	plotter/griddata/unstructured/vtk/VTUTextFileWriter_VertexWriter.cpp \
	plotter/griddata/unstructured/vtk/VTKTextFileWriter.cpp \
	plotter/PVDTimeSeriesWriter.cpp \
	plotter/griddata/blockstructured/PeanoTextPatchFileWriter.cpp \
	plotter/griddata/blockstructured/PeanoTextPatchFileWriter_VertexDataWriter.cpp \
	plotter/griddata/blockstructured/PeanoHDF5PatchFileWriter.cpp \
	plotter/griddata/blockstructured/PeanoHDF5PatchFileWriter_CellDataWriter.cpp \
	plotter/griddata/blockstructured/PeanoHDF5PatchFileWriter_VertexDataWriter.cpp \
	plotter/griddata/blockstructured/PeanoTextPatchFileWriter_CellDataWriter.cpp \
	plotter/pointdata/vtk/VTKWriter.cpp \
	mpi/BooleanSemaphore.cpp \
	mpi/Lock.cpp \
	mpi/Rank.cpp \
	mpi/mpi.cpp \
	mpi/DoubleMessage.cpp \
	mpi/IntegerMessage.cpp \
	mpi/StringMessage.cpp \
	mpi/tests/StringTest.cpp \
	logging/CommandLineLogger.cpp \
	logging/ChromeTraceFileLogger.cpp \
	logging/ITACLogger.cpp \
	logging/ITTLogger.cpp \
	logging/Log.cpp \
	logging/LogFilterFileReader.cpp \
	logging/NVTXLogger.cpp \
	logging/ScorePLogger.cpp \
	logging/LogFilter.cpp \
	logging/Statistics.cpp \
	timing/Watch.cpp \
	timing/Measurement.cpp \
	timing/GlidingAverageMeasurement.cpp \
	services/Service.cpp \
	services/ServiceRepository.cpp

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

if GPUOffloadingCUDA
# The pattern syntax is necessary to compile CUDA source files to multiple targets.
# Pattern syntax is a GNU extension therefore we can't use it (due to treat-warnings-as-error).
# Therefore, we need to statistically list all of the object files.
# Any kind of for-loops are GNU extensions too, therefore one has to do it per hand and
# due to the limitations of the pattern rule, the rules have to be defined per directory.
# I will give some examples for GNU extensions, as one will surely ask, why do you not use a for loop?
# CUDA_OBJS_RELEASE = $(addprefix libTarch_a-, $(CUDA_OBJS))
# $(foreach item,$(ITEMS),libTarch_stats_a-$(item)) is a GNU extension
# CUDA_OBJS_RELEASE = $(patsubst %.cu,libTarch_a-%.o, $(ITEMS))
# libTarch_debug_a-%.o: %.cu again is a GNU extension (% is a GNU extension) unless
# $(CUDA_OBJS_RELEASE): libTarch_a-%.o: %.cu (static pattern).
# Also see here: https://stackoverflow.com/questions/27975545/how-to-deal-with-autoconf-warning-style-pattern-rules-are-a-gnu-make-extens

# The CUDA objects added with LDADD are also tracked as dependencies.
# Therefore adding the sources to the <target>_SOURCES is not necessary.
Tarch_CUDA_OBJS					= accelerator/cuda/libTarch_a-TestKernels.o
Tarch_stats_CUDA_OBJS		= accelerator/cuda/libTarch_stats_a-TestKernels.o
Tarch_trace_CUDA_OBJS		= accelerator/cuda/libTarch_trace_a-TestKernels.o
Tarch_asserts_CUDA_OBJS	= accelerator/cuda/libTarch_asserts_a-TestKernels.o
Tarch_debug_CUDA_OBJS		= accelerator/cuda/libTarch_debug_a-TestKernels.o

# Common flags given to nvcc is used in both device and host code.
# These should be optimisation flags (e.g., -O3), warning flags (e.g., -Wall),
# debugging flags (e.g., -g), std flags (e.g., -std=c++20), definitions (e.g., -DPEANO_DEBUG=4),
# include paths (e.g., -I/path/...) or linker flags (e.g., -L/path/...).
# In case these do not work, you might need to do $(FLAGS) -Xcompiler="$(FLAGS)" but then also
# split the $(CXX) to compiler=CXX[0] and args=CXX[1:] and to $(args) -Xcompiler="$(args) if not
# using a common flag that needs to be replicated.
$(Tarch_CUDA_OBJS): accelerator/cuda/libTarch_a-%.o: accelerator/cuda/%.cu
	$(NVCC) $(NVCC_FLAGS_RELEASE) $(CXXFLAGS_PEANO_2D_RELEASE) -c $< -o $@

$(Tarch_stats_CUDA_OBJS): accelerator/cuda/libTarch_stats_a-%.o: accelerator/cuda/%.cu
	$(NVCC) $(NVCC_FLAGS_STATS) $(CXXFLAGS_PEANO_2D_STATS) -c $< -o $@

$(Tarch_trace_CUDA_OBJS): accelerator/cuda/libTarch_trace_a-%.o: accelerator/cuda/%.cu
	$(NVCC) $(NVCC_FLAGS_TRACE) $(CXXFLAGS_PEANO_2D_TRACE) -c $< -o $@

$(Tarch_asserts_CUDA_OBJS): accelerator/cuda/libTarch_asserts_a-%.o: accelerator/cuda/%.cu
	$(NVCC) $(NVCC_FLAGS_ASSERTS) $(CXXFLAGS_PEANO_2D_ASSERTS) -c $< -o $@

$(Tarch_debug_CUDA_OBJS): accelerator/cuda/libTarch_debug_a-%.o: accelerator/cuda/%.cu
	$(NVCC) $(NVCC_FLAGS_DEBUG) $(CXXFLAGS_PEANO_2D_DEBUG) -c $< -o $@

# Adding $(CUDA_LIBS) to libTarch is not necessary as it is only needed for executables and it is already added
# to PEANO_LDADD; having it twice might cause issues.
libTarch_a_LIBADD         = $(Tarch_CUDA_OBJS)
libTarch_stats_a_LIBADD   = $(Tarch_stats_CUDA_OBJS)
libTarch_trace_a_LIBADD   = $(Tarch_trace_CUDA_OBJS)
libTarch_asserts_a_LIBADD = $(Tarch_asserts_CUDA_OBJS)
libTarch_debug_a_LIBADD   = $(Tarch_debug_CUDA_OBJS)
endif
