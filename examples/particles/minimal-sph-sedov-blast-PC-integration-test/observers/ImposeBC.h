// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _examples_particles_observers_ImposeBC_H_
#define _examples_particles_observers_ImposeBC_H_


#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"
#include "peano4/grid/GridControlEvent.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "peano4/grid/TraversalObserver.h"


#include "repositories/DataRepository.h"


#include "actions/ImposeBC.h"
#include "observers/ImposeBC2peano4_toolbox_particles_UpdateParticleGridAssociation1.h"
#include "observers/ImposeBC2peano4_toolbox_particles_ParticleParticleInteraction2.h"
#include "observers/ImposeBC2peano4_toolbox_particles_ParticleAMR3.h"
#include "observers/ImposeBC2peano4_toolbox_particles_ParticleTreeAnalysis4.h"



namespace examples {namespace particles {namespace observers {
  class ImposeBC;
}}}


class examples::particles::observers::ImposeBC: public peano4::grid::TraversalObserver {
  public:
    /**
     * Logging device
     */
    static tarch::logging::Log  _log;
  
    const int _spacetreeId;
    
    actions::ImposeBC   _actionSet0; 
    observers::ImposeBC2peano4_toolbox_particles_UpdateParticleGridAssociation1   _actionSet1; 
    observers::ImposeBC2peano4_toolbox_particles_ParticleParticleInteraction2   _actionSet2; 
    observers::ImposeBC2peano4_toolbox_particles_ParticleAMR3   _actionSet3; 
    observers::ImposeBC2peano4_toolbox_particles_ParticleTreeAnalysis4   _actionSet4; 

    
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
    ImposeBC(int spacetreeId = SpacetreePrototype);
    
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

    void receiveAndMergeVertex( int position, int fromStack, SendReceiveContext context, const peano4::grid::GridTraversalEvent& event)  override;
    void receiveAndMergeFace(   int position, int fromStack, SendReceiveContext context, const peano4::grid::GridTraversalEvent& event)  override;
    void receiveAndMergeCell(                 int fromStack, SendReceiveContext context, const peano4::grid::GridTraversalEvent& event) override;

    void deleteAllStacks() override;
};

#endif