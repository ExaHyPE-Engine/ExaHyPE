// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

namespace tarch::mpi {
  class ReductionRecord;
} // namespace tarch::mpi

namespace exahype2::runner {
  struct SolverStatistic;
  class GlobalStatistics;
  struct GridSnapshot;
  class Report;

  using SolverStatistics = std::vector<std::pair<std::string, std::vector<SolverStatistic>>>;

  INLINE_METHOD SolverStatistic makeValueStatistic(const char* label, double value);
  INLINE_METHOD SolverStatistic makeRangeStatistic(const char* label, double minimum, double maximum);
  INLINE_METHOD SolverStatistic makeCountStatistic(const char* label, long value);

  template <typename Solver, typename AppendAdditionalStatistics>
  INLINE_METHOD std::pair<std::string, std::vector<SolverStatistic>> makeSolverStatistics(
    const char*                       solverName,
    const Solver&                     solver,
    const AppendAdditionalStatistics& appendAdditionalStatistics
  );

  GlobalStatistics& getGlobalStatistics();

  GridSnapshot collectGridSnapshot();
  std::vector<std::string> describeGridSnapshot(const GridSnapshot& snapshot);
  std::vector<std::string> describeGridTransition(const GridSnapshot& before, const GridSnapshot& after);
  void writeGridSnapshot(Report& report);
} // namespace exahype2::runner

/**
 * One value, range or count a solver reports per time step.
 */
struct exahype2::runner::SolverStatistic {
  enum class Kind { Value, Count };

  std::string label{};
  Kind        kind{Kind::Value};
  double      minimum{0.0};
  double      maximum{0.0};
  long        count{0};
};

/**
 * Grid and memory statistics of all ranks, as of the last completed program step.
 */
class exahype2::runner::GlobalStatistics final {
public:
  GlobalStatistics() = default;

  void setLocalSolverBytes(std::size_t hostBytes, std::size_t deviceBytes);
  void setLocalCellUpdates(std::uint64_t solverCells, std::uint64_t cellDoFs);
  void addLocalValuesToGlobalReduction(tarch::mpi::ReductionRecord& record, bool refreshMemory);
  void readValuesFromGlobalReduction(const tarch::mpi::ReductionRecord& record);

  [[nodiscard]] std::uint64_t getGlobalNumberOfInnerUnrefinedCells() const;
  [[nodiscard]] std::uint64_t getGlobalNumberOfInnerRefinedCells() const;
  [[nodiscard]] std::uint64_t getGlobalNumberOfFaces() const;
  [[nodiscard]] std::uint64_t getGlobalNumberOfHangingFaces() const;
  [[nodiscard]] std::uint64_t getGlobalMinRankBoundaryFaces() const;
  [[nodiscard]] std::uint64_t getGlobalMaxRankBoundaryFaces() const;
  [[nodiscard]] double getGlobalRankBoundaryFacePercentage() const;
  [[nodiscard]] std::uint64_t getGlobalMinRankLoad() const;
  [[nodiscard]] std::uint64_t getGlobalMaxRankLoad() const;
  [[nodiscard]] std::uint64_t getGlobalCellConnectivityHostBytes() const;
  [[nodiscard]] std::uint64_t getGlobalCellConnectivityDeviceBytes() const;
  [[nodiscard]] std::uint64_t getGlobalTreeBytes() const;
  [[nodiscard]] std::uint64_t getGlobalSolverHostBytes() const;
  [[nodiscard]] std::uint64_t getGlobalSolverDeviceBytes() const;
  [[nodiscard]] std::uint64_t getGlobalNumberOfSolverCells() const;
  [[nodiscard]] std::uint64_t getGlobalNumberOfCellDoFs() const;
  [[nodiscard]] std::uint64_t getGlobalProcessMemoryUsageInBytes() const;
  [[nodiscard]] std::uint64_t getGlobalTotalMemoryInBytes() const;
  [[nodiscard]] std::uint64_t getGlobalAvailableMemoryInBytes() const;
  [[nodiscard]] double getGlobalInterRankBalance() const;
  [[nodiscard]] int getGlobalMinimumLevel() const;
  [[nodiscard]] int getGlobalMaximumLevel() const;
  [[nodiscard]] double getGlobalMinimumCellWidth() const;
  [[nodiscard]] int getGlobalNumberOfStationaryRegrids() const;

private:
  static constexpr std::chrono::seconds MemorySamplingInterval{1};

  std::chrono::steady_clock::time_point _lastMemorySample{};
  std::uint64_t                         _localProcessMemoryUsageInBytes{0};
  std::uint64_t                         _localTotalMemoryInBytes{0};
  std::uint64_t                         _localAvailableMemoryInBytes{0};
  std::uint64_t                         _localSolverHostBytes{0};
  std::uint64_t                         _localSolverDeviceBytes{0};
  std::uint64_t                         _localSolverCells{0};
  std::uint64_t                         _localCellDoFs{0};
  int                                   _firstSumSlot{0};
  int                                   _firstMinimumSlot{0};
  int                                   _firstMaximumSlot{0};

  std::uint64_t _globalNumberOfInnerUnrefinedCells{0};
  std::uint64_t _globalNumberOfInnerRefinedCells{0};
  std::uint64_t _globalNumberOfFaces{0};
  std::uint64_t _globalNumberOfHangingFaces{0};
  std::uint64_t _globalNumberOfRankBoundaryFaces{0};
  std::uint64_t _globalMinRankBoundaryFaces{0};
  std::uint64_t _globalMaxRankBoundaryFaces{0};
  std::uint64_t _globalMinRankLoad{0};
  std::uint64_t _globalMaxRankLoad{0};
  std::uint64_t _globalCellConnectivityHostBytes{0};
  std::uint64_t _globalCellConnectivityDeviceBytes{0};
  std::uint64_t _globalTreeBytes{0};
  std::uint64_t _globalSolverHostBytes{0};
  std::uint64_t _globalSolverDeviceBytes{0};
  std::uint64_t _globalNumberOfSolverCells{0};
  std::uint64_t _globalNumberOfCellDoFs{0};
  std::uint64_t _globalProcessMemoryUsageInBytes{0};
  std::uint64_t _globalTotalMemoryInBytes{0};
  std::uint64_t _globalAvailableMemoryInBytes{0};
  int           _globalMinimumLevel{0};
  int           _globalMaximumLevel{0};
  double        _globalMinimumCellWidth{0.0};
  int           _globalNumberOfStationaryRegrids{0};
};

/**
 * One entry per row of the grid summary, rendered as value, suffix and detail in brackets. A row
 * with an empty value stays out of every report.
 */
struct exahype2::runner::GridSnapshot {
  enum Row {
    GridCells,
    GridFaces,
    GridLevels,
    MPILoadBalance,
    MPIRankBoundaryFaces,
    MemoryCPUGrid,
    MemoryGPUGrid,
    MemoryCPUSolverData,
    MemoryGPUSolverData,
    MemoryResident,
    MemoryFree,
    RowCount
  };

  struct Entry {
    std::string value{};
    std::string detail{};
    std::string suffix{};
  };

  std::array<Entry, RowCount> entries{{}};
};

#include "exahype2/runner/Statistics.cpph"
