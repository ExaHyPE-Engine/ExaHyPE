// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/MemoryLocation.h"

namespace tarch::accelerator {
  template <typename T>
  class ChunkedMemoryPool;
} // namespace tarch::accelerator

/**
 * Owns aligned chunks of one padded size in segments that a slot number addresses. reserve() keeps
 * the address of every chunk and invalidates every view. The segment table of a view lies in the
 * memory location of the pool.
 */
template <typename T>
class tarch::accelerator::ChunkedMemoryPool final {
  static_assert(
    std::is_trivial_v<T> and std::is_standard_layout_v<T>,
    "A memory pool stores trivial standard-layout values only."
  );

public:
  using ValueType = T;

  static constexpr int InvalidSlot                 = -1;
  static constexpr int MinimumSegmentSizeExponent  = 6;
  static constexpr int SegmentsPerFirstReservation = 16;

  /**
   * Addresses the chunks from a kernel. A copy of the view stays valid until the next reserve().
   */
  struct View final {
    T* const* segments{nullptr};
    int       segmentSizeExponent{0};
    int       slotMask{0};
    int       paddedChunkSize{0};

    [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD T* getChunk(int slot) const noexcept;
  };

  struct ConstView final {
    const T* const* segments{nullptr};
    int             segmentSizeExponent{0};
    int             slotMask{0};
    int             paddedChunkSize{0};

    [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD const T* getChunk(int slot) const noexcept;
  };

  ChunkedMemoryPool()                                                      = delete;
  ChunkedMemoryPool(const tarch::accelerator::ChunkedMemoryPool<T>& other) = delete;
  INLINE_METHOD ChunkedMemoryPool(tarch::accelerator::ChunkedMemoryPool<T>&& other) noexcept;
  tarch::accelerator::ChunkedMemoryPool<T>& operator=(const tarch::accelerator::ChunkedMemoryPool<T>& other) = delete;
  INLINE_METHOD tarch::accelerator::ChunkedMemoryPool<T>& operator=(
    tarch::accelerator::ChunkedMemoryPool<T>&& other
  ) noexcept;
  INLINE_METHOD ~ChunkedMemoryPool() noexcept;

  INLINE_METHOD explicit ChunkedMemoryPool(
    int                   chunkSize,
    tarch::MemoryLocation location = tarch::MemoryLocation::Device
  ) noexcept;

  INLINE_METHOD void reserve(int numberOfSlots);
  INLINE_METHOD void release() noexcept;
  INLINE_METHOD void copyToHost(std::vector<T>& target) const;
  INLINE_METHOD void copyFromHost(const std::vector<T>& source);

  [[nodiscard]] INLINE_METHOD const tarch::accelerator::ChunkedMemoryPool<T>& download(
    tarch::accelerator::ChunkedMemoryPool<T>& copyOnHost
  ) const;
  [[nodiscard]] INLINE_METHOD tarch::accelerator::ChunkedMemoryPool<T>& download(
    tarch::accelerator::ChunkedMemoryPool<T>& copyOnHost
  );
  INLINE_METHOD void upload(const tarch::accelerator::ChunkedMemoryPool<T>& copyOnHost);

  [[nodiscard]] INLINE_METHOD tarch::accelerator::ChunkedMemoryPool<T>::View getView() noexcept;
  [[nodiscard]] INLINE_METHOD tarch::accelerator::ChunkedMemoryPool<T>::ConstView getView() const noexcept;

  [[nodiscard]] INLINE_METHOD T* getChunk(int slot) noexcept;
  [[nodiscard]] INLINE_METHOD const T* getChunk(int slot) const noexcept;

  [[nodiscard]] INLINE_METHOD tarch::accelerator::Buffer<T>& getSegment(int segmentNumber) noexcept;
  [[nodiscard]] INLINE_METHOD const tarch::accelerator::Buffer<T>& getSegment(int segmentNumber) const noexcept;

  [[nodiscard]] static INLINE_METHOD int getPaddedChunkSizeOf(int chunkSize) noexcept;
  [[nodiscard]] INLINE_METHOD int getChunkSize() const noexcept;
  [[nodiscard]] INLINE_METHOD int getPaddedChunkSize() const noexcept;
  [[nodiscard]] INLINE_METHOD int getCapacity() const noexcept;
  [[nodiscard]] INLINE_METHOD int getSegmentSize() const noexcept;
  [[nodiscard]] INLINE_METHOD int getNumberOfSegments() const noexcept;
  [[nodiscard]] INLINE_METHOD tarch::AllocatedBytes getAllocatedBytes() const noexcept;
  [[nodiscard]] INLINE_METHOD tarch::MemoryLocation getLocation() const noexcept;
  [[nodiscard]] INLINE_METHOD bool isHostAccessible() const noexcept;

private:
  INLINE_METHOD void appendSegment();
  INLINE_METHOD void uploadSegmentTable();
  INLINE_METHOD void copySegmentsTo(tarch::accelerator::ChunkedMemoryPool<T>& copyOnHost) const;

  int                                        _chunkSize{0};
  int                                        _paddedChunkSize{0};
  std::vector<tarch::accelerator::Buffer<T>> _segments{};
  std::vector<T*>                            _segmentBasesOnHost{};
  tarch::accelerator::Buffer<T*>             _segmentTable{tarch::MemoryLocation::Device};
  int                                        _segmentSizeExponent{0};
  tarch::MemoryLocation                      _location{tarch::MemoryLocation::Device};
};

#include "tarch/accelerator/ChunkedMemoryPool.cpph"
