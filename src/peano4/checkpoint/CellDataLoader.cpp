// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CellDataLoader.h"

#ifdef PEANO_WITH_HDF5

#include "peano4/topology/TopologyManager.h"
#include "peano4/topology/Tree.h"
#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/mpi/Rank.h"

peano4::checkpoint::CellDataLoader::CellDataLoader(
  const std::string& path, const std::string& group, const int valuesPerRecord
) :
  _reader(path),
  _group(group),
  _valuesPerRecord(valuesPerRecord) {
  assertion(not group.empty());
  assertion(group.front() == '/', group);
  assertion(valuesPerRecord > 0, valuesPerRecord);
  _timeStamp     = _reader.readDoubleRows(_group + "/time", 0, 1).front();
  _solverScalars = _reader.readDoubleRows(
    _group + "/scalars", 0, _reader.getDatasetDimensions(_group + "/scalars").first
  );
}

void peano4::checkpoint::CellDataLoader::loadLocalRecords() {
  const auto& tree  = peano4::topology::TopologyManager::getInstance().getTree();
  const int   rank  = tarch::mpi::Rank::getInstance().getRank();
  _depth            = tree.getDepth();

  _values.clear();
  _recordOfKey.clear();

  for (int level = _depth; _reader.datasetExists(_group + "/cells/level" + std::to_string(level) + "/count"); level++) {
    const std::string prefix     = _group + "/cells/level" + std::to_string(level);
    const int         blockLevel = peano4::checkpoint::blockLevelOfLevel(level);
    const auto        counts
      = _reader.readInt64Rows(prefix + "/count", 0, peano4::checkpoint::numberOfBlocksOnLevel(level));

    auto& values      = _values.emplace_back();
    auto& recordOfKey = _recordOfKey.emplace_back();

    // A level whose cells are all refined further holds no records, so the
    // writer creates no key and values datasets for it.
    if (not _reader.datasetExists(prefix + "/values")) {
      continue;
    }

    if (_reader.getDatasetDimensions(prefix + "/values").second != static_cast<std::uint64_t>(_valuesPerRecord)) {
      tarch::terminateWithError(
        "The checkpoint " + _reader.getFilename() + " stores "
        + std::to_string(_reader.getDatasetDimensions(prefix + "/values").second)
        + " values per cell, but this solver's cells hold " + std::to_string(_valuesPerRecord) + "."
      );
    }

    std::int64_t firstRow = 0;
    for (std::int64_t block = 0; block < static_cast<std::int64_t>(counts.size()); block++) {
      const std::int64_t rows = counts[block];
      if (rows > 0
          and tree.subtreeTouchesRankAt(
            blockLevel, peano4::checkpoint::latticeIndexOfTriadicMortonKey(block, blockLevel), rank
          )) {
        const auto keys       = _reader.readInt64Rows(prefix + "/key", firstRow, rows);
        const auto blockValues = _reader.readDoubleRows(prefix + "/values", firstRow, rows);
        const auto firstRecord = values.size() / _valuesPerRecord;
        values.insert(values.end(), blockValues.begin(), blockValues.end());
        recordOfKey.reserve(recordOfKey.size() + keys.size());
        for (std::size_t row = 0; row < keys.size(); row++) {
          recordOfKey.emplace(keys[row], firstRecord + row);
        }
      }
      firstRow += rows;
    }
  }
}

const double* peano4::checkpoint::CellDataLoader::record(
  const int level, const tarch::la::Vector<DIMENSIONS, int>& latticeIndex
) const {
  const int planLevel = level - _depth;
  if (planLevel < 0 or planLevel >= static_cast<int>(_recordOfKey.size())) {
    return nullptr;
  }
  const auto entry = _recordOfKey[planLevel].find(peano4::checkpoint::triadicMortonKey(latticeIndex, level));
  if (entry == _recordOfKey[planLevel].end()) {
    return nullptr;
  }
  return _values[planLevel].data() + entry->second * _valuesPerRecord;
}

double peano4::checkpoint::CellDataLoader::getTimeStamp() const { return _timeStamp; }

const std::vector<double>& peano4::checkpoint::CellDataLoader::getSolverScalars() const { return _solverScalars; }

#endif
