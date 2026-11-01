// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/particles/ParticleSet.h"

#include "peano4/CellConnectivity.h"
#include "peano4/Constants.h"
#include "peano4/Spacetree.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Scalar.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/peano4/TestGrids.h"

namespace {
  TARGET_DECLARE
  constexpr int Depth             = 2;
  constexpr int NumberOfData      = 2;
  constexpr int ValuesPerParticle = DIMENSIONS + NumberOfData;
  TARGET_DECLARE_END

  using Set = exahype2::particles::ParticleSet<NumberOfData>;

  const peano4::CellConnectivity& buildGrid() {
    peano4::unittests::buildRegularGrid(Depth);
    return peano4::Spacetree::getInstance().getCellConnectivity();
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
    peano4::CellConnectivity                           cellConnectivity{};
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
      if (not particles.isInsideCell(cellConnectivity, index, position, tarch::la::Vector<DIMENSIONS, double>(0.0))) {
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

UNIT_TEST("An insert keeps every candidate inside the domain on the rank whose cell contains its position") {
  GIVEN("A regular grid over every rank, five candidates of which three lie inside the domain and an empty set") {
    const auto&         cellConnectivity = buildGrid();
    std::vector<double> candidates;
    for (const auto& candidate :
         {getCandidateAt(0.1, 0.1, 10.0),
          getCandidateAt(0.5, 0.5, 20.0),
          getCandidateAt(0.9, 0.9, 30.0),
          getCandidateAt(1.5, 0.5, 40.0),
          getCandidateAt(0.5, -0.1, 50.0)}) {
      candidates.insert(candidates.end(), candidate.begin(), candidate.end());
    }
    Set       particles;
    const int rank = tarch::mpi::Rank::getInstance().getRank();

    WHEN("The set inserts the candidates") {
      particles.insert(cellConnectivity, candidates);
      const int globalNumberOfParticles = peano4::unittests::getGlobalSumOf(particles.getSize());

      THEN("The ranks together keep three particles, each with the slot of its position, its number and its data") {
        VALIDATE_EQUALS(globalNumberOfParticles, 3);
        for (int index = 0; index < particles.getSize(); index++) {
          VALIDATE_EQUALS(
            particles.getSlot(index),
            Set::getSlotAt(cellConnectivity, particles.getPosition(index)),
            index
          );
          VALIDATE_EQUALS(particles.getNumber(index).first, rank, index);
          VALIDATE_EQUALS(particles.getNumber(index).second, index);
          VALIDATE_NUMERICAL_EQUALS(particles.getData(index)[1], particles.getData(index)[0] + 1.0, index);
        }
      }
    }
  }
}

UNIT_TEST(
  "A coordinate on or beside a face belongs to the neighbour of the face towards the lattice centre and a coordinate "
  "on the face through the centre to the cell below that face"
) {
  GIVEN("Lattices of three and of four cells of width 0.25 and shifts of half the numerical zero difference") {
    constexpr double        Width       = 0.25;
    constexpr double        Band        = 0.5 * tarch::la::NUMERICAL_ZERO_DIFFERENCE;
    static constexpr int    Lattices[2] = {3, 4};
    static constexpr double Shifts[3]   = {-Band, 0.0, Band};

    WHEN("Every face coordinate, its shifts, its mirror and every cell centre map to a lattice index") {
      struct Lookup final {
        int    index{0};
        int    expected{0};
        int    cellsPerAxis{0};
        double coordinate{0.0};
      };
      std::vector<Lookup> lookups;
      for (const int cellsPerAxis : Lattices) {
        for (int face = 0; face <= cellsPerAxis; face++) {
          const int cellTowardsTheCentre = 2 * face < cellsPerAxis ? face : face - 1;
          for (const double shift : Shifts) {
            const double coordinate = face * Width + shift;
            lookups.push_back(
              {exahype2::particles::getLatticeIndexOf(coordinate, 0.0, Width, cellsPerAxis),
               cellTowardsTheCentre,
               cellsPerAxis,
               coordinate}
            );
          }
          if (2 * face != cellsPerAxis) {
            const double mirror = (cellsPerAxis - face) * Width;
            lookups.push_back(
              {exahype2::particles::getLatticeIndexOf(mirror, 0.0, Width, cellsPerAxis),
               cellsPerAxis - 1 - cellTowardsTheCentre,
               cellsPerAxis,
               mirror}
            );
          }
        }
        for (int cell = 0; cell < cellsPerAxis; cell++) {
          const double centre = (cell + 0.5) * Width;
          lookups.push_back(
            {exahype2::particles::getLatticeIndexOf(centre, 0.0, Width, cellsPerAxis), cell, cellsPerAxis, centre}
          );
        }
      }

      THEN("Faces map to the cell towards the centre, mirrors to the mirrored cell and cell centres to their cell") {
        for (const Lookup& lookup : lookups) {
          VALIDATE_EQUALS(lookup.index, lookup.expected, lookup.cellsPerAxis, lookup.coordinate);
        }
      }
    }
  }
}

UNIT_TEST("A position on a face lies in the cell towards the domain centre, a position on a domain wall in its cell") {
  GIVEN("A regular grid over every rank and positions on two inner faces, on both walls and outside the domain") {
    const auto&                           cellConnectivity = buildGrid();
    const int                             cellsPerAxis     = peano4::power(peano4::SubdivisionFactor, Depth);
    const double                          width            = 1.0 / cellsPerAxis;
    tarch::la::Vector<DIMENSIONS, double> onLowerFace(0.5 * width);
    onLowerFace(0) = width;
    tarch::la::Vector<DIMENSIONS, double> onUpperFace(0.5 * width);
    onUpperFace(0) = (cellsPerAxis - 1) * width;
    tarch::la::Vector<DIMENSIONS, double> onWall(0.5 * width);
    onWall(0) = 0.0;
    tarch::la::Vector<DIMENSIONS, double> onUpperWall(0.5 * width);
    onUpperWall(0) = 1.0;
    tarch::la::Vector<DIMENSIONS, double> outside(0.5 * width);
    outside(0) = -0.5 * width;
    tarch::la::Vector<DIMENSIONS, int> secondLattice(0);
    secondLattice(0) = 1;
    tarch::la::Vector<DIMENSIONS, int> secondLastLattice(0);
    secondLastLattice(0) = cellsPerAxis - 2;
    tarch::la::Vector<DIMENSIONS, int> lastLattice(0);
    lastLattice(0) = cellsPerAxis - 1;

    WHEN("The set looks up the slot of every position") {
      const int slotOnLowerFace = Set::getSlotAt(cellConnectivity, onLowerFace);
      const int slotOnUpperFace = Set::getSlotAt(cellConnectivity, onUpperFace);
      const int slotOnWall      = Set::getSlotAt(cellConnectivity, onWall);
      const int slotOnUpperWall = Set::getSlotAt(cellConnectivity, onUpperWall);
      const int slotOutside     = Set::getSlotAt(cellConnectivity, outside);

      THEN("Faces map to the cells towards the centre, walls to the wall cells and the outside position to -1") {
        VALIDATE_EQUALS(slotOnLowerFace, getCellAt(secondLattice));
        VALIDATE_EQUALS(slotOnUpperFace, getCellAt(secondLastLattice));
        VALIDATE_EQUALS(slotOnWall, getCellAt(tarch::la::Vector<DIMENSIONS, int>(0)));
        VALIDATE_EQUALS(slotOnUpperWall, getCellAt(lastLattice));
        VALIDATE_EQUALS(slotOutside, -1);
      }
    }
  }
}

UNIT_TEST("A relocation moves a particle into the cell that contains its new position, on its rank or another one") {
  GIVEN("A particle inserted near the lower corner of a regular grid over every rank and a target near the upper one") {
    const auto&                                 cellConnectivity = buildGrid();
    const tarch::la::Vector<DIMENSIONS, double> target(0.95);
    Set                                         particles;
    particles.insert(cellConnectivity, getCandidateAt(0.05, 0.05, 1.0));
    const int globalNumberOfInsertedParticles = peano4::unittests::getGlobalSumOf(particles.getSize());

    WHEN("The particle moves to the target and the set relocates it") {
      for (int index = 0; index < particles.getSize(); index++) {
        particles.getParticle(index).setX(target);
      }
      particles.relocate(cellConnectivity);
      const int globalNumberOfParticles = peano4::unittests::getGlobalSumOf(particles.getSize());

      THEN("The ranks together keep one particle with the slot of the target, its data and its number") {
        VALIDATE_EQUALS(globalNumberOfInsertedParticles, 1);
        VALIDATE_EQUALS(globalNumberOfParticles, 1);
        for (int index = 0; index < particles.getSize(); index++) {
          VALIDATE_EQUALS(particles.getSlot(index), Set::getSlotAt(cellConnectivity, target));
          VALIDATE(particles.getSlot(index) >= 0);
          VALIDATE_NUMERICAL_EQUALS(particles.getPosition(index)(0), 0.95);
          VALIDATE_NUMERICAL_EQUALS(particles.getData(index)[0], 1.0);
          VALIDATE_EQUALS(particles.getNumber(index).second, 0);
        }
      }
    }
  }
}

UNIT_TEST("Every rank counts the particles of all ranks after an insert or a relocation and zero after a release") {
  GIVEN("A regular grid over every rank, an empty set, three candidates and a target near the upper domain corner") {
    const auto&         cellConnectivity = buildGrid();
    std::vector<double> candidates;
    for (const auto& candidate :
         {getCandidateAt(0.1, 0.1, 10.0), getCandidateAt(0.5, 0.5, 20.0), getCandidateAt(0.9, 0.9, 30.0)}) {
      candidates.insert(candidates.end(), candidate.begin(), candidate.end());
    }
    Set                                         particles;
    const int                                   countOfTheEmptySet = particles.getGlobalNumberOfParticles();
    const tarch::la::Vector<DIMENSIONS, double> target(0.95);

    WHEN("The set inserts the candidates, relocates them, moves them to the target, relocates them and releases them") {
      particles.insert(cellConnectivity, candidates);
      const int countAfterTheInsert = particles.getGlobalNumberOfParticles();
      particles.relocate(cellConnectivity);
      const int countAfterTheRelocation = particles.getGlobalNumberOfParticles();
      for (int index = 0; index < particles.getSize(); index++) {
        particles.getParticle(index).setX(target);
      }
      particles.relocate(cellConnectivity);
      const int countAfterTheMove = particles.getGlobalNumberOfParticles();
      const int sumAfterTheMove   = peano4::unittests::getGlobalSumOf(particles.getSize());
      particles.release();
      const int countAfterTheRelease = particles.getGlobalNumberOfParticles();

      THEN("The empty set counts zero and the set counts three from the insert to the release and zero after it") {
        VALIDATE_EQUALS(countOfTheEmptySet, 0);
        VALIDATE_EQUALS(countAfterTheInsert, 3);
        VALIDATE_EQUALS(countAfterTheRelocation, 3);
        VALIDATE_EQUALS(countAfterTheMove, 3);
        VALIDATE_EQUALS(sumAfterTheMove, 3);
        VALIDATE_EQUALS(countAfterTheRelease, 0);
      }
    }
  }
}

UNIT_TEST("An upload copies the values to the device for a kernel and a download copies what the kernel wrote back") {
  GIVEN("A particle with the first data entry three at the domain centre of a regular grid over every rank") {
    const auto& cellConnectivity = buildGrid();
    Set         particles;
    particles.insert(cellConnectivity, getCandidateAt(0.5, 0.5, 3.0));

    WHEN("A kernel checks the uploaded copy and writes into the second data entry and the first coordinate") {
      const int mismatches                 = getNumberOfMismatchesOfTheDeviceCopy(particles, 3.0, 7.0, 0.25);
      const int numberOfParticlesOfTheView = particles.getView().numberOfParticles;

      AND_WHEN("The set downloads the values") {
        particles.download();

        THEN("The device copy contains the slots and the data and the host values contain what the kernel wrote") {
          VALIDATE_EQUALS(mismatches, 0);
          VALIDATE_EQUALS(numberOfParticlesOfTheView, particles.getSize());
          for (int index = 0; index < particles.getSize(); index++) {
            VALIDATE_NUMERICAL_EQUALS(particles.getData(index)[1], 7.0);
            VALIDATE_NUMERICAL_EQUALS(particles.getPosition(index)(0), 0.25);
          }
        }
      }
    }
  }
}

UNIT_TEST("A value written through the host particle reaches the device copy with the next upload") {
  GIVEN("An uploaded particle with the first data entry three at the domain centre of a regular grid over every rank") {
    const auto& cellConnectivity = buildGrid();
    Set         particles;
    particles.insert(cellConnectivity, getCandidateAt(0.5, 0.5, 3.0));
    particles.upload();

    WHEN("The host particle takes the first data entry five and a kernel checks the uploaded values") {
      for (int index = 0; index < particles.getSize(); index++) {
        particles.getParticle(index).setData(0, 5.0);
      }
      const int mismatches = getNumberOfMismatchesOfTheDeviceCopy(particles, 5.0, 7.0, 0.5);

      THEN("The device copy stores the first data entry five") { VALIDATE_EQUALS(mismatches, 0); }
    }
  }
}

UNIT_TEST("A relocation in which every particle stays in its cell keeps the values a kernel wrote on the device") {
  GIVEN("A relocated particle with the first data entry three at the domain centre of a regular grid over every rank") {
    const auto& cellConnectivity = buildGrid();
    Set         particles;
    particles.insert(cellConnectivity, getCandidateAt(0.5, 0.5, 3.0));
    particles.relocate(cellConnectivity);

    WHEN("A kernel checks the device copy and writes a data entry and the set relocates, uploads and downloads") {
      const int mismatchesOfTheDeviceCopy = getNumberOfMismatchesOfTheDeviceCopy(particles, 3.0, 7.0, 0.5);
      particles.relocate(cellConnectivity);
      particles.upload();
      particles.download();

      THEN("The ranks together store one particle whose host values keep the data entry three and the kernel write") {
        VALIDATE_EQUALS(mismatchesOfTheDeviceCopy, 0);
        VALIDATE_EQUALS(particles.getGlobalNumberOfParticles(), 1);
        for (int index = 0; index < particles.getSize(); index++) {
          VALIDATE_NUMERICAL_EQUALS(particles.getData(index)[0], 3.0);
          VALIDATE_NUMERICAL_EQUALS(particles.getData(index)[1], 7.0);
        }
      }
    }
  }
}

UNIT_TEST("A release frees every array of the set") {
  GIVEN("A regular grid over every rank and an empty set") {
    const auto&       cellConnectivity = buildGrid();
    Set               particles;
    const std::size_t bytesOfTheEmptySet = particles.getAllocatedBytes().getTotal();

    WHEN("The set inserts a particle at the domain centre, uploads it and releases it") {
      particles.insert(cellConnectivity, getCandidateAt(0.5, 0.5, 3.0));
      particles.upload();
      const bool        storesTheParticle   = particles.getSize() > 0;
      const std::size_t bytesAfterTheUpload = particles.getAllocatedBytes().getTotal();
      particles.release();

      THEN("The set occupies bytes only between the upload and the release and only on a rank with the particle") {
        VALIDATE_EQUALS(bytesOfTheEmptySet, std::size_t{0});
        if (storesTheParticle) {
          VALIDATE(bytesAfterTheUpload > 0);
        }
        VALIDATE_EQUALS(particles.getSize(), 0);
        VALIDATE_EQUALS(particles.getAllocatedBytes().getTotal(), std::size_t{0});
      }
    }
  }
}

UNIT_TEST("A relocation moves a particle that a kernel marked as moved out of its cell to the cell that contains it") {
  GIVEN("Three particles over every rank and a kernel that raises data entry one of every particle by one") {
    const auto&         cellConnectivity = buildGrid();
    std::vector<double> candidates;
    for (const auto& candidate :
         {getCandidateAt(0.1, 0.1, 10.0), getCandidateAt(0.5, 0.5, 20.0), getCandidateAt(0.9, 0.9, 30.0)}) {
      candidates.insert(candidates.end(), candidate.begin(), candidate.end());
    }
    Set particles;
    particles.insert(cellConnectivity, candidates);
    const int  globalNumberOfInsertedParticles = peano4::unittests::getGlobalSumOf(particles.getSize());
    const auto moveOnTheDevice                 = [&particles, &cellConnectivity](const double target) {
      particles.upload();
      const auto view                   = particles.getView();
      int        numberOfMovedParticles = 0;
      tarch::accelerator::parallelReduce(
        tarch::accelerator::Range(view.numberOfParticles),
        MoveParticleOnDevice{view, cellConnectivity.onDevice(), target, 0.95},
        tarch::accelerator::Sum<int>{numberOfMovedParticles}
      );
      particles.relocateParticlesMovedOutOfCells(cellConnectivity, numberOfMovedParticles);
      return numberOfMovedParticles;
    };

    WHEN("The kernel moves the particle with the first data entry twenty and the set relocates the marked particles") {
      const int globalNumberOfMovedParticles = peano4::unittests::getGlobalSumOf(moveOnTheDevice(20.0));
      const int globalNumberOfParticles      = peano4::unittests::getGlobalSumOf(particles.getSize());
      particles.download();

      THEN("The ranks keep three particles, each in the cell that contains it and with data entry one raised once") {
        VALIDATE_EQUALS(globalNumberOfInsertedParticles, 3);
        VALIDATE_EQUALS(globalNumberOfMovedParticles, 1);
        VALIDATE_EQUALS(globalNumberOfParticles, 3);
        VALIDATE_EQUALS(particles.getGlobalNumberOfParticles(), 3);
        for (int index = 0; index < particles.getSize(); index++) {
          VALIDATE_EQUALS(
            particles.getSlot(index),
            Set::getSlotAt(cellConnectivity, particles.getPosition(index)),
            index
          );
          VALIDATE_NUMERICAL_EQUALS(particles.getData(index)[1], particles.getData(index)[0] + 2.0, index);
          if (particles.getData(index)[0] == 20.0) {
            VALIDATE_NUMERICAL_EQUALS(particles.getPosition(index)(0), 0.95, index);
          }
        }
      }

      AND_WHEN("The kernel raises data entry one again and moves zero particles") {
        const int globalNumberOfMovedParticlesOfTheSecondRun = peano4::unittests::getGlobalSumOf(moveOnTheDevice(-1.0));
        particles.download();

        THEN("The kernel moves zero particles and raises data entry one of every particle a second time") {
          VALIDATE_EQUALS(globalNumberOfMovedParticlesOfTheSecondRun, 0);
          for (int index = 0; index < particles.getSize(); index++) {
            VALIDATE_NUMERICAL_EQUALS(particles.getData(index)[1], particles.getData(index)[0] + 3.0, index);
          }
        }
      }
    }
  }
}
