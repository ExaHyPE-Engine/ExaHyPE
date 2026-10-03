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
   * Compile-time list of particle data entries.
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
 * View of one particle. The array values stores the DIMENSIONS coordinates first and then
 * numberOfData data entries.
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
 * Trivially copyable view of a particle set for a kernel.
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
 * Particles of one rank in a host array and a device copy. A kernel that writes the device copy
 * leaves the host values stale until download(). insert() and relocate() run on the host and move
 * every particle to the rank that owns its cell.
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

  INLINE_METHOD void insert(const peano4::Partition& partition, const std::vector<double>& candidates);
  INLINE_METHOD void relocate(const peano4::Partition& partition);
  INLINE_METHOD void upload();
  INLINE_METHOD void download();
  INLINE_METHOD void release();

  [[nodiscard]] static INLINE_METHOD int getSlotAt(
    const peano4::Partition&                     partition,
    const tarch::la::Vector<DIMENSIONS, double>& position
  );

  [[nodiscard]] INLINE_METHOD ParticleSetView<NumberOfData> getView();

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

  INLINE_METHOD void locate(const peano4::Partition& partition);
  INLINE_METHOD void exchange(const peano4::Partition& partition);
  INLINE_METHOD void append(const double* values, int number0, int number1, int slot);
};

#include "exahype2/particles/ParticleSet.cpph"
