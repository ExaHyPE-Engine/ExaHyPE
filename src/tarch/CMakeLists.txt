# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

file(GLOB_RECURSE HEADERS ${HEADER_EXTENSIONS_TO_GLOB})

set(SOURCES
  StdAfx.cpp
  Init.cpp
  MemoryUsage.cpp
  CompilerSpecificSettings.cpp
  StackSizeLimitDetection.cpp
  Assertions.cpp
  Termination.cpp
  CPUFeatureDetection.cpp
  FloatingPointExceptions.cpp
  NonCriticalAssertions.cpp
  UnitTests.cpp
  tests/TestCase.cpp
  tests/TreeTestCaseCollection.cpp
  tests/NonCriticalAssertionsTest.cpp

  database/SnapshotDatabase.cpp
  database/GlobalTimeSeriesDatabase.cpp
  database/tests/GlobalTimeSeriesDatabaseTest.cpp

  accelerator/Atomic.cpp
  accelerator/accelerator.cpp
  accelerator/Atomics.cpp
  accelerator/Device.cpp
  accelerator/MemoryAllocationCache.cpp
  accelerator/MemoryBlocks.cpp
  accelerator/MemoryLocation.cpp
  accelerator/tests/AtomicTest.cpp
  accelerator/tests/AtomicsTest.cpp
  accelerator/tests/MemoryAllocationCacheTest.cpp
  accelerator/tests/MemoryAllocationTest.cpp
  accelerator/tests/ParallelConstructsTest.cpp
  accelerator/cuda/accelerator.cpp
  accelerator/cuda/Device.cpp
  accelerator/cuda/DevicePropertiesCache.cpp
  accelerator/cuda/ErrorCheck.cpp
  accelerator/cuda/ExecutionPolicy.cpp
  accelerator/hip/accelerator.cpp
  accelerator/hip/Device.cpp
  accelerator/hip/DevicePropertiesCache.cpp
  accelerator/hip/ErrorCheck.cpp
  accelerator/hip/ExecutionPolicy.cpp
  accelerator/kokkos/accelerator.cpp
  accelerator/kokkos/Device.cpp
  accelerator/ompt/accelerator.cpp
  accelerator/ompt/Device.cpp
  accelerator/sycl/accelerator.cpp
  accelerator/sycl/Device.cpp

  la/ExactAccumulator.cpp
  la/Matrix.cpp
  la/Matrix2x2.cpp
  la/Matrix3x3.cpp
  la/Matrix4x4.cpp
  la/MatrixMatrixOperations.cpp
  la/MatrixOperations.cpp
  la/MatrixScalarOperations.cpp
  la/MatrixVectorOperations.cpp
  la/Scalar.cpp
  la/ScalarOperations.cpp
  la/Vector.cpp
  la/Vector2.cpp
  la/Vector3.cpp
  la/Vector4.cpp
  la/VectorCompare.cpp
  la/VectorOperations.cpp
  la/VectorScalarOperations.cpp
  la/VectorVectorOperations.cpp
  la/tests/ExactAccumulatorTest.cpp
  la/tests/MatrixTest.cpp
  la/tests/MatrixVectorTest.cpp
  la/tests/ScalarTest.cpp
  la/tests/VectorTest.cpp

  instrumentation/CommandLineSink.cpp
  instrumentation/Debug.cpp
  instrumentation/Error.cpp
  instrumentation/FileSink.cpp
  instrumentation/Info.cpp
  instrumentation/Log.cpp
  instrumentation/Warning.cpp

  mpi/AbstractCommunicationManager.cpp
  mpi/Collectives.cpp
  mpi/Diagnostics.cpp
  mpi/HelperThread.cpp
  mpi/PersistentChannel.cpp
  mpi/Rank.cpp
  mpi/Request.cpp
  mpi/RequestGroup.cpp
  mpi/SerialCommunicationManager.cpp
  mpi/ThreadedCommunicationManager.cpp
  mpi/tests/CommunicationManagerTest.cpp
  mpi/tests/DiagnosticsTest.cpp
  mpi/tests/HelperThreadTest.cpp
  mpi/tests/PersistentChannelTest.cpp
  mpi/tests/PingPongTest.cpp
  mpi/tests/PollPacingTest.cpp
  mpi/tests/RequestGroupTest.cpp

  tasking/Runtime.cpp
  tasking/FunctorTask.cpp
  tasking/TaskGraph.cpp
  tasking/tests/RuntimeTest.cpp
  tasking/tests/TaskGraphTest.cpp

  plotter/CellDataPlotter.cpp
  plotter/CellPlotter.cpp
  plotter/CellTypes.cpp
  plotter/FileWriter.cpp
  plotter/HDF5FileWriter.cpp
  plotter/JSONTimeSeriesWriter.cpp
  plotter/NetCDFFileWriter.cpp
  plotter/PVDTimeSeriesWriter.cpp
  plotter/TimeSeriesWriter.cpp
  plotter/VertexDataPlotter.cpp
  plotter/VertexPlotter.cpp
  plotter/VTKHDFFileWriter.cpp
  plotter/VTKHDFTimeSeriesWriter.cpp
  plotter/VTKHDFFileWriter.cpp
  plotter/VTKHDFTimeSeriesWriter.cpp
  plotter/VTUFileWriter.cpp
  plotter/XDMFTimeSeriesWriter.cpp
  plotter/tests/HDF5FileWriterTest.cpp
  plotter/tests/NetCDFFileWriterTest.cpp
  plotter/tests/VTKHDFFileWriterTest.cpp
  plotter/tests/VTUFileWriterTest.cpp

  reader/CSVReader.cpp
  reader/JSONReader.cpp
  reader/NetCDFFieldSampler.cpp
  reader/NetCDFFileReader.cpp
  reader/VTKSampler.cpp
  reader/VTKSamplerVTKHDF.cpp
  reader/VTKSamplerVTU.cpp
  reader/tests/NetCDFFieldSamplerTest.cpp
  reader/tests/NetCDFFileReaderTest.cpp
  reader/tests/JSONReaderTest.cpp
  reader/tests/VTKSamplerTest.cpp

  instrumentation/Measurement.cpp
  instrumentation/Watch.cpp
)

if(_peano_gpu_backend STREQUAL "cuda")
  list(APPEND SOURCES accelerator/cuda/tests/ReductionTest.cpp)
  set_source_files_properties(accelerator/cuda/tests/ReductionTest.cpp PROPERTIES LANGUAGE CUDA)
  set_source_files_properties(accelerator/tests/AtomicsTest.cpp PROPERTIES LANGUAGE CUDA)
  set_source_files_properties(accelerator/tests/ParallelConstructsTest.cpp PROPERTIES LANGUAGE CUDA)
elseif(_peano_gpu_backend STREQUAL "hip")
  list(APPEND SOURCES accelerator/hip/tests/ReductionTest.cpp)
  set_source_files_properties(accelerator/hip/tests/ReductionTest.cpp PROPERTIES LANGUAGE HIP)
  set_source_files_properties(accelerator/tests/AtomicsTest.cpp PROPERTIES LANGUAGE HIP)
  set_source_files_properties(accelerator/tests/ParallelConstructsTest.cpp PROPERTIES LANGUAGE HIP)
elseif(_peano_gpu_backend STREQUAL "sycl")
  list(APPEND SOURCES accelerator/sycl/tests/ReductionTest.cpp)
elseif(_peano_gpu_backend STREQUAL "kokkos")
  list(APPEND SOURCES accelerator/kokkos/tests/ReductionTest.cpp)
elseif(_peano_gpu_backend STREQUAL "ompt")
  list(APPEND SOURCES accelerator/ompt/tests/ReductionTest.cpp)
endif()

install_peano_headers(
  PATTERNS ${HEADER_EXTENSIONS_TO_GLOB}
)

set(TARCH_COMPILE_DEFINITIONS)
if(
  PEANO_WITH_FPE
  AND CMAKE_SYSTEM_NAME STREQUAL "Linux"
  AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|i.86)$"
)
  find_program(PEANO_FPE_SYMBOLISER NAMES llvm-addr2line addr2line REQUIRED)
  list(APPEND TARCH_COMPILE_DEFINITIONS PEANO_FPE_SYMBOLISER="${PEANO_FPE_SYMBOLISER}")
endif()

foreach(mode IN LISTS SUPPORTED_BUILD_TYPES)
  peano_add_target(
    TYPE STATIC_LIBRARY
    NAME Tarch${mode}
    MODE ${mode}
    SOURCES ${SOURCES}
    HEADERS ${HEADERS}
    COMPILE_DEFINITIONS ${TARCH_COMPILE_DEFINITIONS}
  )
endforeach()
