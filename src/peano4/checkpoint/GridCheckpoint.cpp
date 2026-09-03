// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridCheckpoint.h"

#ifdef PEANO_WITH_HDF5

#include "peano4/topology/TopologyManager.h"
#include "peano4/topology/Tree.h"
#include "tarch/Termination.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/HDF5File.h"
#include "tarch/reader/HDF5FileReader.h"

namespace {
  std::string gridFilePath(const std::string& directory) {
    return (std::filesystem::path(directory) / peano4::checkpoint::GridFileName).string();
  }
} // namespace

void peano4::checkpoint::createDirectoryFor(const std::string& path) {
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::error_code directoryError;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), directoryError);
    if (directoryError) {
      tarch::terminateWithError("Cannot create checkpoint directory for \"" + path + "\": " + directoryError.message());
    }
  }
  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
}

void peano4::checkpoint::writeGridFile(
  const std::string&                           directory,
  const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
  const tarch::la::Vector<DIMENSIONS, double>& domainSize,
  const std::vector<std::int32_t>&             extraTable,
  const int                                    extraTableColumns,
  const double                                 timeStamp
) {
  const std::string path = gridFilePath(directory);
  createDirectoryFor(path);

  const auto file = tarch::plotter::HDF5File::openForDataHoldingRanks(
    path, tarch::mpi::Rank::getInstance().isGlobalMaster()
  );
  if (file == nullptr) {
    return;
  }

  const auto& topologyManager = peano4::topology::TopologyManager::getInstance();
  const auto& plan            = topologyManager.getRefinementPlan();
  const int   depth           = topologyManager.getTree().getDepth();

  const std::int32_t dimensions = DIMENSIONS;
  const std::int32_t depthValue = depth;
  file->writeTable("/dimensions", 1, 0, &dimensions);
  file->writeTable("/depth", 1, 0, &depthValue);
  file->writeTable("/domain/offset", DIMENSIONS, 0, domainOffset.data());
  file->writeTable("/domain/size", DIMENSIONS, 0, domainSize.data());

  for (std::size_t planLevel = 0; planLevel < plan.size(); planLevel++) {
    std::vector<std::int32_t> flaggedVertices;
    flaggedVertices.reserve(plan[planLevel].size() * DIMENSIONS);
    for (const auto& vertex : plan[planLevel]) {
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        flaggedVertices.push_back(vertex(axis));
      }
    }
    file->writeTable(
      "/plan/level" + std::to_string(depth + static_cast<int>(planLevel)),
      plan[planLevel].size(),
      DIMENSIONS,
      flaggedVertices.data()
    );
  }

  file->writeTable("/extra", extraTableColumns > 0 ? extraTable.size() / extraTableColumns : 0, extraTableColumns, extraTable.data());
  file->writeTable("/time", 1, 0, &timeStamp);
}

int peano4::checkpoint::restoreGrid(
  const std::string&                           directory,
  const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
  const tarch::la::Vector<DIMENSIONS, double>& domainSize
) {
  const tarch::reader::HDF5FileReader reader(gridFilePath(directory));

  const std::int32_t dimensions = reader.readInt32Rows("/dimensions", 0, 1).front();
  if (dimensions != DIMENSIONS) {
    tarch::terminateWithError(
      "The checkpoint " + reader.getFilename() + " stores a " + std::to_string(dimensions)
      + "-dimensional grid, but this executable is built for " + std::to_string(DIMENSIONS) + " dimensions."
    );
  }
  const auto storedOffset = reader.readDoubleRows("/domain/offset", 0, DIMENSIONS);
  const auto storedSize   = reader.readDoubleRows("/domain/size", 0, DIMENSIONS);
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    if (storedOffset[axis] != domainOffset(axis) or storedSize[axis] != domainSize(axis)) {
      tarch::terminateWithError(
        "The checkpoint " + reader.getFilename() + " belongs to a different domain than this executable's."
      );
    }
  }

  const int depth = reader.readInt32Rows("/depth", 0, 1).front();

  peano4::topology::Tree::RefinementPlan plan;
  for (int level = depth; reader.datasetExists("/plan/level" + std::to_string(level)); level++) {
    const std::string path = "/plan/level" + std::to_string(level);
    const auto        rows = reader.getDatasetDimensions(path).first;
    const auto        flat = reader.readInt32Rows(path, 0, rows);
    auto&             flaggedVertices = plan.emplace_back();
    flaggedVertices.reserve(rows);
    for (std::uint64_t row = 0; row < rows; row++) {
      tarch::la::Vector<DIMENSIONS, int> vertex(0);
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        vertex(axis) = flat[row * DIMENSIONS + axis];
      }
      flaggedVertices.push_back(vertex);
    }
  }

  peano4::topology::TopologyManager::getInstance().adoptRefinementPlan(std::move(plan));
  return depth;
}

std::vector<std::int32_t> peano4::checkpoint::readExtraTable(const std::string& directory, int& columns) {
  const tarch::reader::HDF5FileReader reader(gridFilePath(directory));
  const auto                          dimensions = reader.getDatasetDimensions("/extra");
  columns                                        = static_cast<int>(dimensions.second);
  return reader.readInt32Rows("/extra", 0, dimensions.first);
}

#endif
