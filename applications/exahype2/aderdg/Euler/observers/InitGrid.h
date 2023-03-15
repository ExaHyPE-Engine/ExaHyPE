// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"
#include "peano4/grid/GridControlEvent.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "peano4/grid/TraversalObserver.h"


#include "repositories/DataRepository.h"


#include "observers/InitGrid2exahype2_solvers_aderdg_actionsets_InitialCondition0.h"
#include "observers/InitGrid2exahype2_grid_FaceLabel1.h"
#include "observers/InitGrid2exahype2_grid_CellLabel2.h"
#include "observers/InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3.h"
#include "observers/InitGrid2exahype2_solvers_aderdg_actionsets_PostprocessSolution4.h"



namespace examples {namespace exahype2 {namespace euler2D {namespace observers {
  class InitGrid;
}}}}


class examples::exahype2::euler2D::observers::InitGrid: public peano4::grid::TraversalObserver {
  public:
    /**
     * Logging device
     */
    static tarch::logging::Log  _log;
  
    const int _spacetreeId;
    
    observers::InitGrid2exahype2_solvers_aderdg_actionsets_InitialCondition0   _actionSet0; 
    observers::InitGrid2exahype2_grid_FaceLabel1   _actionSet1; 
    observers::InitGrid2exahype2_grid_CellLabel2   _actionSet2; 
    observers::InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3   _actionSet3; 
    observers::InitGrid2exahype2_solvers_aderdg_actionsets_PostprocessSolution4   _actionSet4; 

    
  public:
    /**
     * If you pass this marker into the constructor, it implies that this 
     * observer is used as prototype for the real observers which are created
     * per local spacetree.
     */
    static constexpr int SpacetreePrototype = -1; 
  
    /**
     * @param spacetreeId -1 is the dummy, i.e. this observer is not 
     *                    instantiated for a spacetree yet but serves as 
     *                    blueprint for the real trees later on.
     */
    InitGrid(int spacetreeId = SpacetreePrototype);
    
    void loadCell(
     const peano4::grid::GridTraversalEvent&  event
    ) override;

    void enterCell(
     const peano4::grid::GridTraversalEvent&  event
    ) override;

    void leaveCell(
      const peano4::grid::GridTraversalEvent&  event
    ) override;

    void storeCell(
      const peano4::grid::GridTraversalEvent&  event
    ) override;

    TraversalObserver* clone(int spacetreeId) override;

    std::vector< peano4::grid::GridControlEvent > getGridControlEvents() const override;

    /**
     * This routine is not called by the spacetree set or the spacetree. It is
     * up to the user to invoke it.
     */
    static void prepareTraversal();
    static void unprepareTraversal();

    void beginTraversal(
      const tarch::la::Vector<Dimensions,double>&  x,
      const tarch::la::Vector<Dimensions,double>&  h
    ) override;

    void endTraversal(
      const tarch::la::Vector<Dimensions,double>&  x,
      const tarch::la::Vector<Dimensions,double>&  h
    ) override;
    
    void exchangeAllVerticalDataExchangeStacks( int masterId ) override;
    void exchangeAllHorizontalDataExchangeStacks( bool symmetricDataCardinality ) override;
    void exchangeAllPeriodicBoundaryDataStacks() override;
    
    void streamDataFromSplittingTreeToNewTree( int newWorker ) override;
    void streamDataFromJoiningTreeToMasterTree( int masterId ) override;
    
    void finishAllOutstandingSendsAndReceives() override;
    
    void sendVertex(int position, int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent&  event) override;
    void sendFace(  int position, int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent&  event) override;
    void sendCell(                int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent&  event) override;

    void receiveAndMergeVertex( int position, int fromStack, SendReceiveContext context, const peano4::grid::GridTraversalEvent& event) override;
    void receiveAndMergeFace(   int position, int fromStack, SendReceiveContext context, const peano4::grid::GridTraversalEvent& event) override;
    void receiveAndMergeCell(                 int fromStack, SendReceiveContext context, const peano4::grid::GridTraversalEvent& event) override;

    void deleteAllStacks() override;
};
