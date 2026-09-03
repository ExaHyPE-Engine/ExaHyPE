// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParticleDataWriter.h"

#ifdef PEANO_WITH_HDF5

#include "peano4/checkpoint/GridCheckpoint.h"
#include "tarch/Assertions.h"
#include "tarch/plotter/HDF5File.h"

void peano4::checkpoint::sortParticleRows(std::vector<double>& rows, const int valuesPerParticle) {
  assertion(valuesPerParticle >= 2);
  assertion(rows.size() % valuesPerParticle == 0);
  const std::size_t        numberOfRows = rows.size() / valuesPerParticle;
  std::vector<std::size_t> order(numberOfRows);
  std::iota(order.begin(), order.end(), std::size_t{0});
  std::sort(order.begin(), order.end(), [&rows, valuesPerParticle](const std::size_t left, const std::size_t right) {
    const double* const leftRow  = rows.data() + left * valuesPerParticle;
    const double* const rightRow = rows.data() + right * valuesPerParticle;
    return leftRow[0] != rightRow[0] ? leftRow[0] < rightRow[0] : leftRow[1] < rightRow[1];
  });
  std::vector<double> sortedRows(rows.size());
  for (std::size_t row = 0; row < numberOfRows; row++) {
    std::copy_n(
      rows.begin() + static_cast<std::ptrdiff_t>(order[row] * valuesPerParticle),
      valuesPerParticle,
      sortedRows.begin() + static_cast<std::ptrdiff_t>(row * valuesPerParticle)
    );
  }
  rows = std::move(sortedRows);
}

peano4::checkpoint::ParticleDataWriter::ParticleDataWriter(const std::string& path, const int valuesPerParticle) :
  _path(path),
  _valuesPerParticle(valuesPerParticle) {}

void peano4::checkpoint::ParticleDataWriter::collect(std::vector<double>&& rows) {
  assertion(rows.size() % _valuesPerParticle == 0);
  const std::lock_guard<std::mutex> lock(_collectMutex);
  _collectedRows.push_back(std::move(rows));
}

void peano4::checkpoint::ParticleDataWriter::write() {
  std::vector<double> rows;
  for (const auto& collected : _collectedRows) {
    rows.insert(rows.end(), collected.begin(), collected.end());
  }
  _collectedRows.clear();
  sortParticleRows(rows, _valuesPerParticle);

  createDirectoryFor(_path);
  const unsigned long long localRows = rows.size() / _valuesPerParticle;
  const auto               file      = tarch::plotter::HDF5File::openForDataHoldingRanks(_path, localRows > 0);
  if (file == nullptr) {
    return;
  }
  unsigned long long firstRow  = 0;
  unsigned long long totalRows = 0;
  file->deriveRowRanges(&localRows, &firstRow, &totalRows, 1);
  tarch::plotter::HDF5File::Dataset dataset(
    *file, "/particles", totalRows, _valuesPerParticle, tarch::plotter::HDF5File::ElementType::Float64
  );
  dataset.writeRows(firstRow, localRows, rows.data(), tarch::plotter::HDF5File::ElementType::Float64);
}

#endif
