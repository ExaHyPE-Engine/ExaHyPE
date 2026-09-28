// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Partition.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace exahype2::particles {
  struct Particle;

  /**
   * Names particle data entries as a type, so a dispatch over them needs no parameter pack.
   */
  template <int... Fields>
  struct FieldList final {};

  template <int NumberOfData>
  struct ParticleSetView;

  template <int NumberOfData>
  class ParticleSet;
} // namespace exahype2::particles

TARGET_DECLARE
/**
 * One particle as a kernel reads and writes it: The values array holds the DIMENSIONS
 * coordinates first and the numberOfData data entries behind them.
 */
struct exahype2::particles::Particle final {
  double* values{nullptr};
  int     numberOfData{0};

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD double getX(int axis) const;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double> getX() const;
  INLINE_METHOD GPU_CALLABLE_METHOD void setX(int axis, double value);
  INLINE_METHOD GPU_CALLABLE_METHOD void setX(const tarch::la::Vector<DIMENSIONS, double>& position);
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD double getData(int index) const;
  INLINE_METHOD GPU_CALLABLE_METHOD void setData(int index, double value);
};

/**
 * Trivially copyable description of a particle set a kernel reads: The values of every particle
 * and the slot of the cell that holds it.
 */
template <int NumberOfData>
struct exahype2::particles::ParticleSetView final {
  static constexpr int ValuesPerParticle = DIMENSIONS + NumberOfData;

  int        numberOfParticles{0};
  double*    values{nullptr};
  const int* slots{nullptr};

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD Particle getParticle(int index) const;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getSlot(int index) const;
};
TARGET_DECLARE_END

/**
 * Holds the particles of one rank: The position and the data of every particle in a host array
 * and its device copy, its two-integer number and the slot of the cell that holds it. upload()
 * and download() copy only what changed since the last copy, and a kernel that writes the device
 * copy leaves the host values stale until download(). insert() and relocate() run on the host and
 * move every particle to the rank whose partition holds its position.
 */
template <int NumberOfData>
class exahype2::particles::ParticleSet final {
public:
  static constexpr int ValuesPerParticle = DIMENSIONS + NumberOfData;

  ParticleSet()                                    = default;
  ParticleSet(const ParticleSet& other)            = delete;
  ParticleSet(ParticleSet&& other)                 = delete;
  ParticleSet& operator=(const ParticleSet& other) = delete;
  ParticleSet& operator=(ParticleSet&& other)      = delete;
  ~ParticleSet()                                   = default;

  void insert(const peano4::Partition& partition, const std::vector<double>& candidates);
  void relocate(const peano4::Partition& partition);
  void upload();
  void download();
  void release();

  [[nodiscard]] static int getSlotAt(
    const peano4::Partition&                     partition,
    const tarch::la::Vector<DIMENSIONS, double>& position
  );

  [[nodiscard]] ParticleSetView<NumberOfData> getView();

  [[nodiscard]] INLINE_METHOD int getSize() const noexcept;
  [[nodiscard]] INLINE_METHOD int getGlobalNumberOfParticles() const noexcept;
  [[nodiscard]] INLINE_METHOD tarch::AllocatedBytes getAllocatedBytes() const noexcept;
  [[nodiscard]] INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> getPosition(int index) const;
  [[nodiscard]] INLINE_METHOD const double* getData(int index) const;
  [[nodiscard]] INLINE_METHOD Particle getParticle(int index);
  [[nodiscard]] INLINE_METHOD std::pair<int, int> getNumber(int index) const;
  [[nodiscard]] INLINE_METHOD int getSlot(int index) const;

private:
  std::vector<double> _values{};
  std::vector<int>    _numbers{};
  std::vector<int>    _slots{};
  int                 _nextNumber{0};
  int                 _globalNumberOfParticles{0};

  tarch::accelerator::Buffer<double> _valuesOnDevice{tarch::MemoryLocation::Device};
  tarch::accelerator::Buffer<int>    _slotsOnDevice{tarch::MemoryLocation::Device};
  bool                               _hostValuesAreCurrent{true};
  bool                               _deviceValuesAreCurrent{false};
  bool                               _deviceSlotsAreCurrent{false};

  void locate(const peano4::Partition& partition);
  void exchange(const peano4::Partition& partition);
  void append(const double* values, int number0, int number1, int slot);
};

#include "exahype2/particles/ParticleSet.cpph"
