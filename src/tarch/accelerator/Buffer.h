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
 * storage only. allocate() places the elements on the device that is current at the call.
 */
template <typename T>
class tarch::accelerator::Buffer final {
  static_assert(std::is_trivially_copyable_v<T>, "A buffer element travels between memory locations as bytes.");

public:
  Buffer()                                           = default;
  Buffer(const tarch::accelerator::Buffer<T>& other) = delete;
  INLINE_METHOD Buffer(tarch::accelerator::Buffer<T>&& other) noexcept;
  tarch::accelerator::Buffer<T>& operator=(const tarch::accelerator::Buffer<T>& other) = delete;
  INLINE_METHOD tarch::accelerator::Buffer<T>& operator=(tarch::accelerator::Buffer<T>&& other) noexcept;
  INLINE_METHOD ~Buffer() noexcept;

  INLINE_METHOD explicit Buffer(tarch::MemoryLocation location) noexcept;
  INLINE_METHOD Buffer(std::size_t size, tarch::MemoryLocation location);

  INLINE_METHOD void allocate(std::size_t size);
  INLINE_METHOD void release() noexcept;

  INLINE_METHOD void copyFromHost(const T* source, std::size_t count);
  INLINE_METHOD void copyToHost(T* target, std::size_t count) const;
  INLINE_METHOD void setValues(const std::vector<T>& values);
  [[nodiscard]] INLINE_METHOD std::vector<T> getValues() const;

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
