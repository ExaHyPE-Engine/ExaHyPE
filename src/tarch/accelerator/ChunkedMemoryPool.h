// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/accelerator.h"

namespace tarch::accelerator {
  template <typename T, int N>
  class ChunkedMemoryPool;
} // namespace tarch::accelerator

/**
 * Owns an aligned contiguous buffer of equally sized chunks.
 *
 * Each slot selects one chunk at a padded stride. The host-accessible buffer
 * also exposes the strided layout for batched operations. A chunk that enters
 * the live range reads as zero, and a heap chunk turns resident on its first write.
 *
 * @tparam T Trivial, standard-layout element type.
 * @tparam N Positive number of elements per chunk.
 */
template <typename T, int N>
class tarch::accelerator::ChunkedMemoryPool final {
  static_assert(N > 0, "A chunk holds a positive number of elements.");
  static_assert(
    std::is_trivial_v<T> and std::is_standard_layout_v<T>,
    "The element type has to be trivial and standard-layout so a chunk is a flat byte range."
  );

public:
  //! Identifies a chunk by its owning entity's pool slot.
  using SlotIndex = std::uint32_t;

  static constexpr SlotIndex InvalidSlot = std::numeric_limits<SlotIndex>::max();

  //! Stores the padded element stride between consecutive chunks.
  static constexpr int PaddedChunkSize = [] {
    // The smallest run of elements whose bytes fill whole alignment blocks
    // follows from the greatest common divisor. std::lcm would say the same,
    // but its overflow check rests on a builtin that not every compiler
    // evaluates in a constant expression.
    constexpr auto GreatestCommonDivisor = [](std::size_t left, std::size_t right) {
      while (right != 0) {
        const std::size_t remainder = left % right;
        left                        = right;
        right                       = remainder;
      }
      return left;
    };
    constexpr auto        Alignment                 = static_cast<std::size_t>(PEANO_WITH_ALIGNMENT);
    constexpr std::size_t ElementsPerAlignmentBlock = Alignment / GreatestCommonDivisor(Alignment, sizeof(T));
    return static_cast<int>(
      (static_cast<std::size_t>(N) + ElementsPerAlignmentBlock - 1) / ElementsPerAlignmentBlock
      * ElementsPerAlignmentBlock
    );
  }();

  explicit ChunkedMemoryPool(
    MemoryLocation location = MemoryLocation::Heap,
    int            device   = accelerator::Device::DefaultDevice
  ) noexcept;
  ChunkedMemoryPool(const ChunkedMemoryPool& other) = delete;
  ChunkedMemoryPool(ChunkedMemoryPool&& other) noexcept;
  ChunkedMemoryPool& operator=(const ChunkedMemoryPool& other) = delete;
  ChunkedMemoryPool& operator=(ChunkedMemoryPool&& other) noexcept;
  ~ChunkedMemoryPool() noexcept;

  void clear() noexcept;

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD T*       chunk(SlotIndex slot) noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD const T* chunk(SlotIndex slot) const noexcept;

  [[nodiscard]] INLINE_METHOD T*       rawData() noexcept;
  [[nodiscard]] INLINE_METHOD const T* rawData() const noexcept;

  [[nodiscard]] INLINE_METHOD std::size_t size() const noexcept;
  [[nodiscard]] INLINE_METHOD std::size_t capacity() const noexcept;
  [[nodiscard]] INLINE_METHOD bool        empty() const noexcept;
  [[nodiscard]] INLINE_METHOD std::size_t rawSize() const noexcept;
  [[nodiscard]] INLINE_METHOD std::size_t rawSizeInBytes() const noexcept;
  [[nodiscard]] INLINE_METHOD std::size_t allocatedBytes() const noexcept;

  void resize(std::size_t numberOfChunks);

private:
  T*   rawAllocate(std::size_t chunks) const;
  void rawFree(T* buffer, std::size_t chunks) const noexcept;
  void clearChunks(std::size_t firstChunk, std::size_t endChunk) noexcept;

  //! Points to the contiguous buffer of _capacity chunks.
  T* _data{nullptr};

  std::size_t _capacity{0};

  //! Counts the live chunks in the buffer.
  std::size_t _size{0};

  //! Counts the leading chunks that have been live since the buffer was
  //! allocated. Every chunk beyond them reads as zero.
  std::size_t _touchedChunks{0};

  MemoryLocation _location{MemoryLocation::Heap};
  int            _device{accelerator::Device::DefaultDevice};
};

#include "tarch/accelerator/ChunkedMemoryPool.cpph"
