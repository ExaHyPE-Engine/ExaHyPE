// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParticleDataWriter.h"

#ifdef PEANO_WITH_HDF5

#include "peano4/checkpoint/CheckpointFile.h"
#include "tarch/Assertions.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
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

peano4::checkpoint::ParticleDataWriter::ParticleDataWriter(const std::string& group, const int valuesPerParticle) :
  _group(group),
  _valuesPerParticle(valuesPerParticle) {
  assertion(not group.empty());
  assertion(group.front() == '/', group);
  assertion(valuesPerParticle >= 2, valuesPerParticle);
}

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
  peano4::checkpoint::sortParticleRows(rows, _valuesPerParticle);

  tarch::plotter::HDF5File& file      = peano4::checkpoint::CheckpointFile::getInstance().getFile();
  const int                 localRows = static_cast<int>(rows.size() / _valuesPerParticle);

  // The checkpoint file spans every rank, so the numbers travel over the global
  // communicator. Every rank then derives each row from the particle numbers
  // alone, which gives the same file whatever rank holds which particle.
  const MPI_Comm   communicator  = tarch::mpi::Rank::getInstance().getCommunicator();
  const int        numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  std::vector<int> rowsPerRank(numberOfRanks, 0);
  tarch::mpi::allgather(&localRows, 1, MPI_INT, rowsPerRank.data(), 1, MPI_INT, communicator);

  std::vector<int> numbersPerRank(numberOfRanks, 0);
  std::vector<int> displacements(numberOfRanks, 0);
  int              totalRows = 0;
  for (int rank = 0; rank < numberOfRanks; rank++) {
    numbersPerRank[rank] = 2 * rowsPerRank[rank];
    displacements[rank]  = 2 * totalRows;
    totalRows += rowsPerRank[rank];
  }

  std::vector<double> localNumbers(2 * static_cast<std::size_t>(localRows), 0.0);
  for (std::size_t row = 0; row < static_cast<std::size_t>(localRows); row++) {
    localNumbers[2 * row]     = rows[row * _valuesPerParticle];
    localNumbers[2 * row + 1] = rows[row * _valuesPerParticle + 1];
  }
  std::vector<double> gatheredNumbers(2 * static_cast<std::size_t>(totalRows), 0.0);
  tarch::mpi::allgatherv(
    localNumbers.data(),
    2 * localRows,
    MPI_DOUBLE,
    gatheredNumbers.data(),
    numbersPerRank.data(),
    displacements.data(),
    MPI_DOUBLE,
    communicator
  );

  std::vector<std::pair<double, double>> sortedNumbers;
  sortedNumbers.reserve(static_cast<std::size_t>(totalRows));
  for (std::size_t row = 0; row < static_cast<std::size_t>(totalRows); row++) {
    sortedNumbers.emplace_back(gatheredNumbers[2 * row], gatheredNumbers[2 * row + 1]);
  }
  std::sort(sortedNumbers.begin(), sortedNumbers.end());
  assertion(std::adjacent_find(sortedNumbers.begin(), sortedNumbers.end()) == sortedNumbers.end());

  // The local rows ascend in the same order as the global ones, so consecutive
  // rows merge into runs and one collective write covers them all.
  std::vector<unsigned long long> runFirstRows;
  std::vector<unsigned long long> runRowCounts;
  for (std::size_t row = 0; row < static_cast<std::size_t>(localRows); row++) {
    const std::pair<double, double> number(localNumbers[2 * row], localNumbers[2 * row + 1]);
    const auto                      entry = std::lower_bound(sortedNumbers.begin(), sortedNumbers.end(), number);
    assertion(entry != sortedNumbers.end() and *entry == number, number.first, number.second);
    const auto globalRow = static_cast<unsigned long long>(entry - sortedNumbers.begin());
    if (not runFirstRows.empty() and runFirstRows.back() + runRowCounts.back() == globalRow) {
      runRowCounts.back()++;
    } else {
      runFirstRows.push_back(globalRow);
      runRowCounts.push_back(1);
    }
  }

  tarch::plotter::HDF5File::Dataset dataset(
    file,
    _group + "/particles",
    static_cast<unsigned long long>(totalRows),
    _valuesPerParticle,
    tarch::plotter::HDF5File::ElementType::Float64
  );
  dataset.writeRuns(
    runFirstRows.data(),
    runRowCounts.data(),
    static_cast<int>(runFirstRows.size()),
    rows.data(),
    tarch::plotter::HDF5File::ElementType::Float64
  );
}

#endif
