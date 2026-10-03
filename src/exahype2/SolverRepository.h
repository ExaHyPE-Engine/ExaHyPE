// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/RefinementRequests.h"
#include "peano4/RefinementRequest.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/plotter/Format.h"

namespace peano4 {
  class Partition;
  struct Regrid;
} // namespace peano4

namespace exahype2 {
  template <typename... Solvers>
  class SolverRepository;
} // namespace exahype2

/**
 * Runs every phase on every solver. Every solver outlives the repository. A phase that votes
 * merges the votes of all solvers into one list of refinement requests.
 */
template <typename... Solvers>
class exahype2::SolverRepository final {
public:
  INLINE_METHOD explicit SolverRepository(Solvers&... solvers);

  SolverRepository(const SolverRepository&)            = delete;
  SolverRepository(SolverRepository&&)                 = delete;
  SolverRepository& operator=(const SolverRepository&) = delete;
  SolverRepository& operator=(SolverRepository&&)      = delete;
  ~SolverRepository()                                  = default;

  [[nodiscard]] INLINE_METHOD double getMinTimeStamp() const;
  [[nodiscard]] INLINE_METHOD double getMaxTimeStamp() const;
  [[nodiscard]] INLINE_METHOD std::uint64_t getNumberOfSolverCells(const peano4::Partition& partition) const;
  [[nodiscard]] INLINE_METHOD std::uint64_t getNumberOfCellDoFs(const peano4::Partition& partition) const;

  INLINE_METHOD void startSimulation();
  INLINE_METHOD void finishSimulation();
  INLINE_METHOD void allocate(const peano4::Partition& partition);
  [[nodiscard]] INLINE_METHOD tarch::AllocatedBytes getAllocatedBytes() const;
  INLINE_METHOD void requestRefinementForGridConstruction(const peano4::Partition& partition);
  INLINE_METHOD void initialise(const peano4::Partition& partition);
  INLINE_METHOD void performTimeStep(const peano4::Partition& partition);
  INLINE_METHOD void regrid(const peano4::Partition& partition, const peano4::Regrid& regrid);
  INLINE_METHOD void plot(
    const peano4::Partition& partition,
    tarch::plotter::Format   format,
    const std::string&       outputDirectory
  );

  [[nodiscard]] INLINE_METHOD const std::vector<peano4::RefinementRequest>& getRefinementRequests() const;

private:
  template <typename Operation>
  INLINE_METHOD void forEachSolver(const Operation& operation);

  template <typename Operation>
  INLINE_METHOD void forEachSolver(const Operation& operation) const;

  INLINE_METHOD void commitRefinementRequests(const peano4::Partition& partition);

  template <typename Solver>
  [[nodiscard]] static INLINE_METHOD int getNumberOfCellsOf(const Solver& solver, const peano4::Partition& partition);

  std::tuple<Solvers&...> _solvers;

  RefinementRequests _refinementRequests{};

  std::vector<peano4::RefinementRequest> _requestsOfTheRun{};
};

#include "exahype2/SolverRepository.cpph"
