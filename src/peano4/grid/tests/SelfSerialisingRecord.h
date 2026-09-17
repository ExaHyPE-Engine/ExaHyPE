// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/grid/Stack.h"
#include "peano4/grid/StackExchange.h"

namespace peano4::grid::tests {
  class SelfSerialisingRecord;
} // namespace peano4::grid::tests

/**
 * Variable-length record that ships through the exchange on its own protocol.
 *
 * The record owns its values on the heap, so the bytes behind it are not the
 * bytes to send. It therefore provides the protocol statics the exchange looks
 * for and sends one message of per-record counts followed by one message with
 * all values. The particle sets work the same way.
 */
class peano4::grid::tests::SelfSerialisingRecord final {
public:
  using DoFType = int;

  SelfSerialisingRecord()                                              = default;
  SelfSerialisingRecord& operator=(const SelfSerialisingRecord& other) = delete;

  SelfSerialisingRecord(const SelfSerialisingRecord& other);
  SelfSerialisingRecord(SelfSerialisingRecord&& other) noexcept;
  SelfSerialisingRecord& operator=(SelfSerialisingRecord&& other) noexcept;
  explicit SelfSerialisingRecord(std::size_t numberOfValues, DoFType value);
  ~SelfSerialisingRecord() noexcept;

  DoFType     at(std::size_t index) const;
  std::size_t size() const;

  static void startSend(Stack<SelfSerialisingRecord>& stack, StackExchangeJob<SelfSerialisingRecord>& job);

  static void startReceive(
    Stack<SelfSerialisingRecord>&            stack,
    StackExchangeJob<SelfSerialisingRecord>& job,
    int                                      numberOfElements
  );

  static bool tryToFinishSendOrReceive(
    Stack<SelfSerialisingRecord>&            stack,
    StackExchangeJob<SelfSerialisingRecord>& job
  );

private:
  void clear();

  std::vector<DoFType*> _values{};
};
