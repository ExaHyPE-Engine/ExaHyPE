// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Partition.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/MemoryLocation.h"

#include <mpi.h>

namespace peano4 {
  template <typename T>
  class HaloExchange;
} // namespace peano4

/**
 * Exchanges the halves of the faces this rank and a neighbour rank both hold. A face chunk of the
 * pool holds one half per side; post() sends the half this rank owns of every exchanged face and
 * wait() writes the half the neighbour owns. The buffers live where the pool lives, so with a GPU
 * backend MPI reads and writes device memory.
 */
template <typename T>
class peano4::HaloExchange final {
public:
  using Pool = tarch::accelerator::ChunkedMemoryPool<T>;

  explicit HaloExchange(Pool& pool);
  HaloExchange(const HaloExchange& other)            = delete;
  HaloExchange(HaloExchange&& other)                 = delete;
  HaloExchange& operator=(const HaloExchange& other) = delete;
  HaloExchange& operator=(HaloExchange&& other)      = delete;
  ~HaloExchange() noexcept;

  void rebuild(const peano4::Partition& partition);
  void release();
  void post();
  void wait();

  [[nodiscard]] INLINE_METHOD int getNumberOfEntries() const noexcept;
  [[nodiscard]] INLINE_METHOD int getTag() const noexcept;
  [[nodiscard]] INLINE_METHOD tarch::AllocatedBytes getAllocatedBytes() const noexcept;

private:
  struct Message final {
    int rank{-1};
    int firstEntry{0};
    int numberOfEntries{0};
  };

  bool pollMessages();

  Pool& _pool;

  int _halfSize{0};

  static constexpr int NoTag = -1;

  int _tag{NoTag};

  std::vector<Message> _messages{};
  int                  _numberOfEntries{0};

  tarch::accelerator::Buffer<peano4::ExchangedFace> _entries;
  tarch::accelerator::Buffer<T>                     _sendBuffer;
  tarch::accelerator::Buffer<T>                     _receiveBuffer;

  std::vector<MPI_Request> _requests{};

  bool _inFlight{false};

  std::atomic<bool> _messagesCompleted{true};
};

#include "peano4/HaloExchange.cpph"
