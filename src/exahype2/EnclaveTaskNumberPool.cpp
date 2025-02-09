// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "EnclaveTaskNumberPool.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"

exahype2::EnclaveTaskNumberPool::EnclaveTaskNumberPool(int refillSize)
  : _refillSize(refillSize)
  , _semaphore() {

  reset(0);
}

int exahype2::EnclaveTaskNumberPool::getNext() {
  int number = -1;

  {
    tarch::multicore::Lock lock(_semaphore);
    if (!_numberPool.empty()) {
      number = _numberPool.front();
      _numberPool.pop();
      assertion(number >= 0);
      return number;
    }
  }

  // If the pool is empty, refill it
  refillPool();
  return getNext(); // Recursive call now that pool is refilled
}

void exahype2::EnclaveTaskNumberPool::release(int number) {
  tarch::multicore::Lock lock(_semaphore);
  _numberPool.push(number);
}

void exahype2::EnclaveTaskNumberPool::reset(int size) {
  tarch::multicore::Lock lock(_semaphore);
  while (!_numberPool.empty()) {
    _numberPool.pop();
  }
  _counter.store(0, std::memory_order_relaxed);

  if (size > 0) {
    int start = _counter.fetch_add(size, std::memory_order_relaxed);
    for (int i = 0; i < size; ++i) {
      _numberPool.push(start + i);
    }
  }
}

int exahype2::EnclaveTaskNumberPool::sizeInPool() {
  tarch::multicore::Lock lock(_semaphore);
  return static_cast<int>(_numberPool.size());
}

int exahype2::EnclaveTaskNumberPool::sizeInGenerated() {
  return _counter.load(std::memory_order_relaxed);
}

int exahype2::EnclaveTaskNumberPool::sizeInUse() {
  return sizeInGenerated() - sizeInPool();
}

void exahype2::EnclaveTaskNumberPool::refillPool() {
  tarch::multicore::Lock lock(_semaphore);
  // Double-check that no other thread refilled in the meantime
  if (!_numberPool.empty()) return;
  int start = _counter.fetch_add(_refillSize, std::memory_order_relaxed);
  for (int i = 0; i < _refillSize; ++i) {
    _numberPool.push(start + i);
  }
}
