// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/Device.h"
#include "tarch/accelerator/MemoryLocation.h"

namespace tarch::accelerator {
  template <typename T>
  class Buffer;
} // namespace tarch::accelerator

/**
 * Owns trivially copyable elements in one memory location. Host code dereferences host-accessible
 * storage only. An allocation lands on the current device, which holds the elements until the next
 * allocation.
 */
template <typename T>
class tarch::accelerator::Buffer final {
  static_assert(std::is_trivially_copyable_v<T>, "A buffer element travels between memory locations as bytes.");

public:
  Buffer()                                           = default;
  Buffer(const tarch::accelerator::Buffer<T>& other) = delete;
  Buffer(tarch::accelerator::Buffer<T>&& other) noexcept;
  tarch::accelerator::Buffer<T>& operator=(const tarch::accelerator::Buffer<T>& other) = delete;
  tarch::accelerator::Buffer<T>& operator=(tarch::accelerator::Buffer<T>&& other) noexcept;
  ~Buffer() noexcept;

  explicit Buffer(tarch::MemoryLocation location) noexcept;
  Buffer(std::size_t size, tarch::MemoryLocation location);

  void allocate(std::size_t size);
  void release() noexcept;

  void copyFromHost(const T* source, std::size_t count);
  void copyToHost(T* target, std::size_t count) const;
  void setValues(const std::vector<T>& values);
  [[nodiscard]] std::vector<T> getValues() const;

  [[nodiscard]] INLINE_METHOD T* getData() noexcept;
  [[nodiscard]] INLINE_METHOD const T* getData() const noexcept;
  [[nodiscard]] INLINE_METHOD std::size_t getSize() const noexcept;
  [[nodiscard]] INLINE_METHOD std::size_t getSizeInBytes() const noexcept;
  [[nodiscard]] INLINE_METHOD tarch::AllocatedBytes getAllocatedBytes() const noexcept;
  [[nodiscard]] INLINE_METHOD bool isEmpty() const noexcept;
  [[nodiscard]] INLINE_METHOD tarch::MemoryLocation getLocation() const noexcept;

private:
  T*                    _data{nullptr};
  std::size_t           _size{0};
  tarch::MemoryLocation _location{tarch::MemoryLocation::Heap};
  int                   _device{tarch::accelerator::Device::DefaultDevice};
};

#include "tarch/accelerator/Buffer.cpph"
