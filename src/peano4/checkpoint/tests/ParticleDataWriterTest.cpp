// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParticleDataWriterTest.h"

#ifdef PEANO_WITH_HDF5
#include "peano4/checkpoint/ParticleDataWriter.h"
#endif

peano4::checkpoint::tests::ParticleDataWriterTest::ParticleDataWriterTest() :
  TestCase("peano4::checkpoint::tests::ParticleDataWriterTest") {}

void peano4::checkpoint::tests::ParticleDataWriterTest::testSortParticleRows() {
#ifdef PEANO_WITH_HDF5
  // Three rows of [number0, number1, payload], deliberately unsorted.
  std::vector<double> rows{1.0, 2.0, 30.0, 0.0, 5.0, 10.0, 1.0, 0.0, 20.0};
  peano4::checkpoint::sortParticleRows(rows, 3);
  const std::vector<double> sorted{0.0, 5.0, 10.0, 1.0, 0.0, 20.0, 1.0, 2.0, 30.0};
  validateEquals(rows.size(), sorted.size());
  for (std::size_t entry = 0; entry < sorted.size(); entry++) {
    validateNumericalEqualsWithParams1(rows[entry], sorted[entry], entry);
  }
#endif
}

void peano4::checkpoint::tests::ParticleDataWriterTest::run() { testMethod(testSortParticleRows); }
