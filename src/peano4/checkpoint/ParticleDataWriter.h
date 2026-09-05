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
 * Writes one particle set's records into one group of the checkpoint file.
 *
 * Every mesh traversal task appends one row per local particle it visits and
 * hands its rows over through collect(). write() derives each row from the
 * particle numbers of every rank, so the rows follow the particle numbers and
 * not the rank layout, and places them into the group with one collective
 * write.
 */
class peano4::checkpoint::ParticleDataWriter final {
public:
  /** The group is the checkpoint file's group this particle set writes into, such as "/particles/Tracer". */
  ParticleDataWriter(const std::string& group, int valuesPerParticle);
  ParticleDataWriter(const ParticleDataWriter&)            = delete;
  ParticleDataWriter(ParticleDataWriter&&)                 = delete;
  ParticleDataWriter& operator=(const ParticleDataWriter&) = delete;
  ParticleDataWriter& operator=(ParticleDataWriter&&)      = delete;
  ~ParticleDataWriter() = default;

  void collect(std::vector<double>&& rows);

  /** Writes the collected rows into the /particles dataset. Collective over every rank. */
  void write();

private:
  const std::string _group;
  const int         _valuesPerParticle;

  /**
   * Each traversal task takes this lock once, when collect() hands its rows
   * over. The rows move into the deque, which no atomic operation performs.
   */
  std::mutex                      _collectMutex;
  std::deque<std::vector<double>> _collectedRows;
};

#endif
