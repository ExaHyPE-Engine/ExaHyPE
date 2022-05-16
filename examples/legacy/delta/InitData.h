// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAMPLES_DELTA_INIT_DATA_H_
#define _EXAMPLES_DELTA_INIT_DATA_H_


#include "Mapping.h"

#include "tarch/logging/Log.h"

#if Dimensions==3
#include "delta/primitives/Cylinder.h"
#endif


namespace examples {
  namespace delta {
    class InitData;
  }
}


/**
 * Observer which pipes the automaton transitions into a Peano patch file
 */
class examples::delta::InitData: public examples::delta::Mapping {
  private:
    static tarch::logging::Log _log;

    /**
     * We also provide a fake Delta 2d code which we use for debugging.
     * Usually, one would never translate any user code with Delta with
     * 2d, so this ifdef is obsolete.
     */
    #if Dimensions==3
    ::delta::Mesh*             _primitive;
    #endif
  public:
    /**
     * You have to invoke startNewSnapshot() if you wanna have a pvd file
     * immediately after you've created this observer in the main code.
     *
     * If this guy is ran on the global master,
     */
    InitData();
    virtual ~InitData();

	void beginTraversal() final;
	void endTraversal() final;

  	void createCell(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      CellData&                                    data,
      CellData&                                    coarseData
    ) final;

    void destroyCell(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      CellData&                                    data,
      CellData&                                    coarseData
    ) final;

    void touchCellFirstTime(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      CellData&                                    data,
      CellData&                                    coarseData
    ) final;

    void touchCellLastTime(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      CellData&                                    data,
      CellData&                                    coarseData
    ) final;
};

#endif
