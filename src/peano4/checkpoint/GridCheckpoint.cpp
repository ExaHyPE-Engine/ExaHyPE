// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridCheckpoint.h"

#ifdef PEANO_WITH_HDF5

#include "peano4/checkpoint/CheckpointFile.h"
#include "peano4/topology/TopologyManager.h"
#include "peano4/topology/Tree.h"
#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/HDF5File.h"
#include "tarch/reader/HDF5FileReader.h"

void peano4::checkpoint::createDirectoryFor(const std::string& path) {
  assertion(not path.empty());
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::error_code directoryError;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), directoryError);
    if (directoryError) {
      tarch::terminateWithError("Cannot create checkpoint directory for \"" + path + "\": " + directoryError.message());
    }
  }
  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
}

void peano4::checkpoint::writeGrid(
  const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
  const tarch::la::Vector<DIMENSIONS, double>& domainSize,
  const std::vector<std::int32_t>&             extraTable,
  const int                                    extraTableColumns,
  const double                                 timeStamp
) {
  assertion(extraTableColumns >= 0, extraTableColumns);
  assertion(
    extraTableColumns == 0 or extraTable.size() % extraTableColumns == 0, extraTable.size(), extraTableColumns
  );

  tarch::plotter::HDF5File& file  = peano4::checkpoint::CheckpointFile::getInstance().getFile();
  const std::string         group = peano4::checkpoint::GridGroup;

  const auto& topologyManager = peano4::topology::TopologyManager::getInstance();
  const auto& plan            = topologyManager.getRefinementPlan();
  const int   depth           = topologyManager.getTree().getDepth();

  const std::int32_t dimensions = DIMENSIONS;
  const std::int32_t depthValue = depth;
  file.writeTable(group + "/dimensions", 1, 0, &dimensions);
  file.writeTable(group + "/depth", 1, 0, &depthValue);
  file.writeTable(group + "/domain/offset", DIMENSIONS, 0, domainOffset.data());
  file.writeTable(group + "/domain/size", DIMENSIONS, 0, domainSize.data());

  for (std::size_t planLevel = 0; planLevel < plan.size(); planLevel++) {
    std::vector<std::int32_t> flaggedVertices;
    flaggedVertices.reserve(plan[planLevel].size() * DIMENSIONS);
    for (const auto& vertex : plan[planLevel]) {
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        flaggedVertices.push_back(vertex(axis));
      }
    }
    file.writeTable(
      group + "/plan/level" + std::to_string(depth + static_cast<int>(planLevel)),
      plan[planLevel].size(),
      DIMENSIONS,
      flaggedVertices.data()
    );
  }

  file.writeTable(
    group + "/extra",
    extraTableColumns > 0 ? extraTable.size() / extraTableColumns : 0,
    extraTableColumns,
    extraTable.data()
  );
  file.writeTable(group + "/time", 1, 0, &timeStamp);
}

int peano4::checkpoint::restoreGrid(
  const std::string&                           path,
  const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
  const tarch::la::Vector<DIMENSIONS, double>& domainSize
) {
  assertion(not path.empty());

  const tarch::reader::HDF5FileReader reader(path);
  const std::string                   group = peano4::checkpoint::GridGroup;

  const std::int32_t dimensions = reader.readInt32Rows(group + "/dimensions", 0, 1).front();
  if (dimensions != DIMENSIONS) {
    tarch::terminateWithError(
      "The checkpoint " + reader.getFilename() + " stores a " + std::to_string(dimensions)
      + "-dimensional grid, but this executable is built for " + std::to_string(DIMENSIONS) + " dimensions."
    );
  }
  const auto storedOffset = reader.readDoubleRows(group + "/domain/offset", 0, DIMENSIONS);
  const auto storedSize   = reader.readDoubleRows(group + "/domain/size", 0, DIMENSIONS);
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    if (storedOffset[axis] != domainOffset(axis) or storedSize[axis] != domainSize(axis)) {
      tarch::terminateWithError(
        "The checkpoint " + reader.getFilename() + " belongs to a different domain than this executable's."
      );
    }
  }

  const int depth = reader.readInt32Rows(group + "/depth", 0, 1).front();

  peano4::topology::Tree::RefinementPlan plan;
  for (int level = depth; reader.datasetExists(group + "/plan/level" + std::to_string(level)); level++) {
    const std::string planPath = group + "/plan/level" + std::to_string(level);
    const auto        rows     = reader.getDatasetDimensions(planPath).first;
    const auto        flat     = reader.readInt32Rows(planPath, 0, rows);
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

std::vector<std::int32_t> peano4::checkpoint::readExtraTable(const std::string& path, int& columns) {
  assertion(not path.empty());

  const tarch::reader::HDF5FileReader reader(path);
  const std::string                   extraPath  = std::string(peano4::checkpoint::GridGroup) + "/extra";
  const auto                          dimensions = reader.getDatasetDimensions(extraPath);
  columns                                        = static_cast<int>(dimensions.second);
  return reader.readInt32Rows(extraPath, 0, dimensions.first);
}

#endif
