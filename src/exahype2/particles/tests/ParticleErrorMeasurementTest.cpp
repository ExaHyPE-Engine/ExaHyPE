// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParticleErrorMeasurementTest.h"

#include "exahype2/ErrorMeasurement.h"
#include "exahype2/particles/ParticleErrorMeasurement.h"
#include "exahype2/particles/ParticleSet.h"
#include "peano4/Spacetree.h"
#include "peano4/tests/TestGrids.h"
#include "tarch/mpi/Rank.h"

namespace {
  constexpr int Depth        = 1;
  constexpr int NumberOfData = 2;

  struct ShiftedSolver final {
    static void analyticalSolution(
      const exahype2::particles::Particle* const particle,
      [[maybe_unused]] const double              t,
      [[maybe_unused]] const double              dt,
      double* const                              solution
    ) {
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        solution[axis] = particle->getX(axis);
      }
      solution[0] += 0.5;
      for (int entry = 0; entry < NumberOfData; entry++) {
        solution[DIMENSIONS + entry] = particle->getData(entry);
      }
    }
  };

  /**
   * Deletes the time series file. Only the global master writes this file.
   */
  void deleteTimeSeriesFile(const std::string& fileName) {
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      std::filesystem::remove(fileName + ".csv");
    }
  }
} // namespace

exahype2::particles::tests::ParticleErrorMeasurementTest::ParticleErrorMeasurementTest() :
  tarch::tests::TestCase("exahype2::particles::tests::ParticleErrorMeasurementTest") {}

void
exahype2::particles::tests::ParticleErrorMeasurementTest::run() {
  testMethod(testTheNormsSumThePositionErrorsOfEveryRank);
}

void
exahype2::particles::tests::ParticleErrorMeasurementTest::testTheNormsSumThePositionErrorsOfEveryRank() {
  peano4::tests::buildRegularGrid(Depth);
  const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();

  exahype2::particles::ParticleSet<NumberOfData> particles;
  std::vector<double>                            candidates;
  for (const double coordinate : {0.2, 0.5, 0.8}) {
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      candidates.push_back(coordinate);
    }
    candidates.push_back(1.0);
    candidates.push_back(2.0);
  }
  particles.insert(cellConnectivity, candidates);
  const auto globalNumberOfParticles = static_cast<double>(peano4::tests::getGlobalSumOf(particles.getSize()));

  exahype2::ErrorMeasurement measurement;
  const std::string fileName = (tarch::tests::getWritableTestDirectory() / "ParticleErrorMeasurementTest").string();
  measurement.open(fileName, 0.0);
  const auto norms = exahype2::particles::measurePositionErrors<ShiftedSolver>(particles, 0.25, 0.1, measurement);
  validateNumericalEquals(norms[0], 0.5 * globalNumberOfParticles);
  validateNumericalEquals(norms[1], std::sqrt(0.25 * globalNumberOfParticles));
  validateNumericalEquals(norms[2], 0.5);
  deleteTimeSeriesFile(fileName);
}
