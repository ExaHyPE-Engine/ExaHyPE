// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <string>


namespace peano4 {
  namespace grid {
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
    enum class LoadStoreComputeFlag {
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
     *
     * @see peano4.output.Observer for the usage of this routine within an
     *    observer
     * @see LoadStoreComputeFlag for an overview
     * @see constructLoadStoreComputeFlag() for a discussion of the data flow
     *    semantics
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
     * @see peano4::stacks::STDVectorStackOverSmartPointers Study the
     *   documentation of the constructor with the ObjectConstruction::NoData
     *   argument in detail.
     * @see peano4.output.Observer for the usage of this routine within an
     *    observer
     * @see LoadStoreComputeFlag for an overview
     * @see constructLoadStoreComputeFlag() for a discussion of the data flow
     *    semantics
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
     *
     *
     * predicateToUseData | predicateForLoad | predicateForStore |  Result        | Description
     * -------------------|------------------|-------------------|----------------|-------------
     * False              | False            | False             | NoData         | Just ignore the data, as it is not used
     * False              | True             | False             | LoadFromInputStream_ProvideToCalculations_Discard | Load it (and keep it as you've loaded it anyway) but then throw it away, as it is not used
     * False              | False            | True              | CreateDummy_ProvideToCalculations_StoreToOutputStream | It will not be used, but create a dummy, as it has to be stored eventually
     * False              | True             | True              | assertion      | Makes no sense
     * True               | False            | False             | CreateDummy_ProvideToCalculations_Discard | Data is required temporarily throughout calculations but not persistent in-between two grid sweeps
     * True               | True             | False             | LoadFromInputStream_ProvideToCalculations_Discard | Data flow-wisely the same as False, True, False
     * True               | False            | True              | CreateDummy_ProvideToCalculations_StoreToOutputStream | Introduce a new piece of data into the data flow. Same as false, false, true
     * True               | True             | True              | LoadFromInputStream_ProvideToCalculations_StoreToOutputStream | Proper piece of data that's held persistently in-between two mesh sweeps
     *
     */
    LoadStoreComputeFlag constructLoadStoreComputeFlag(bool predicateToUseData, bool predicateForLoad, bool predicateForStore);
  }
}
