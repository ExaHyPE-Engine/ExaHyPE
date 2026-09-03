// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#ifdef PEANO_WITH_HDF5

namespace peano4::checkpoint {
  class ParticleDataWriter;

  /** Sorts the rows by their two leading number columns. */
  void sortParticleRows(std::vector<double>& rows, int valuesPerParticle);
} // namespace peano4::checkpoint

/**
 * Writes one particle set's records into one checkpoint file.
 *
 * Every mesh traversal task appends one row per local particle it visits and
 * hands its rows over through collect(). write() sorts every rank's rows by
 * particle number and places them into one file with one collective write.
 */
class peano4::checkpoint::ParticleDataWriter {
public:
  ParticleDataWriter(const std::string& path, int valuesPerParticle);
  ParticleDataWriter(const ParticleDataWriter&)            = delete;
  ParticleDataWriter(ParticleDataWriter&&)                 = delete;
  ParticleDataWriter& operator=(const ParticleDataWriter&) = delete;
  ParticleDataWriter& operator=(ParticleDataWriter&&)      = delete;
  ~ParticleDataWriter() = default;

  void collect(std::vector<double>&& rows);

  /** Writes the collected rows into the /particles dataset. Collective over every rank. */
  void write();

private:
  const std::string _path;
  const int         _valuesPerParticle;

  std::mutex                      _collectMutex;
  std::deque<std::vector<double>> _collectedRows;
};

#endif
