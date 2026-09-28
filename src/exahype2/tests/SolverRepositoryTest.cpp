// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SolverRepositoryTest.h"

#include "exahype2/RefinementRequests.h"
#include "exahype2/SolverRepository.h"
#include "exahype2/TimeStepping.h"
#include "peano4/Partition.h"
#include "peano4/Regrid.h"
#include "peano4/Spacetree.h"
#include "peano4/tests/TestGrids.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/plotter/Format.h"

namespace {
  constexpr int Depth = 2;

  /**
   * A solver with OwnsCells reports the cells it holds data on; the other one holds data on every cell.
   */
  template <bool OwnsCells>
  struct RecordingSolver final {
    exahype2::TimeStepping timeStepping{
      exahype2::TimeStepping::Rule::Fixed,
      1.0,
      exahype2::TimeStepping::Stability::PhysicalEigenvalue
    };
    int                   numberOfDoFs{0};
    int                   numberOfCells{0};
    tarch::AllocatedBytes allocatedBytes{};

    int phaseCalls{0};

    [[nodiscard]] const exahype2::TimeStepping& getTimeStepping() const { return timeStepping; }
    [[nodiscard]] int getNumberOfDoFs() const { return numberOfDoFs; }
    [[nodiscard]] int getNumberOfCells([[maybe_unused]] const peano4::Partition& partition) const
      requires OwnsCells
    {
      return numberOfCells;
    }
    [[nodiscard]] tarch::AllocatedBytes getAllocatedBytes() const { return allocatedBytes; }

    void startSimulation() { phaseCalls++; }
    void finishSimulation() { phaseCalls++; }
    void allocate([[maybe_unused]] const peano4::Partition& partition) { phaseCalls++; }
    void release() { phaseCalls++; }
    void requestRefinementForGridConstruction(
      [[maybe_unused]] const peano4::Partition&      partition,
      [[maybe_unused]] exahype2::RefinementRequests& requests
    ) {
      phaseCalls++;
    }
    void initialise(
      [[maybe_unused]] const peano4::Partition&      partition,
      [[maybe_unused]] exahype2::RefinementRequests& requests
    ) {
      phaseCalls++;
    }
    void performTimeStep(
      [[maybe_unused]] const peano4::Partition&      partition,
      [[maybe_unused]] exahype2::RefinementRequests& requests
    ) {
      phaseCalls++;
    }
    void regrid(
      [[maybe_unused]] const peano4::Partition&      partition,
      [[maybe_unused]] const peano4::Regrid&         regrid,
      [[maybe_unused]] exahype2::RefinementRequests& requests
    ) {
      phaseCalls++;
    }
    void plot(
      [[maybe_unused]] const peano4::Partition&     partition,
      [[maybe_unused]] const tarch::plotter::Format format,
      [[maybe_unused]] const std::string&           outputDirectory
    ) {
      phaseCalls++;
    }
  };
} // namespace

exahype2::tests::SolverRepositoryTest::SolverRepositoryTest() :
  tarch::tests::TestCase("exahype2::tests::SolverRepositoryTest") {}

void
exahype2::tests::SolverRepositoryTest::run() {
  testMethod(testTheRepositoryReducesTheValuesOfEverySolver);
  testMethod(testEveryPhaseCallReachesEverySolver);
}

/**
 * The time stamps, cells, degrees of freedom and bytes of the project come from every solver, and a
 * solver without own cells counts every cell of the partition.
 */
void
exahype2::tests::SolverRepositoryTest::testTheRepositoryReducesTheValuesOfEverySolver() {
  RecordingSolver<true>  coarse;
  RecordingSolver<false> fine;
  coarse.timeStepping.advanceTimeStamp(0.5);
  fine.timeStepping.advanceTimeStamp(0.125);
  fine.timeStepping.advanceTimeStamp(0.125);
  coarse.numberOfDoFs   = 64;
  coarse.numberOfCells  = 3;
  coarse.allocatedBytes = {1024, 256};
  fine.numberOfDoFs     = 128;
  fine.allocatedBytes   = {512, 0};

  peano4::tests::buildRegularGrid(Depth);
  const auto& partition = peano4::Spacetree::getInstance().getPartition();

  const exahype2::SolverRepository repository{coarse, fine};
  validateNumericalEquals(repository.getMinTimeStamp(), 0.25);
  validateNumericalEquals(repository.getMaxTimeStamp(), 0.5);
  validateEquals(
    repository.getNumberOfSolverCells(partition),
    std::uint64_t{3} + static_cast<std::uint64_t>(partition.getCells().size)
  );
  validateEquals(
    repository.getNumberOfCellDoFs(partition),
    std::uint64_t{3} * 64 + static_cast<std::uint64_t>(partition.getCells().size) * 128
  );
  validateEquals(repository.getAllocatedBytes().host, std::size_t{1536});
  validateEquals(repository.getAllocatedBytes().device, std::size_t{256});
}

/**
 * Every phase call reaches every solver once, and an empty partition yields no refinement request.
 */
void
exahype2::tests::SolverRepositoryTest::testEveryPhaseCallReachesEverySolver() {
  RecordingSolver<false>     first;
  RecordingSolver<false>     second;
  exahype2::SolverRepository repository{first, second};
  const peano4::Partition    partition{};
  const peano4::Regrid       regrid{};

  repository.startSimulation();
  repository.allocate(partition);
  repository.requestRefinementForGridConstruction(partition);
  repository.initialise(partition);
  repository.performTimeStep(partition);
  repository.regrid(partition, regrid);
  repository.plot(partition, tarch::plotter::Format::VTU, "");
  repository.finishSimulation();

  validateEquals(first.phaseCalls, 9);
  validateEquals(second.phaseCalls, 9);
  validate(repository.getRefinementRequests().empty());
}
