// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAMPLES_INTEGER_DIFFUSION_THROUGH_FACES_MYOBSERVER_H_
#define _EXAMPLES_INTEGER_DIFFUSION_THROUGH_FACES_MYOBSERVER_H_


#include "peano4/grid/TraversalObserver.h"
#include <map>

#include "../../peano4/stacks/STDVectorStack.h"
#include "Mapping.h"
#include "FaceData.h"
#include "CellData.h"


namespace examples {
  namespace integerdiffusionthroughfaces {
    class MyObserver;
  }
}


class examples::integerdiffusionthroughfaces::MyObserver: public peano4::grid::TraversalObserver {
  private:
	static tarch::logging::Log  _log;

	const int _spacetreeId;
	/**
	 * We use this counter to steer/hardcode dynamic AMR
	 */
	int _iterationCounter;

	/**
	 * To look up the right stack, we use a combination of tree number (id) and
	 * stack number.
	 */
	typedef std::pair<int,int>  DataKey;

	typedef peano4::stacks::STDVectorStack< FaceData >         FaceDataContainer;
	typedef peano4::stacks::STDVectorStack< CellData >         CellDataContainer;
	typedef peano4::stacks::STDVectorStack< Mapping::Faces >   FacesDataContainer;

	/**
	 * It is absolutely essential that these guys are static. They are global
	 * repositories shared by all observers.
	 */
	static std::map< DataKey, FaceDataContainer >   _faceData;
	static std::map< DataKey, CellDataContainer >   _cellData;

	/**
	 * All observers share one mapping, i.e. you have to make it thread-safe
	 */
	Mapping*                                _mapping;
	FacesDataContainer                      _facesCallStack;
  public:
    MyObserver();
    MyObserver(int spacetreeId, int counter, Mapping* mapping);
    ~MyObserver();

    void beginTraversal(
      const tarch::la::Vector<Dimensions,double>&  x,
      const tarch::la::Vector<Dimensions,double>&  h
    ) override;

    void endTraversal(
      const tarch::la::Vector<Dimensions,double>&  x,
      const tarch::la::Vector<Dimensions,double>&  h
    ) override;

    void enterCell(
      const peano4::grid::GridTraversalEvent&  event
    ) override;

    void leaveCell(
      const peano4::grid::GridTraversalEvent&  event
    ) override;

  	/**
  	 * I use the clone to create one observer object per traversal thread. So
  	 * between different spacetrees of one spacetree set, there can be no race
  	 * condition. Yet, the clone() itself could be called in parallel.
  	 */
  	TraversalObserver* clone(int spacetreeId) override;

    std::vector< peano4::grid::GridControlEvent > getGridControlEvents() override;
};

#endif
