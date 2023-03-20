// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"
#include "peano4/grid/GridControlEvent.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "peano4/grid/TraversalObserver.h"


#include "repositories/DataRepository.h"


#include "observers/TimeStep2exahype2_solvers_rkfd_SeparateSweepsWithEnclaveTasking_MergeEnclaveTaskOutcome0.h"
#include "observers/TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR1.h"
#include "observers/TimeStep2exahype2_solvers_rkfd_actionsets_RollOverUpdatedFace2.h"
#include "observers/TimeStep2exahype2_grid_FaceLabel3.h"
#include "observers/TimeStep2exahype2_grid_CellLabel4.h"
#include "observers/TimeStep2exahype2_solvers_rkfd_actionsets_HandleBoundary5.h"
#include "observers/TimeStep2exahype2_solvers_rkfd_SeparateSweepsWithEnclaveTasking_UpdateCell6.h"
#include "observers/TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7.h"
#include "observers/TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8.h"
#include "observers/TimeStep2exahype2_solvers_rkfd_actionsets_AdaptivityCriterion9.h"
#include "observers/TimeStep2exahype2_solvers_rkfd_actionsets_PostprocessSolution10.h"



namespace benchmarks {namespace exahype2 {namespace euler {namespace sphericalaccretionupscaling {namespace observers {
  class TimeStep;
}}}}}


class benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::TimeStep: public peano4::grid::TraversalObserver {
  public:
    /**
     * Logging device
     */
    static tarch::logging::Log  _log;
  
    const int _spacetreeId;
    
    observers::TimeStep2exahype2_solvers_rkfd_SeparateSweepsWithEnclaveTasking_MergeEnclaveTaskOutcome0   _actionSet0; 
    observers::TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR1   _actionSet1; 
    observers::TimeStep2exahype2_solvers_rkfd_actionsets_RollOverUpdatedFace2   _actionSet2; 
    observers::TimeStep2exahype2_grid_FaceLabel3   _actionSet3; 
    observers::TimeStep2exahype2_grid_CellLabel4   _actionSet4; 
    observers::TimeStep2exahype2_solvers_rkfd_actionsets_HandleBoundary5   _actionSet5; 
    observers::TimeStep2exahype2_solvers_rkfd_SeparateSweepsWithEnclaveTasking_UpdateCell6   _actionSet6; 
    observers::TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7   _actionSet7; 
    observers::TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8   _actionSet8; 
    observers::TimeStep2exahype2_solvers_rkfd_actionsets_AdaptivityCriterion9   _actionSet9; 
    observers::TimeStep2exahype2_solvers_rkfd_actionsets_PostprocessSolution10   _actionSet10; 

    
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
    TimeStep(int spacetreeId = SpacetreePrototype);
    
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
