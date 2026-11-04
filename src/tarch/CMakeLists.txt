# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

file(GLOB_RECURSE HEADERS ${HEADER_EXTENSIONS_TO_GLOB})

set(SOURCES
  Init.cpp
  MemoryUsage.cpp
  CompilerSpecificSettings.cpp
  StackSizeLimitDetection.cpp
  Assertions.cpp
  Termination.cpp
  CPUFeatureDetection.cpp
  FloatingPointExceptions.cpp
  NonCriticalAssertions.cpp

  database/SnapshotDatabase.cpp
  database/GlobalTimeSeriesDatabase.cpp

  accelerator/accelerator.cpp
  accelerator/Device.cpp
  accelerator/MemoryBlocks.cpp
  accelerator/MemoryLocation.cpp
  accelerator/Scratch.cpp
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
  mpi/Handles.cpp
  mpi/HelperThread.cpp
  mpi/Rank.cpp
  mpi/ReductionRecord.cpp
  mpi/Request.cpp
  mpi/SerialCommunicationManager.cpp
  mpi/ThreadedCommunicationManager.cpp

  tasking/Runtime.cpp
  tasking/FunctorTask.cpp
  tasking/TaskGraph.cpp

  plotter/FileWriter.cpp
  plotter/Format.cpp
  plotter/HDF5FileWriter.cpp
  plotter/NetCDFFileWriter.cpp
  plotter/VTKHDFFileWriter.cpp
  plotter/VTUFileWriter.cpp

  reader/CSVReader.cpp
  reader/JSONReader.cpp
  reader/NetCDFFieldSampler.cpp
  reader/NetCDFFileReader.cpp

  instrumentation/Measurement.cpp
  instrumentation/Watch.cpp

  testing/UnitTest.cpp
)

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
