// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Termination.h"

namespace toolbox::blockstructured {
  template <typename T>
  class ChildDataStage;
} // namespace toolbox::blockstructured

/**
 * Collects the data of the children of one coarse cell or face until the sweep
 * reduces it.
 */
template <typename T>
class toolbox::blockstructured::ChildDataStage final {
public:
  ChildDataStage(int numberOfChildren, int valuesPerChild);

  ChildDataStage(const ChildDataStage&)            = delete;
  ChildDataStage(ChildDataStage&&)                 = delete;
  ChildDataStage& operator=(const ChildDataStage&) = delete;
  ChildDataStage& operator=(ChildDataStage&&)      = delete;
  ~ChildDataStage();

  INLINE_METHOD bool     stage(const void* coarseEntityKey, int childIndex, const T* childValues);
  INLINE_METHOD T*       claim(const void* coarseEntityKey, int childIndex, int frameTag);
  INLINE_METHOD bool     hasFrame(const void* coarseEntityKey) const;
  INLINE_METHOD bool     hasChild(const void* coarseEntityKey, int childIndex) const;
  INLINE_METHOD int      frameTag(const void* coarseEntityKey) const;
  INLINE_METHOD const T* child(const void* coarseEntityKey, int childIndex) const;
  INLINE_METHOD void     discard(const void* coarseEntityKey);

private:
  struct Frame {
    const void*    key{nullptr};
    std::uint64_t  filledChildren{0};
    int            tag{0};
    std::vector<T> values{};
  };

  INLINE_METHOD Frame*       find(const void* coarseEntityKey);
  INLINE_METHOD const Frame* find(const void* coarseEntityKey) const;

  const int          _numberOfChildren;
  const int          _valuesPerChild;
  std::vector<Frame> _frames{};
};

#include "toolbox/blockstructured/ChildDataStage.cpph"
