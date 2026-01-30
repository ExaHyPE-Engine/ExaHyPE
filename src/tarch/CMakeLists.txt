# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

file(GLOB_RECURSE HEADERS ${HEADER_EXTENSIONS_TO_GLOB})

set(SOURCES
  tarch.cpp
  CPUFeatures.cpp
  Enumerator.cpp
  NonCriticalAssertions.cpp
  UnitTests.cpp
  tests/TestCase.cpp
  tests/TreeTestCaseCollection.cpp

  accelerator/accelerator.cpp
  accelerator/Device.cpp
  accelerator/MemoryBlocks.cpp
  accelerator/MemoryLocation.cpp
  accelerator/MemoryManager.cpp
  accelerator/tests/MemoryManagerTest.cpp
  accelerator/cuda/accelerator.cpp
  accelerator/cuda/Device.cpp
  accelerator/cuda/DevicePropertiesCache.cpp
  accelerator/cuda/ErrorCheck.cpp
  accelerator/cuda/ExecutionPolicy.cpp
  accelerator/kokkos/accelerator.cpp
  accelerator/kokkos/Device.cpp
  accelerator/ompt/accelerator.cpp
  accelerator/ompt/Device.cpp
  accelerator/sycl/accelerator.cpp
  accelerator/sycl/Device.cpp

  la/DynamicMatrix.cpp
  la/DynamicMatrixOperations.cpp
  la/GramSchmidt.cpp
  la/LUDecomposition.cpp
  la/LUDecompositionDynamic.cpp
  la/Matrix.cpp
  la/MatrixMatrixOperations.cpp
  la/MatrixOperations.cpp
  la/MatrixScalarOperations.cpp
  la/MatrixVectorOperations.cpp
  la/Scalar.cpp
  la/ScalarOperations.cpp
  la/Vector.cpp
  la/VectorOperations.cpp
  la/VectorScalarOperations.cpp
  la/VectorSlice.cpp
  la/VectorVectorOperations.cpp
  la/tests/DynamicMatrixTest.cpp
  la/tests/GramSchmidtTest.cpp
  la/tests/LUDecompositionTest.cpp
  la/tests/MatrixTest.cpp
  la/tests/MatrixVectorTest.cpp
  la/tests/ScalarTest.cpp
  la/tests/VectorTest.cpp

  logging/ChromeTraceFileLogger.cpp
  logging/CommandLineLogger.cpp
  logging/ITACLogger.cpp
  logging/ITTLogger.cpp
  logging/Log.cpp
  logging/LogFilter.cpp
  logging/LogFilterFileReader.cpp
  logging/NVTXLogger.cpp
  logging/ScorePLogger.cpp
  logging/Statistics.cpp

  mpi/mpi.cpp
  mpi/BooleanSemaphore.cpp
  mpi/DoubleMessage.cpp
  mpi/IntegerMessage.cpp
  mpi/Lock.cpp
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
  reader/NetCDFFieldParser.cpp
  reader/NetCDFFileReader.cpp

  services/Service.cpp
  services/ServiceRepository.cpp

  timing/GlidingAverageMeasurement.cpp
  timing/Measurement.cpp
  timing/Watch.cpp
)

install_peano_headers(
  PATTERNS ${HEADER_EXTENSIONS_TO_GLOB}
)

foreach(mode IN LISTS SUPPORTED_BUILD_TYPES)
  add_peano_library(
    NAME Tarch${mode}
    MODE ${mode}
    SOURCES ${SOURCES}
    HEADERS ${HEADERS}
  )
endforeach()
