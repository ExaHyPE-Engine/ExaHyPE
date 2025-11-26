// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <cstdint>
#include <string>

namespace peano4::grid {
  /**
   * Flag to control data movements
   *
   * This flag is used to determine data movements. It has nothing to do with
   * which events are triggered at any point. It simply controls which data is moved
   * between the stacks by the tree traversal automaton. The routine
   * constructLoadStoreComputeFlag() provides some documentation of the
   * semantics of the variants, while loadPersistently(), storePersistently()
   * and computeOnData() are used to make decisions which data to copy/move
   * from stack to stack.
   */
  enum class LoadStoreComputeFlag : std::uint8_t {
    LoadFromInputStream_ProvideToCalculations_StoreToOutputStream,
    LoadFromInputStream_ProvideToCalculations_Discard,
    CreateDummy_ProvideToCalculations_StoreToOutputStream,
    CreateDummy_ProvideToCalculations_Discard,
    NoData
  };

  /**
   * Data is stored persistently on input/output stream
   *
   * These actions are derived from the storage annotation. They do not imply
   * in any way that events are called or are not called. That is, we we only
   * control data movements and initialisations.
   */
  bool loadPersistently(LoadStoreComputeFlag flag);

  /**
   * Data is stored persistently on input/output stream
   *
   * These actions are derived from the storage annotation. They do not imply
   * in any way that events are called or are not called. That is, we we only
   * control data movements and initialisations.
   *
   *
   * @see peano4.output.Observer for the usage of this routine within an
   *    observer
   * @see LoadStoreComputeFlag for an overview
   * @see constructLoadStoreComputeFlag() for a discussion of the data flow
   *    semantics
   */
  bool storePersistently(LoadStoreComputeFlag flag);

  /**
   * Data is stored persistently on input/output stream
   *
   * These actions are derived from the storage annotation. They do not imply
   * in any way that events are called or are not called. That is, we we only
   * control data movements and initialisations. In this particular case, we
   * use the predicate to find out if we have to initialise stack data once
   * we have created it (as it is hanging or brand new). We always set the
   * meta data, but some stack entries holds (smart) pointers and by default
   * point into the nirvana. They might have to be initialised explicitly by
   * using the default constructor.
   *
   */
  bool computeOnData(LoadStoreComputeFlag flag);

  std::string toString(LoadStoreComputeFlag flag);

  /**
   * Constructs a data storage scheme
   *
   * Same as constructLoadStoreComputeFlag(bool,bool,bool), assuming that its
   * predicateToUseData holds all the time.
   */
  LoadStoreComputeFlag constructLoadStoreComputeFlag(bool predicateForLoad, bool predicateForStore);

  /**
   * Constructs a data storage scheme
   *
   * Same as the other constructor, assuming that its predicateToUseData
   * holds all the time. This routine is often used by user code to construct
   * the enum, i.e. it is a sole helper routine. Peano itself does not
   * directly use it.
   */
  LoadStoreComputeFlag constructLoadStoreComputeFlag(
    bool predicateToUseData,
    bool predicateForLoad,
    bool predicateForStore
  );
} // namespace peano4::grid
