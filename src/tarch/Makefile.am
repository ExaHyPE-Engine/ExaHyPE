# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

AUTOMAKE_OPTIONS = subdir-objects

lib_LIBRARIES = libTarch.a libTarch_debug.a libTarch_asserts.a libTarch_trace.a libTarch_stats.a

Tarch_SOURCES = \
	tarch.cpp \
	Enumerator.cpp \
	NonCriticalAssertions.cpp \
	UnitTests.cpp \
	tests/TestCaseCollection.cpp \
	tests/TreeTestCaseCollection.cpp \
	tests/TestCase.cpp \
	la/Scalar.cpp \
	la/Vector.cpp \
	la/DynamicMatrix.cpp \
	la/DynamicMatrixOperations.cpp \
	la/GramSchmidt.cpp \
	la/LUDecompositionDynamic.cpp \
	la/tests/DynamicMatrixTest.cpp \
	la/tests/MatrixVectorTest.cpp \
	la/tests/MatrixTest.cpp \
	la/tests/ScalarTest.cpp \
	la/tests/GramSchmidtTest.cpp \
	la/tests/LUDecompositionTest.cpp \
	la/tests/VectorTest.cpp \
	accelerator/accelerator.cpp \
	accelerator/Device.cpp \
	accelerator/MemoryLocation.cpp \
	accelerator/MemoryManager.cpp \
	accelerator/tests/MemoryManagerTest.cpp \
	accelerator/cpp/accelerator.cpp \
	accelerator/cpp/Device.cpp \
	accelerator/omp/accelerator.cpp \
	accelerator/omp/Device.cpp \
	accelerator/sycl/accelerator.cpp \
	accelerator/sycl/Device.cpp \
	accelerator/cuda/accelerator.cpp \
	accelerator/cuda/Device.cpp \
	accelerator/cuda/DevicePropertiesCache.cpp \
	accelerator/cuda/ErrorCheck.cpp \
	accelerator/cuda/ExecutionPolicy.cpp \
	accelerator/kokkos/accelerator.cpp \
	accelerator/kokkos/Device.cpp \
	accelerator/hip/accelerator.cpp \
	multicore/BooleanSemaphore.cpp \
	multicore/multicore.cpp \
	multicore/Core.cpp \
	multicore/Lock.cpp \
	multicore/MultiReadSingleWriteSemaphore.cpp \
	multicore/MultiReadSingleWriteLock.cpp \
	multicore/RecursiveLock.cpp \
	multicore/RecursiveSemaphore.cpp \
	multicore/Task.cpp \
	multicore/cpp/multicore.cpp \
	multicore/cpp/BooleanSemaphore.cpp \
	multicore/cpp/Core.cpp \
	multicore/cpp/RecursiveSemaphore.cpp \
	multicore/cpp/Tasks.cpp \
	multicore/omp/multicore.cpp \
	multicore/omp/BooleanSemaphore.cpp \
	multicore/omp/Core.cpp \
	multicore/omp/RecursiveSemaphore.cpp \
	multicore/tbb/multicore.cpp \
	multicore/tbb/BooleanSemaphore.cpp \
	multicore/tbb/Core.cpp \
	multicore/tbb/MultiReadSingleWriteSemaphore.cpp \
	multicore/tbb/RecursiveSemaphore.cpp \
	multicore/tbb/Tasks.cpp \
	multicore/tbb/TaskHandleRepository.cpp \
	multicore/sycl/multicore.cpp \
	multicore/sycl/BooleanSemaphore.cpp \
	multicore/sycl/Core.cpp \
	multicore/sycl/RecursiveSemaphore.cpp \
	multicore/tests/SpawnTaskTest.cpp \
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
	plotter/griddata/blockstructured/PeanoTextPatchFileWriter_CellDataWriter.cpp \
	plotter/griddata/blockstructured/PeanoTextPatchFileWriter_VertexDataWriter.cpp \
	plotter/griddata/blockstructured/PeanoHDF5FileWriter.cpp \
	plotter/griddata/blockstructured/PeanoHDF5FileWriter_CellDataWriter.cpp \
	plotter/griddata/blockstructured/PeanoHDF5FileWriter_VertexDataWriter.cpp \
	plotter/pointdata/vtk/VTKWriter.cpp \
	reader/NetCDFReader.cpp \
	reader/PeanoTextPatchFileReader.cpp \
	reader/TopologyParser.cpp \
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
