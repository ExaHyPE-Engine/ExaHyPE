// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/CellConnectivity.h"
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
 * Exchanges the faces that both this rank and a neighbour rank store. post() sends the half of a face
 * chunk that this rank owns and wait() writes the half of the neighbour. The buffers use the memory
 * location of the pool.
 */
template <typename T>
class peano4::HaloExchange final {
public:
  using Pool = tarch::accelerator::ChunkedMemoryPool<T>;

  INLINE_METHOD explicit HaloExchange(Pool& pool);
  INLINE_METHOD HaloExchange(Pool& pool, peano4::FaceKind exchangedFaceKind);
  HaloExchange(const HaloExchange& other)            = delete;
  HaloExchange(HaloExchange&& other)                 = delete;
  HaloExchange& operator=(const HaloExchange& other) = delete;
  HaloExchange& operator=(HaloExchange&& other)      = delete;
  INLINE_METHOD ~HaloExchange() noexcept;

  INLINE_METHOD void rebuild(const peano4::CellConnectivity& cellConnectivity);
  INLINE_METHOD void release();
  INLINE_METHOD void post();
  INLINE_METHOD void wait();

  [[nodiscard]] INLINE_METHOD int getNumberOfEntries() const noexcept;
  [[nodiscard]] INLINE_METHOD int getTag() const noexcept;
  [[nodiscard]] INLINE_METHOD tarch::AllocatedBytes getAllocatedBytes() const noexcept;

private:
  struct Message final {
    int rank{-1};
    int firstEntry{0};
    int numberOfEntries{0};
  };

  INLINE_METHOD void postMessages();
  INLINE_METHOD bool pollMessages();

  Pool& _pool;

  int _halfSize{0};

  std::optional<peano4::FaceKind> _exchangedFaceKind{};

  static constexpr int NoTag = -1;

  int _tag{NoTag};

  std::vector<Message> _messages{};
  int                  _numberOfEntries{0};

  tarch::accelerator::Buffer<peano4::ExchangedFace> _entries;
  tarch::accelerator::Buffer<T>                     _sendBuffer;
  tarch::accelerator::Buffer<T>                     _receiveBuffer;

  std::vector<MPI_Request> _requests{};

  bool _inFlight{false};

  bool _messagesPosted{true};

  std::atomic<bool> _messagesCompleted{true};
};

#include "peano4/HaloExchange.cpph"
