# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

file(GLOB_RECURSE HEADERS ${HEADER_EXTENSIONS_TO_GLOB})

set(SOURCES
  Init.cpp
  MemoryUsage.cpp
  CompilerSpecificSettings.cpp
  StackSizeLimitDetection.cpp
  Assertions.cpp
  CPUFeatureDetection.cpp
  FloatingPointExceptions.cpp
  NonCriticalAssertions.cpp
  UnitTests.cpp
  tests/TestCase.cpp
  tests/TreeTestCaseCollection.cpp

  accelerator/accelerator.cpp
  accelerator/Device.cpp
  accelerator/MemoryAllocationCache.cpp
  accelerator/MemoryBlocks.cpp
  accelerator/MemoryLocation.cpp
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

  la/DynamicMatrix.cpp
  la/GramSchmidt.cpp
  la/LUDecomposition.cpp
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
  la/tests/DynamicMatrixTest.cpp
  la/tests/GramSchmidtTest.cpp
  la/tests/LUDecompositionTest.cpp
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
  instrumentation/Trace.cpp
  instrumentation/Warning.cpp

  mpi/mpi.cpp
  mpi/DoubleMessage.cpp
  mpi/IntegerMessage.cpp
  mpi/Rank.cpp
  mpi/StringMessage.cpp
  mpi/tests/StringTest.cpp

  multicore/multicore.cpp
  multicore/BooleanSemaphore.cpp
  multicore/Core.cpp
  multicore/Lock.cpp
  multicore/MultiReadSingleWriteLock.cpp
  multicore/MultiReadSingleWriteSemaphore.cpp
  multicore/RecursiveLock.cpp
  multicore/RecursiveSemaphore.cpp
  multicore/Task.cpp
  multicore/tests/SpawnTaskTest.cpp
  multicore/omp/multicore.cpp
  multicore/omp/BooleanSemaphore.cpp
  multicore/omp/Core.cpp
  multicore/omp/RecursiveSemaphore.cpp

  plotter/AMRPlotter.cpp
  plotter/CellDataPlotter.cpp
  plotter/CellPlotter.cpp
  plotter/FileWriter.cpp
  plotter/HDF5FileWriter.cpp
  plotter/JSONTimeSeriesWriter.cpp
  plotter/NetCDFFileWriter.cpp
  plotter/PVDTimeSeriesWriter.cpp
  plotter/TimeSeriesWriter.cpp
  plotter/VertexDataPlotter.cpp
  plotter/VertexPlotter.cpp
  plotter/VTUFileWriter.cpp
  plotter/XDMFTimeSeriesWriter.cpp
  plotter/tests/HDF5FileWriterTest.cpp
  plotter/tests/NetCDFFileWriterTest.cpp
  plotter/tests/VTUFileWriterTest.cpp

  reader/CSVReader.cpp
  reader/JSONReader.cpp
  reader/NetCDFFieldSampler.cpp
  reader/NetCDFFileReader.cpp
  reader/tests/NetCDFFieldSamplerTest.cpp
  reader/tests/NetCDFFileReaderTest.cpp
  reader/tests/JSONReaderTest.cpp

  services/Service.cpp
  services/ServiceRepository.cpp

  instrumentation/Measurement.cpp
  instrumentation/Watch.cpp
)

if(_peano_gpu_backend STREQUAL "cuda")
  list(APPEND SOURCES accelerator/cuda/tests/ReductionTest.cpp)
  set_source_files_properties(accelerator/cuda/tests/ReductionTest.cpp PROPERTIES LANGUAGE CUDA)
  set_source_files_properties(accelerator/tests/ParallelConstructsTest.cpp PROPERTIES LANGUAGE CUDA)
elseif(_peano_gpu_backend STREQUAL "hip")
  list(APPEND SOURCES accelerator/hip/tests/ReductionTest.cpp)
  set_source_files_properties(accelerator/hip/tests/ReductionTest.cpp PROPERTIES LANGUAGE HIP)
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

set(TARCH_LINK_LIBRARIES)
if(PEANO_WITH_FPE AND CMAKE_DL_LIBS)
  list(APPEND TARCH_LINK_LIBRARIES ${CMAKE_DL_LIBS})
endif()

foreach(mode IN LISTS SUPPORTED_BUILD_TYPES)
  peano_add_target(
    TYPE STATIC_LIBRARY
    NAME Tarch${mode}
    MODE ${mode}
    SOURCES ${SOURCES}
    HEADERS ${HEADERS}
    LINK_LIBRARIES ${TARCH_LINK_LIBRARIES}
  )
endforeach()
