// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CellDataWriter.h"

#ifdef PEANO_WITH_HDF5

#include "peano4/checkpoint/CheckpointFile.h"
#include "peano4/topology/TopologyManager.h"
#include "peano4/topology/Tree.h"
#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/HDF5File.h"

template void peano4::checkpoint::CellDataWriter::Records::append(
  int level, const tarch::la::Vector<DIMENSIONS, int>& latticeIndex, const float* cellValues, int count
);
template void peano4::checkpoint::CellDataWriter::Records::append(
  int level, const tarch::la::Vector<DIMENSIONS, int>& latticeIndex, const double* cellValues, int count
);
template void peano4::checkpoint::CellDataWriter::Records::append(
  int level, const tarch::la::Vector<DIMENSIONS, int>& latticeIndex, const long double* cellValues, int count
);

void peano4::checkpoint::CellDataWriter::Records::clear() {
  levels.clear();
  keys.clear();
  values.clear();
}

peano4::checkpoint::CellDataWriter::CellDataWriter(const std::string& group, const int valuesPerRecord) :
  _group(group),
  _valuesPerRecord(valuesPerRecord) {
  assertion(not group.empty());
  assertion(group.front() == '/', group);
  assertion(valuesPerRecord > 0, valuesPerRecord);
}

void peano4::checkpoint::CellDataWriter::collect(Records&& records) {
  const std::lock_guard<std::mutex> lock(_collectMutex);
  _collectedRecords.push_back(std::move(records));
}

void peano4::checkpoint::CellDataWriter::write(const std::vector<double>& solverScalars, const double timeStamp) {
  const auto& topologyManager = peano4::topology::TopologyManager::getInstance();
  const auto& tree            = topologyManager.getTree();
  const int   depth           = tree.getDepth();
  const int   maxLevel        = depth + static_cast<int>(topologyManager.getRefinementPlan().size());
  const int   numberOfLevels  = maxLevel - depth + 1;

  // Merge the per-task buffers and sort the records by level and key, which
  // is the order the file keeps.
  std::size_t numberOfRecords = 0;
  for (const auto& records : _collectedRecords) {
    numberOfRecords += records.keys.size();
  }
  std::vector<std::int32_t> levels;
  std::vector<std::int64_t> keys;
  std::vector<double>       values;
  levels.reserve(numberOfRecords);
  keys.reserve(numberOfRecords);
  values.reserve(numberOfRecords * _valuesPerRecord);
  std::vector<std::size_t> order(numberOfRecords);
  {
    std::vector<double> unsortedValues;
    unsortedValues.reserve(numberOfRecords * _valuesPerRecord);
    for (const auto& records : _collectedRecords) {
      levels.insert(levels.end(), records.levels.begin(), records.levels.end());
      keys.insert(keys.end(), records.keys.begin(), records.keys.end());
      unsortedValues.insert(unsortedValues.end(), records.values.begin(), records.values.end());
    }
    _collectedRecords.clear();

    std::iota(order.begin(), order.end(), std::size_t{0});
    std::sort(order.begin(), order.end(), [&levels, &keys](const std::size_t left, const std::size_t right) {
      return levels[left] != levels[right] ? levels[left] < levels[right] : keys[left] < keys[right];
    });
    std::vector<std::int32_t> sortedLevels(numberOfRecords);
    std::vector<std::int64_t> sortedKeys(numberOfRecords);
    values.resize(unsortedValues.size());
    for (std::size_t record = 0; record < numberOfRecords; record++) {
      sortedLevels[record] = levels[order[record]];
      sortedKeys[record]   = keys[order[record]];
      std::copy_n(
        unsortedValues.begin() + static_cast<std::ptrdiff_t>(order[record] * _valuesPerRecord),
        _valuesPerRecord,
        values.begin() + static_cast<std::ptrdiff_t>(record * _valuesPerRecord)
      );
    }
    levels = std::move(sortedLevels);
    keys   = std::move(sortedKeys);
  }

  // Count the local records per block, then reduce the counts into the global
  // table every rank derives its row offsets from.
  std::vector<std::int64_t> firstBlockOfLevel(numberOfLevels + 1, 0);
  for (int level = depth; level <= maxLevel; level++) {
    firstBlockOfLevel[level - depth + 1] = firstBlockOfLevel[level - depth]
                                           + peano4::checkpoint::numberOfBlocksOnLevel(level);
  }
  std::vector<std::int64_t> blockCounts(firstBlockOfLevel[numberOfLevels], 0);
  const auto             blockOfRecord = [depth, &levels, &keys, &firstBlockOfLevel](const std::size_t record) {
    const int level = levels[record];
    assertion(level >= depth, level, depth);
    std::int64_t blockKey = keys[record];
    for (int digit = peano4::checkpoint::blockLevelOfLevel(level); digit < level; digit++) {
      blockKey /= THREE_POWER_D;
    }
    return firstBlockOfLevel[level - depth] + blockKey;
  };
  for (std::size_t record = 0; record < numberOfRecords; record++) {
    blockCounts[blockOfRecord(record)]++;
  }
  std::vector<std::int64_t> globalBlockCounts(blockCounts.size(), 0);
  tarch::mpi::allreduce(
    blockCounts.data(),
    globalBlockCounts.data(),
    static_cast<int>(blockCounts.size()),
    MPI_INT64_T,
    MPI_SUM,
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
  std::vector<std::int64_t> firstRowOfBlock(globalBlockCounts.size() + 1, 0);
  std::vector<std::int64_t> firstRowOfLevel(numberOfLevels + 1, 0);
  for (std::size_t block = 0; block < globalBlockCounts.size(); block++) {
    firstRowOfBlock[block + 1] = firstRowOfBlock[block] + globalBlockCounts[block];
  }
  for (int level = 0; level <= numberOfLevels; level++) {
    firstRowOfLevel[level] = firstRowOfBlock[firstBlockOfLevel[level]];
  }

  // Turn each record's key into its global row: The block's first row plus
  // the key's rank in the block's leaf enumeration, which the plan fixes
  // identically on every rank.
  std::vector<std::int64_t> rows(numberOfRecords, 0);
  {
    std::vector<std::int64_t> blockLeafKeys;
    std::size_t               record = 0;
    while (record < numberOfRecords) {
      const std::int64_t block      = blockOfRecord(record);
      std::size_t        blockEnd   = record;
      while (blockEnd < numberOfRecords and blockOfRecord(blockEnd) == block) {
        blockEnd++;
      }
      const int level      = levels[record];
      const int blockLevel = peano4::checkpoint::blockLevelOfLevel(level);
      blockLeafKeys.clear();
      peano4::checkpoint::appendLeafKeysOfBlock(
        tree,
        level,
        blockLevel,
        peano4::checkpoint::latticeIndexOfTriadicMortonKey(block - firstBlockOfLevel[level - depth], blockLevel),
        blockLeafKeys
      );
      if (static_cast<std::int64_t>(blockLeafKeys.size()) != globalBlockCounts[block]) {
        tarch::terminateWithError(
          "The refinement plan holds " + std::to_string(blockLeafKeys.size())
          + " leaves for a checkpoint block, but the traversals recorded " + std::to_string(globalBlockCounts[block])
          + "."
        );
      }
      std::size_t leaf = 0;
      for (; record < blockEnd; record++) {
        while (leaf < blockLeafKeys.size() and blockLeafKeys[leaf] < keys[record]) {
          leaf++;
        }
        if (leaf >= blockLeafKeys.size() or blockLeafKeys[leaf] != keys[record]) {
          tarch::terminateWithError("A checkpointed cell is not a leaf of the refinement plan.");
        }
        rows[record] = firstRowOfBlock[block] - firstRowOfLevel[levels[record] - depth]
                       + static_cast<std::int64_t>(leaf);
        leaf++;
      }
    }
  }

  tarch::plotter::HDF5File& file = peano4::checkpoint::CheckpointFile::getInstance().getFile();

  std::size_t firstRecordOfLevel = 0;
  for (int level = depth; level <= maxLevel; level++) {
    const std::string        prefix    = _group + "/cells/level" + std::to_string(level);
    const std::int64_t       totalRows = firstRowOfLevel[level - depth + 1] - firstRowOfLevel[level - depth];
    std::size_t              levelEnd  = firstRecordOfLevel;
    while (levelEnd < numberOfRecords and levels[levelEnd] == level) {
      levelEnd++;
    }

    // The records are sorted by key, so their rows ascend and consecutive
    // rows merge into runs; one collective write per dataset covers them all.
    std::vector<unsigned long long> runFirstRows;
    std::vector<unsigned long long> runRowCounts;
    for (std::size_t record = firstRecordOfLevel; record < levelEnd; record++) {
      if (not runFirstRows.empty()
          and runFirstRows.back() + runRowCounts.back() == static_cast<unsigned long long>(rows[record])) {
        runRowCounts.back()++;
      } else {
        runFirstRows.push_back(static_cast<unsigned long long>(rows[record]));
        runRowCounts.push_back(1);
      }
    }
    const int numberOfRuns = static_cast<int>(runFirstRows.size());

    file.writeTable(
      prefix + "/count",
      static_cast<unsigned long long>(peano4::checkpoint::numberOfBlocksOnLevel(level)),
      0,
      globalBlockCounts.data() + firstBlockOfLevel[level - depth]
    );
    {
      tarch::plotter::HDF5File::Dataset keyDataset(
        file,
        prefix + "/key",
        static_cast<unsigned long long>(totalRows),
        0,
        tarch::plotter::HDF5File::ElementType::Int64
      );
      keyDataset.writeRuns(
        runFirstRows.data(),
        runRowCounts.data(),
        numberOfRuns,
        keys.data() + firstRecordOfLevel,
        tarch::plotter::HDF5File::ElementType::Int64
      );
    }
    {
      tarch::plotter::HDF5File::Dataset valueDataset(
        file,
        prefix + "/values",
        static_cast<unsigned long long>(totalRows),
        _valuesPerRecord,
        tarch::plotter::HDF5File::ElementType::Float64
      );
      valueDataset.writeRuns(
        runFirstRows.data(),
        runRowCounts.data(),
        numberOfRuns,
        values.data() + firstRecordOfLevel * _valuesPerRecord,
        tarch::plotter::HDF5File::ElementType::Float64
      );
    }

    firstRecordOfLevel = levelEnd;
  }

  file.writeTable(_group + "/scalars", solverScalars.size(), 0, solverScalars.data());
  file.writeTable(_group + "/time", 1, 0, &timeStamp);
}

#endif
