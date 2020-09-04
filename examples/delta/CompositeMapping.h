// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAMPLES_DELTA_COMPOSITE_MAPPING_H_
#define _EXAMPLES_DELTA_COMPOSITE_MAPPING_H_


#include "Mapping.h"
#include <vector>

namespace examples {
  namespace delta {
    class CompositeMapping;
  }
}


/**
 * Observer which pipes the automaton transitions into a Peano patch file
 */
class examples::delta::CompositeMapping: public examples::delta::Mapping {
  private:
    std::vector<Mapping*> _mappings;
  public:
    /**
     * You have to invoke startNewSnapshot() if you wanna have a pvd file
     * immediately after you've created this observer in the main code.
     *
     * If this guy is ran on the global master,
     */
	CompositeMapping();
    virtual ~CompositeMapping();

	/**
	 * Append a new mapping. Ownership goes over to the composite.
	 */
	void append( Mapping* mapping );

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
