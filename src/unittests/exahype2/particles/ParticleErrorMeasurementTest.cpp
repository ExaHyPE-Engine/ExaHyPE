// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/particles/ParticleErrorMeasurement.h"

#include "exahype2/ErrorMeasurement.h"
#include "exahype2/particles/ParticleSet.h"
#include "peano4/Spacetree.h"
#include "tarch/mpi/Rank.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/peano4/TestGrids.h"

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
   * Deletes the time series file that only the global master writes.
   */
  void deleteTimeSeriesFile(const std::string& fileName) {
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      std::filesystem::remove(fileName + ".csv");
    }
  }
} // namespace

UNIT_TEST("The norms sum the position errors of every particle of every rank") {
  GIVEN("Three particles on the diagonal of a regular grid over every rank and an open measurement") {
    peano4::unittests::buildRegularGrid(Depth);
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
    const auto globalNumberOfParticles = static_cast<double>(peano4::unittests::getGlobalSumOf(particles.getSize()));

    exahype2::ErrorMeasurement measurement;
    const std::string fileName = (tarch::testing::getWritableTestDirectory() / "ParticleErrorMeasurementTest").string();
    measurement.open(fileName, 0.0);

    WHEN("The measurement records the position errors against the exact position half a unit along the first axis") {
      const auto norms = exahype2::particles::measurePositionErrors<ShiftedSolver>(particles, 0.25, 0.1, measurement);

      THEN("The L1 norm adds one half and the squared L2 norm one quarter per particle, the Linf norm is one half") {
        VALIDATE_NUMERICAL_EQUALS(norms[0], 0.5 * globalNumberOfParticles);
        VALIDATE_NUMERICAL_EQUALS(norms[1], std::sqrt(0.25 * globalNumberOfParticles));
        VALIDATE_NUMERICAL_EQUALS(norms[2], 0.5);
      }
    }
    deleteTimeSeriesFile(fileName);
  }
}
