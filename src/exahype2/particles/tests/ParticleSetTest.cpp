// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParticleSetTest.h"

#include "exahype2/particles/ParticleSet.h"
#include "peano4/Constants.h"
#include "peano4/Partition.h"
#include "peano4/Spacetree.h"
#include "peano4/tests/TestGrids.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

namespace {
  TARGET_DECLARE
  constexpr int Depth             = 2;
  constexpr int NumberOfData      = 2;
  constexpr int ValuesPerParticle = DIMENSIONS + NumberOfData;
  TARGET_DECLARE_END

  using Set = exahype2::particles::ParticleSet<NumberOfData>;

  const peano4::Partition& buildGrid() {
    peano4::tests::buildRegularGrid(Depth);
    return peano4::Spacetree::getInstance().getPartition();
  }

  int getCellAt(const tarch::la::Vector<DIMENSIONS, int>& lattice) {
    return peano4::Spacetree::getInstance().getCellAt(Depth, lattice);
  }

  std::vector<double> getCandidateAt(const double first, const double second, const double firstData) {
    std::vector<double> values(ValuesPerParticle, 0.5);
    values[0] = first;
    values[1] = second;
    for (int entry = 0; entry < NumberOfData; entry++) {
      values[static_cast<std::size_t>(DIMENSIONS + entry)] = firstData + entry;
    }
    return values;
  }

  TARGET_DECLARE
  struct CheckAndOverwriteParticle final {
    exahype2::particles::ParticleSetView<NumberOfData> particles{};
    const int*                                         slots{nullptr};
    double                                             firstData{0.0};
    double                                             secondData{0.0};
    double                                             firstCoordinate{0.0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int index, int& accumulator) const {
      exahype2::particles::Particle particle = particles.getParticle(index);
      if (particles.getSlot(index) != slots[index] or particle.getData(0) != firstData) {
        accumulator++;
      }
      particle.setData(1, secondData);
      particle.setX(0, firstCoordinate);
    }
  };

  /**
   * Raises data entry one of every particle by one and moves the particle whose first data entry
   * is target to the position with every coordinate equal to coordinate. The functor marks and
   * counts every particle that it moves out of its cell.
   */
  struct MoveParticleOnDevice final {
    exahype2::particles::ParticleSetView<NumberOfData> particles{};
    peano4::Partition                                  partition{};
    double                                             target{0.0};
    double                                             coordinate{0.0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int index, int& numberOfMovedParticles) const {
      exahype2::particles::Particle particle = particles.getParticle(index);
      particle.setData(1, particle.getData(1) + 1.0);
      if (particle.getData(0) != target) {
        return;
      }
      const tarch::la::Vector<DIMENSIONS, double> position(coordinate);
      particle.setX(position);
      if (not particles.isInsideCell(partition, index, position, tarch::la::Vector<DIMENSIONS, double>(0.0))) {
        particles.markAsMovedOutOfCell(index);
        numberOfMovedParticles++;
      }
    }
  };
  TARGET_DECLARE_END

  /**
   * Returns the number of particles whose device copy has an unexpected slot or first data entry.
   * The kernel also writes secondData and firstCoordinate into every particle on the device.
   */
  int getNumberOfMismatchesOfTheDeviceCopy(
    Set&         particles,
    const double firstData,
    const double secondData,
    const double firstCoordinate
  ) {
    std::vector<int> slotsOnHost(static_cast<std::size_t>(particles.getSize()));
    for (int index = 0; index < particles.getSize(); index++) {
      slotsOnHost[static_cast<std::size_t>(index)] = particles.getSlot(index);
    }
    tarch::accelerator::Buffer<int> expectedSlots(tarch::MemoryLocation::Device);
    expectedSlots.setValues(slotsOnHost);
    particles.upload();
    const auto view       = particles.getView();
    int        mismatches = 0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(view.numberOfParticles),
      CheckAndOverwriteParticle{view, expectedSlots.getData(), firstData, secondData, firstCoordinate},
      tarch::accelerator::Sum<int>{mismatches}
    );
    return mismatches;
  }
} // namespace

exahype2::particles::tests::ParticleSetTest::ParticleSetTest() :
  tarch::tests::TestCase("exahype2::particles::tests::ParticleSetTest") {}

void
exahype2::particles::tests::ParticleSetTest::run() {
  testMethod(testInsertKeepsTheParticlesOfTheRank);
  testMethod(testAPositionOnAFaceBelongsToTheLowerCell);
  testMethod(testRelocateMovesAParticleToTheCellThatContainsIt);
  testMethod(testInsertAndRelocateCountTheParticlesOfAllRanks);
  testMethod(testUploadAndDownloadCarryTheValues);
  testMethod(testAHostWriteReachesTheDeviceCopy);
  testMethod(testRelocateKeepsTheDeviceValuesWhileNoParticleLeaves);
  testMethod(testReleaseFreesEveryArray);
  testMethod(testAParticleMarkedOnTheDeviceMovesToTheCellThatContainsIt);
}

/**
 * relocateParticlesMovedOutOfCells() puts a particle that a kernel moved and marked into the cell
 * that holds it and keeps the device values of the other particles.
 */
void
exahype2::particles::tests::ParticleSetTest::testAParticleMarkedOnTheDeviceMovesToTheCellThatContainsIt() {
  const auto&         partition = buildGrid();
  std::vector<double> candidates;
  for (const auto& candidate :
       {getCandidateAt(0.1, 0.1, 10.0), getCandidateAt(0.5, 0.5, 20.0), getCandidateAt(0.9, 0.9, 30.0)}) {
    candidates.insert(candidates.end(), candidate.begin(), candidate.end());
  }
  Set particles;
  particles.insert(partition, candidates);
  validateEquals(peano4::tests::getGlobalSumOf(particles.getSize()), 3);

  const auto moveOnTheDevice = [&](const double target) {
    particles.upload();
    const auto view                   = particles.getView();
    int        numberOfMovedParticles = 0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(view.numberOfParticles),
      MoveParticleOnDevice{view, partition.onDevice(), target, 0.95},
      tarch::accelerator::Sum<int>{numberOfMovedParticles}
    );
    particles.relocateParticlesMovedOutOfCells(partition, numberOfMovedParticles);
    return numberOfMovedParticles;
  };

  const int numberOfMovedParticles = moveOnTheDevice(20.0);
  validateEquals(peano4::tests::getGlobalSumOf(numberOfMovedParticles), 1);
  validateEquals(peano4::tests::getGlobalSumOf(particles.getSize()), 3);
  validateEquals(particles.getGlobalNumberOfParticles(), 3);
  particles.download();
  for (int index = 0; index < particles.getSize(); index++) {
    validateEqualsWithParams1(particles.getSlot(index), Set::getSlotAt(partition, particles.getPosition(index)), index);
    validateNumericalEqualsWithParams1(particles.getData(index)[1], particles.getData(index)[0] + 2.0, index);
    if (particles.getData(index)[0] == 20.0) {
      validateNumericalEquals(particles.getPosition(index)(0), 0.95);
    }
  }

  validateEquals(peano4::tests::getGlobalSumOf(moveOnTheDevice(-1.0)), 0);
  particles.download();
  for (int index = 0; index < particles.getSize(); index++) {
    validateNumericalEqualsWithParams1(particles.getData(index)[1], particles.getData(index)[0] + 3.0, index);
  }
}

/**
 * Of three candidates in the domain and two outside it, the ranks together keep three.
 */
void
exahype2::particles::tests::ParticleSetTest::testInsertKeepsTheParticlesOfTheRank() {
  const auto&         partition = buildGrid();
  std::vector<double> candidates;
  for (const auto& candidate :
       {getCandidateAt(0.1, 0.1, 10.0),
        getCandidateAt(0.5, 0.5, 20.0),
        getCandidateAt(0.9, 0.9, 30.0),
        getCandidateAt(1.5, 0.5, 40.0),
        getCandidateAt(0.5, -0.1, 50.0)}) {
    candidates.insert(candidates.end(), candidate.begin(), candidate.end());
  }

  Set particles;
  particles.insert(partition, candidates);
  validateEquals(peano4::tests::getGlobalSumOf(particles.getSize()), 3);

  const int rank = tarch::mpi::Rank::getInstance().getRank();
  for (int index = 0; index < particles.getSize(); index++) {
    validateEqualsWithParams1(particles.getSlot(index), Set::getSlotAt(partition, particles.getPosition(index)), index);
    validateEqualsWithParams1(particles.getNumber(index).first, rank, index);
    validateEqualsWithParams1(particles.getNumber(index).second, index, index);
    validateNumericalEqualsWithParams1(particles.getData(index)[1], particles.getData(index)[0] + 1.0, index);
  }
}

/**
 * A position on a face belongs to the lower cell, and a position on a domain wall to the cell at the wall.
 */
void
exahype2::particles::tests::ParticleSetTest::testAPositionOnAFaceBelongsToTheLowerCell() {
  const auto&  partition = buildGrid();
  const double width     = 1.0 / peano4::power(peano4::SubdivisionFactor, Depth);

  tarch::la::Vector<DIMENSIONS, double> onFace(0.5 * width);
  onFace(0) = 2.0 * width;
  tarch::la::Vector<DIMENSIONS, int> lowerLattice(0);
  lowerLattice(0) = 1;
  validateEquals(Set::getSlotAt(partition, onFace), getCellAt(lowerLattice));

  tarch::la::Vector<DIMENSIONS, double> onWall(0.5 * width);
  onWall(0) = 0.0;
  validateEquals(Set::getSlotAt(partition, onWall), getCellAt(tarch::la::Vector<DIMENSIONS, int>(0)));

  tarch::la::Vector<DIMENSIONS, double> onUpperWall(0.5 * width);
  onUpperWall(0) = 1.0;
  tarch::la::Vector<DIMENSIONS, int> lastLattice(0);
  lastLattice(0) = peano4::power(peano4::SubdivisionFactor, Depth) - 1;
  validateEquals(Set::getSlotAt(partition, onUpperWall), getCellAt(lastLattice));

  tarch::la::Vector<DIMENSIONS, double> outside(0.5 * width);
  outside(0) = -0.5 * width;
  validateEquals(Set::getSlotAt(partition, outside), -1);
}

void
exahype2::particles::tests::ParticleSetTest::testRelocateMovesAParticleToTheCellThatContainsIt() {
  const auto& partition = buildGrid();
  Set         particles;
  particles.insert(partition, getCandidateAt(0.05, 0.05, 1.0));
  validateEquals(peano4::tests::getGlobalSumOf(particles.getSize()), 1);

  const tarch::la::Vector<DIMENSIONS, double> target(0.95);
  for (int index = 0; index < particles.getSize(); index++) {
    particles.getParticle(index).setX(target);
  }
  particles.relocate(partition);
  validateEquals(peano4::tests::getGlobalSumOf(particles.getSize()), 1);
  for (int index = 0; index < particles.getSize(); index++) {
    validateEquals(particles.getSlot(index), Set::getSlotAt(partition, target));
    validate(particles.getSlot(index) >= 0);
    validateNumericalEquals(particles.getPosition(index)(0), 0.95);
    validateNumericalEquals(particles.getData(index)[0], 1.0);
    validateEquals(particles.getNumber(index).second, 0);
  }
}

void
exahype2::particles::tests::ParticleSetTest::testInsertAndRelocateCountTheParticlesOfAllRanks() {
  const auto&         partition = buildGrid();
  std::vector<double> candidates;
  for (const auto& candidate :
       {getCandidateAt(0.1, 0.1, 10.0), getCandidateAt(0.5, 0.5, 20.0), getCandidateAt(0.9, 0.9, 30.0)}) {
    candidates.insert(candidates.end(), candidate.begin(), candidate.end());
  }
  Set particles;
  validateEquals(particles.getGlobalNumberOfParticles(), 0);
  particles.insert(partition, candidates);
  validateEquals(particles.getGlobalNumberOfParticles(), 3);

  particles.relocate(partition);
  validateEquals(particles.getGlobalNumberOfParticles(), 3);

  const tarch::la::Vector<DIMENSIONS, double> target(0.95);
  for (int index = 0; index < particles.getSize(); index++) {
    particles.getParticle(index).setX(target);
  }
  particles.relocate(partition);
  validateEquals(particles.getGlobalNumberOfParticles(), 3);
  validateEquals(peano4::tests::getGlobalSumOf(particles.getSize()), 3);

  particles.release();
  validateEquals(particles.getGlobalNumberOfParticles(), 0);
}

void
exahype2::particles::tests::ParticleSetTest::testUploadAndDownloadCarryTheValues() {
  const auto& partition = buildGrid();
  Set         particles;
  particles.insert(partition, getCandidateAt(0.5, 0.5, 3.0));
  validateEquals(getNumberOfMismatchesOfTheDeviceCopy(particles, 3.0, 7.0, 0.25), 0);
  validateEquals(particles.getView().numberOfParticles, particles.getSize());
  particles.download();
  for (int index = 0; index < particles.getSize(); index++) {
    validateNumericalEquals(particles.getData(index)[1], 7.0);
    validateNumericalEquals(particles.getPosition(index)(0), 0.25);
  }
}

void
exahype2::particles::tests::ParticleSetTest::testAHostWriteReachesTheDeviceCopy() {
  const auto& partition = buildGrid();
  Set         particles;
  particles.insert(partition, getCandidateAt(0.5, 0.5, 3.0));
  particles.upload();
  for (int index = 0; index < particles.getSize(); index++) {
    particles.getParticle(index).setData(0, 5.0);
  }
  validateEquals(getNumberOfMismatchesOfTheDeviceCopy(particles, 5.0, 7.0, 0.5), 0);
}

void
exahype2::particles::tests::ParticleSetTest::testRelocateKeepsTheDeviceValuesWhileNoParticleLeaves() {
  const auto& partition = buildGrid();
  Set         particles;
  particles.insert(partition, getCandidateAt(0.5, 0.5, 3.0));
  particles.relocate(partition);
  validateEquals(getNumberOfMismatchesOfTheDeviceCopy(particles, 3.0, 7.0, 0.5), 0);
  particles.relocate(partition);
  validateEquals(particles.getGlobalNumberOfParticles(), 1);
  particles.upload();
  particles.download();
  for (int index = 0; index < particles.getSize(); index++) {
    validateNumericalEquals(particles.getData(index)[0], 3.0);
    validateNumericalEquals(particles.getData(index)[1], 7.0);
  }
}

void
exahype2::particles::tests::ParticleSetTest::testReleaseFreesEveryArray() {
  const auto& partition = buildGrid();
  Set         particles;
  validateEquals(particles.getAllocatedBytes().getTotal(), std::size_t{0});
  particles.insert(partition, getCandidateAt(0.5, 0.5, 3.0));
  particles.upload();
  if (particles.getSize() > 0) {
    validate(particles.getAllocatedBytes().getTotal() > 0);
  }
  particles.release();
  validateEquals(particles.getSize(), 0);
  validateEquals(particles.getAllocatedBytes().getTotal(), std::size_t{0});
}
