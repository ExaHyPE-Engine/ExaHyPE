// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"

namespace toolbox::blockstructured {
  template <typename T>
  class ChildDataStage;
} // namespace toolbox::blockstructured

/**
 * Collects the patches of a destroyed sibling set until the set is complete.
 */
template <typename T>
class toolbox::blockstructured::ChildDataStage final {
public:
  ChildDataStage(int numberOfChildren, int valuesPerChild);
  ~ChildDataStage();
  ChildDataStage(const ChildDataStage&)            = delete;
  ChildDataStage(ChildDataStage&&)                 = delete;
  ChildDataStage& operator=(const ChildDataStage&) = delete;
  ChildDataStage& operator=(ChildDataStage&&)      = delete;

  INLINE_METHOD bool     stage(const void* coarseCellKey, int childIndex, const T* const childValues);
  INLINE_METHOD bool     hasFrame(const void* coarseCellKey) const;
  INLINE_METHOD bool     hasChild(const void* coarseCellKey, int childIndex) const;
  INLINE_METHOD const T* child(const void* coarseCellKey, int childIndex) const;
  INLINE_METHOD void     discard(const void* coarseCellKey);

private:
  struct Frame {
    const void*    key{nullptr};
    std::uint64_t  filledChildren{0};
    std::vector<T> values;
  };

  INLINE_METHOD Frame*       find(const void* coarseCellKey);
  INLINE_METHOD const Frame* find(const void* coarseCellKey) const;

  int                _numberOfChildren;
  int                _valuesPerChild;
  std::vector<Frame> _frames;
};

#include "toolbox/blockstructured/ChildDataStage.cpph"
