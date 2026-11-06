// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/CellConnectivity.h"
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
  struct CompactParticlesMovedOutOfCells;

  template <int NumberOfData>
  class ParticleSet;
} // namespace exahype2::particles

TARGET_DECLARE
namespace exahype2::particles {
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getLatticeIndexOf(
    double coordinate,
    double latticeOffset,
    double width,
    int    cellsPerAxis
  );
} // namespace exahype2::particles

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
 * Trivially copyable view of a particle set for a kernel. markAsMovedOutOfCell() records in the slot
 * array that a kernel moved the particle out of its cell. ParticleSet::relocateParticlesMovedOutOfCells()
 * reads the marks.
 */
template <int NumberOfData>
struct exahype2::particles::ParticleSetView final {
  static constexpr int ValuesPerParticle = DIMENSIONS + NumberOfData;

  int     numberOfParticles{0};
  double* values{nullptr};
  int*    slots{nullptr};

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD Particle getParticle(int index) const;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getSlot(int index) const;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD bool hasMovedOutOfCell(int index) const;
  INLINE_METHOD GPU_CALLABLE_METHOD void markAsMovedOutOfCell(int index) const;

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD bool isInsideCell(
    const peano4::CellConnectivity&              cellConnectivity,
    int                                          index,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset
  ) const;
};

/**
 * Copies the values of the particle movedParticles[entry] into movedValues and moves the particle
 * fillers[entry] from the tail of the set into the vacated entry. A kernel calls it once per
 * moved particle.
 */
template <int NumberOfData>
struct exahype2::particles::CompactParticlesMovedOutOfCells final {
  ParticleSetView<NumberOfData> particles{};
  double*                       movedValues{nullptr};
  const int*                    movedParticles{nullptr};
  const int*                    fillers{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
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

  INLINE_METHOD void insert(const peano4::CellConnectivity& cellConnectivity, const std::vector<double>& candidates);
  INLINE_METHOD void relocate(const peano4::CellConnectivity& cellConnectivity);

  INLINE_METHOD void relocateParticlesMovedOutOfCells(
    const peano4::CellConnectivity& cellConnectivity,
    int                             numberOfMovedParticles
  );

  INLINE_METHOD void upload();
  INLINE_METHOD void download();
  INLINE_METHOD void release();

  [[nodiscard]] static INLINE_METHOD int getSlotAt(
    const peano4::CellConnectivity&              cellConnectivity,
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
  static constexpr int MinimumDeviceCapacity = 64;

  std::vector<double> _values{};
  std::vector<int>    _numbers{};
  std::vector<int>    _slots{};
  int                 _nextNumber{0};
  int                 _globalNumberOfParticles{0};

  tarch::accelerator::Buffer<double> _valuesOnDevice{tarch::MemoryLocation::Device};
  tarch::accelerator::Buffer<int>    _slotsOnDevice{tarch::MemoryLocation::Device};
  tarch::accelerator::Buffer<int>    _movedParticlesOnDevice{tarch::MemoryLocation::Device};
  tarch::accelerator::Buffer<int>    _fillersOnDevice{tarch::MemoryLocation::Device};
  tarch::accelerator::Buffer<double> _movedValuesOnDevice{tarch::MemoryLocation::Device};
  bool                               _hostValuesAreCurrent{true};
  bool                               _deviceValuesAreCurrent{false};
  bool                               _deviceSlotsAreCurrent{false};

  INLINE_METHOD void takeMovedParticlesFromTheDevice(
    int                  numberOfMovedParticles,
    std::vector<double>& values,
    std::vector<int>&    numbers
  );

  INLINE_METHOD void placeParticles(
    const peano4::CellConnectivity& cellConnectivity,
    const std::vector<double>&      values,
    const std::vector<int>&         numbers
  );

  INLINE_METHOD void appendArrivals(
    const std::vector<double>& values,
    const std::vector<int>&    numbers,
    const std::vector<int>&    slots
  );

  INLINE_METHOD void append(const double* values, int number0, int number1, int slot);
};

#include "exahype2/particles/ParticleSet.cpph"
