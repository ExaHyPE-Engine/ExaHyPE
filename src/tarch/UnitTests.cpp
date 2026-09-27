// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UnitTests.h"

#include "tarch/accelerator/tests/BufferTest.h"
#include "tarch/accelerator/tests/ChunkedMemoryPoolTest.h"
#include "tarch/accelerator/tests/DeviceCopiesTest.h"
#include "tarch/accelerator/tests/MemoryAllocationTest.h"
#if defined(PEANO_WITH_GPU_BACKEND_CUDA)
#include "tarch/accelerator/cuda/tests/ReductionTest.h"
#elif defined(PEANO_WITH_GPU_BACKEND_HIP)
#include "tarch/accelerator/hip/tests/ReductionTest.h"
#elif defined(PEANO_WITH_GPU_BACKEND_SYCL)
#include "tarch/accelerator/sycl/tests/ReductionTest.h"
#elif defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
#include "tarch/accelerator/kokkos/tests/ReductionTest.h"
#elif defined(PEANO_WITH_GPU_BACKEND_OMPT)
#include "tarch/accelerator/ompt/tests/ReductionTest.h"
#endif
#include "tarch/accelerator/tests/ParallelConstructsTest.h"
#include "tarch/accelerator/tests/ScratchTest.h"
#include "tarch/database/tests/GlobalTimeSeriesDatabaseTest.h"
#include "tarch/la/tests/ExactAccumulatorTest.h"
#include "tarch/la/tests/MatrixTest.h"
#include "tarch/la/tests/MatrixVectorTest.h"
#include "tarch/la/tests/ScalarTest.h"
#include "tarch/la/tests/VectorTest.h"
#include "tarch/mpi/tests/CommunicationManagerTest.h"
#include "tarch/mpi/tests/DiagnosticsTest.h"
#include "tarch/mpi/tests/HelperThreadTest.h"
#include "tarch/mpi/tests/PingPongTest.h"
#include "tarch/mpi/tests/PollPacingTest.h"
#include "tarch/mpi/tests/RankTest.h"
#include "tarch/plotter/tests/FormatTest.h"
#include "tarch/plotter/tests/HDF5FileWriterTest.h"
#include "tarch/plotter/tests/NetCDFFileWriterTest.h"
#include "tarch/plotter/tests/VTKHDFFileWriterTest.h"
#include "tarch/plotter/tests/VTUFileWriterTest.h"
#include "tarch/reader/tests/JSONReaderTest.h"
#include "tarch/reader/tests/NetCDFFieldSamplerTest.h"
#include "tarch/reader/tests/NetCDFFileReaderTest.h"
#include "tarch/tasking/tests/RuntimeTest.h"
#include "tarch/tasking/tests/TaskGraphTest.h"
#include "tarch/tests/AssertionsTest.h"
#include "tarch/tests/MemoryUsageTest.h"
#include "tarch/tests/NonCriticalAssertionsTest.h"
#include "tarch/tests/SortTest.h"
#include "tarch/tests/SpanTest.h"
#include "tarch/tests/TestCaseTest.h"
#include "tarch/tests/TreeTestCaseCollection.h"

tarch::tests::TestCase*
tarch::getUnitTests() {
  auto* const result = new tarch::tests::TreeTestCaseCollection();

  result->addTestCase(new tarch::la::tests::ExactAccumulatorTest());
  result->addTestCase(new tarch::la::tests::MatrixTest());
  result->addTestCase(new tarch::la::tests::MatrixVectorTest());
  result->addTestCase(new tarch::la::tests::ScalarTest());
  result->addTestCase(new tarch::la::tests::VectorTest());

  result->addTestCase(new tarch::tests::AssertionsTest());
  result->addTestCase(new tarch::tests::MemoryUsageTest());
  result->addTestCase(new tarch::tests::NonCriticalAssertionsTest());
  result->addTestCase(new tarch::tests::SortTest());
  result->addTestCase(new tarch::tests::SpanTest());
  result->addTestCase(new tarch::tests::TestCaseTest());

  result->addTestCase(new tarch::database::tests::GlobalTimeSeriesDatabaseTest());

  result->addTestCase(new tarch::mpi::tests::CommunicationManagerTest());
  result->addTestCase(new tarch::mpi::tests::DiagnosticsTest());
  result->addTestCase(new tarch::mpi::tests::HelperThreadTest());
  result->addTestCase(new tarch::mpi::tests::PingPongTest());
  result->addTestCase(new tarch::mpi::tests::PollPacingTest());
  result->addTestCase(new tarch::mpi::tests::RankTest());

  result->addTestCase(new tarch::accelerator::tests::BufferTest());
  result->addTestCase(new tarch::accelerator::tests::ChunkedMemoryPoolTest());
  result->addTestCase(new tarch::accelerator::tests::DeviceCopiesTest());
  result->addTestCase(new tarch::accelerator::tests::MemoryAllocationTest());
  result->addTestCase(new tarch::accelerator::tests::ParallelConstructsTest());
  result->addTestCase(new tarch::accelerator::tests::ScratchTest());

#if defined(PEANO_WITH_GPU_BACKEND_CUDA)
  result->addTestCase(new tarch::accelerator::cuda::tests::ReductionTest());
#elif defined(PEANO_WITH_GPU_BACKEND_HIP)
  result->addTestCase(new tarch::accelerator::hip::tests::ReductionTest());
#elif defined(PEANO_WITH_GPU_BACKEND_SYCL)
  result->addTestCase(new tarch::accelerator::sycl::tests::ReductionTest());
#elif defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
  result->addTestCase(new tarch::accelerator::kokkos::tests::ReductionTest());
#elif defined(PEANO_WITH_GPU_BACKEND_OMPT)
  result->addTestCase(new tarch::accelerator::ompt::tests::ReductionTest());
#endif

  result->addTestCase(new tarch::tasking::tests::RuntimeTest());
  result->addTestCase(new tarch::tasking::tests::TaskGraphTest());

  result->addTestCase(new tarch::plotter::tests::FormatTest());
  result->addTestCase(new tarch::plotter::tests::HDF5FileWriterTest());
  result->addTestCase(new tarch::plotter::tests::NetCDFFileWriterTest());
  result->addTestCase(new tarch::plotter::tests::VTKHDFFileWriterTest());
  result->addTestCase(new tarch::plotter::tests::VTUFileWriterTest());

  result->addTestCase(new tarch::reader::tests::JSONReaderTest());
  result->addTestCase(new tarch::reader::tests::NetCDFFileReaderTest());
  result->addTestCase(new tarch::reader::tests::NetCDFFieldSamplerTest());

  return result;
}
