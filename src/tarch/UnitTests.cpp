// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UnitTests.h"

#include "accelerator/tests/MemoryAllocationCacheTest.h"
#include "accelerator/tests/MemoryAllocationTest.h"
#if defined(PEANO_WITH_GPU_BACKEND_CUDA)
#include "accelerator/cuda/tests/ReductionTest.h"
#elif defined(PEANO_WITH_GPU_BACKEND_HIP)
#include "accelerator/hip/tests/ReductionTest.h"
#elif defined(PEANO_WITH_GPU_BACKEND_SYCL)
#include "accelerator/sycl/tests/ReductionTest.h"
#elif defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
#include "accelerator/kokkos/tests/ReductionTest.h"
#elif defined(PEANO_WITH_GPU_BACKEND_OMPT)
#include "accelerator/ompt/tests/ReductionTest.h"
#endif
#include "la/tests/DynamicMatrixTest.h"
#include "la/tests/GramSchmidtTest.h"
#include "la/tests/LUDecompositionTest.h"
#include "la/tests/MatrixTest.h"
#include "la/tests/MatrixVectorTest.h"
#include "la/tests/ScalarTest.h"
#include "la/tests/VectorTest.h"
#include "mpi/tests/StringTest.h"
#include "multicore/tests/SpawnTaskTest.h"
#include "plotter/tests/HDF5FileWriterTest.h"
#include "plotter/tests/NetCDFFileWriterTest.h"
#include "plotter/tests/VTUFileWriterTest.h"
#include "reader/tests/JSONReaderTest.h"
#include "reader/tests/NetCDFFieldSamplerTest.h"
#include "reader/tests/NetCDFFileReaderTest.h"
#include "tests/TreeTestCaseCollection.h"

tarch::tests::TestCase* tarch::getUnitTests() {
  tests::TreeTestCaseCollection* result = new tests::TreeTestCaseCollection();

  result->addTestCase(new la::tests::GramSchmidtTest());
  result->addTestCase(new la::tests::LUDecompositionTest());
  result->addTestCase(new la::tests::MatrixTest());
  result->addTestCase(new la::tests::MatrixVectorTest());
  result->addTestCase(new la::tests::ScalarTest());
  result->addTestCase(new la::tests::VectorTest());
  result->addTestCase(new la::tests::DynamicMatrixTest());

  result->addTestCase(new mpi::tests::StringTest());

  result->addTestCase(new accelerator::tests::MemoryAllocationCacheTest());
  result->addTestCase(new accelerator::tests::MemoryAllocationTest());
#if defined(PEANO_WITH_GPU_BACKEND_CUDA)
  result->addTestCase(new accelerator::cuda::tests::ReductionTest());
#elif defined(PEANO_WITH_GPU_BACKEND_HIP)
  result->addTestCase(new accelerator::hip::tests::ReductionTest());
#elif defined(PEANO_WITH_GPU_BACKEND_SYCL)
  result->addTestCase(new accelerator::sycl::tests::ReductionTest());
#elif defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
  result->addTestCase(new accelerator::kokkos::tests::ReductionTest());
#elif defined(PEANO_WITH_GPU_BACKEND_OMPT)
  result->addTestCase(new accelerator::ompt::tests::ReductionTest());
#endif

  result->addTestCase(new multicore::tests::SpawnTaskTest());

  result->addTestCase(new plotter::tests::HDF5FileWriterTest());
  result->addTestCase(new plotter::tests::NetCDFFileWriterTest());
  result->addTestCase(new plotter::tests::VTUFileWriterTest());
  result->addTestCase(new reader::tests::JSONReaderTest());
  result->addTestCase(new reader::tests::NetCDFFileReaderTest());
  result->addTestCase(new reader::tests::NetCDFFieldSamplerTest());

  return result;
}
