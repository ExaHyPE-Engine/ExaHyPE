
#include "ImposeBC.h"
#include "repositories/DataRepository.h"

#include "peano4/grid/PeanoCurve.h"

#include "peano4/datamanagement/VertexEnumerator.h"
#include "peano4/datamanagement/VertexMarker.h"
#include "peano4/datamanagement/FaceEnumerator.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "peano4/datamanagement/CellMarker.h"

#include "peano4/parallel/SpacetreeSet.h"


tarch::logging::Log examples::particles::observers::ImposeBC::_log( "examples::particles::observers::ImposeBC" );



examples::particles::observers::ImposeBC::ImposeBC(int spacetreeId):
  _spacetreeId( spacetreeId ) , _actionSet0(spacetreeId), _actionSet1(spacetreeId), _actionSet2(spacetreeId), _actionSet3(spacetreeId), _actionSet4(spacetreeId)
{
}


  

peano4::grid::TraversalObserver* examples::particles::observers::ImposeBC::clone(int spacetreeId) {
  return new ImposeBC(spacetreeId);
}

  

std::vector< peano4::grid::GridControlEvent > examples::particles::observers::ImposeBC::getGridControlEvents() const {
  std::vector< peano4::grid::GridControlEvent > result;

  {
    const std::vector< peano4::grid::GridControlEvent > actionResult = _actionSet0.getGridControlEvents();
    result.insert(result.begin(),actionResult.begin(),actionResult.end());
  }

  {
    const std::vector< peano4::grid::GridControlEvent > actionResult = _actionSet1.getGridControlEvents();
    result.insert(result.begin(),actionResult.begin(),actionResult.end());
  }

  {
    const std::vector< peano4::grid::GridControlEvent > actionResult = _actionSet2.getGridControlEvents();
    result.insert(result.begin(),actionResult.begin(),actionResult.end());
  }

  {
    const std::vector< peano4::grid::GridControlEvent > actionResult = _actionSet3.getGridControlEvents();
    result.insert(result.begin(),actionResult.begin(),actionResult.end());
  }

  {
    const std::vector< peano4::grid::GridControlEvent > actionResult = _actionSet4.getGridControlEvents();
    result.insert(result.begin(),actionResult.begin(),actionResult.end());
  }

  return result;
}



void examples::particles::observers::ImposeBC::beginTraversal( const tarch::la::Vector<Dimensions,double>&  x, const tarch::la::Vector<Dimensions,double>&  h ) {
  logTraceInWith2Arguments( "beginTraversal(...)", x, h );
  //
  // Invoke beginTraversal() on the actions
  //
  _actionSet0.beginTraversal();
  _actionSet1.beginTraversal();
  _actionSet2.beginTraversal();
  _actionSet3.beginTraversal();
  _actionSet4.beginTraversal();


  //
  // Fill call stacks with dummies which represent level -1 such that we can
  // call standard action routines on level 0 with parents. Without these
  // statements, a top(1) call would raise an assertion.
  //
  repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( examples::particles::celldata::ParticleSetCellStatistics() );
  for (int i=0; i<TwoPowerD; i++) {
    repositories::DataRepository::_VertexDataParticleSetStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( examples::particles::vertexdata::ParticleSet() );
  }

  logTraceOutWith2Arguments( "beginTraversal(...)", x, h );
}

  

void examples::particles::observers::ImposeBC::endTraversal( const tarch::la::Vector<Dimensions,double>&  x, const tarch::la::Vector<Dimensions,double>&  h ) {
  logTraceInWith2Arguments( "endTraversal(...)", x, h );
  _actionSet0.endTraversal();
  _actionSet1.endTraversal();
  _actionSet2.endTraversal();
  _actionSet3.endTraversal();
  _actionSet4.endTraversal();

  repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();
  for (int i=0; i<TwoPowerD; i++) {
    repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();
  }

  logTraceOutWith2Arguments( "endTraversal(...)", x, h );
}

  
void examples::particles::observers::ImposeBC::loadCell( const peano4::grid::GridTraversalEvent&  event ) {
  logTraceInWith2Arguments( "loadCell(...)", _spacetreeId, event.toString() );

  // Load vertex ParticleSet
  {
    auto view = repositories::DataRepository::_VertexDataParticleSetStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( TwoPowerD );
    for (int i=0; i<TwoPowerD; i++) {
      int inVertexStack          = event.getVertexDataFrom(i);
      int outVertexStackPosition = event.getVertexDataTo(i);
      logDebug("loadCell(...)", "vertex stack " << inVertexStack << "->pos-" << outVertexStackPosition );

      peano4::datamanagement::VertexMarker  marker(event,outVertexStackPosition);

      examples::particles::vertexdata::ParticleSet data;
      if (
        inVertexStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        inVertexStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        inVertexStack!=peano4::grid::TraversalObserver::NoData
        and
        (not peano4::grid::PeanoCurve::isInOutStack(inVertexStack) or examples::particles::vertexdata::ParticleSet::loadPersistently(marker))
      ) {
        assertion4( not repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inVertexStack))->empty(), event.toString(), peano4::datamanagement::VertexMarker(event).toString(), _spacetreeId, inVertexStack);
        data = repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inVertexStack))->pop();
      }

      #if PeanoDebug>0
      if (
        inVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        or
        inVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        or
        inVertexStack==peano4::grid::TraversalObserver::NoData
        or
        not examples::particles::vertexdata::ParticleSet::loadPersistently(marker)
      ) {
        logDebug( "loadCell(...)", "initialise meta data of vertex ParticleSet with " << marker.x(outVertexStackPosition) << " x " << marker.h() << ": load persistent data=" << examples::particles::vertexdata::ParticleSet::loadPersistently(marker) << " (" << marker.toString() << ")" );
        data.setDebugX( marker.x(outVertexStackPosition) );
        data.setDebugH( marker.h() );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inVertexStack)
      ) {
        assertionVectorNumericalEquals7( data.getDebugX(), marker.x(outVertexStackPosition), event.toString(), data.getDebugX(), marker.toString(), inVertexStack, i, outVertexStackPosition, _spacetreeId );
        assertionVectorNumericalEquals6( data.getDebugH(), marker.h(),                       event.toString(), data.getDebugX(), marker.toString(), inVertexStack, i, _spacetreeId );
      }
      #endif

      view.set(outVertexStackPosition,data);
    }
  }

  // Load cell ParticleSetCellStatistics
  {
    auto view = repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( 1 );

    peano4::datamanagement::CellMarker  marker(event);

    const int inCellStack  = event.getCellData();
    const int outCellStack = peano4::grid::PeanoCurve::CallStack;
    logDebug("loadCell(...)", "cell stack " << inCellStack << "->pos-" << outCellStack << "(" << examples::particles::celldata::ParticleSetCellStatistics::loadPersistently(marker) << ")" );

    examples::particles::celldata::ParticleSetCellStatistics& data = view.get(0);
    if (
      inCellStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      inCellStack!=peano4::grid::TraversalObserver::NoData
      and
      (not peano4::grid::PeanoCurve::isInOutStack(inCellStack) or examples::particles::celldata::ParticleSetCellStatistics::loadPersistently(marker))
    ) {
      assertion3( not repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inCellStack))->empty(), event.toString(), _spacetreeId, inCellStack);
      data = repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inCellStack))->pop();
    }

    #if PeanoDebug>0
    if (
      inCellStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      or
      inCellStack==peano4::grid::TraversalObserver::NoData
      or
      not examples::particles::celldata::ParticleSetCellStatistics::loadPersistently(marker)
    ) {
      logDebug( "loadCell(...)", "initialise meta data of cell ParticleSetCellStatistics with " << marker.x() << " x " << marker.h()  << " (" << marker.toString() << ")");
      data.setDebugX( marker.x() );
      data.setDebugH( marker.h() );
    }
    else {
      assertionVectorNumericalEquals4( data.getDebugX(), marker.x(), data.getDebugX(), data.getDebugH(), marker.toString(), _spacetreeId );
      assertionVectorNumericalEquals4( data.getDebugH(), marker.h(), data.getDebugX(), data.getDebugH(), marker.toString(), _spacetreeId );
    }
    #endif
  }

  logTraceOutWith1Argument( "loadCell(...)", _spacetreeId );
}

void examples::particles::observers::ImposeBC::enterCell( const peano4::grid::GridTraversalEvent&  event ) {
  logTraceInWith2Arguments( "enterCell(...)", _spacetreeId, event.toString() );

  // Handle vertex ParticleSetCellStatistics
  {
    peano4::datamanagement::VertexMarker  marker(event);

    for (int i=0; i<TwoPowerD; i++) {
      int inVertexStack  = event.getVertexDataFrom(i);
      int pick          = event.getVertexDataTo(i);   // the vertex position

      marker.select(pick);

      if (
        inVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        // Take care about the coarse grid accesses: Faces and cells are not yet loaded.
        // Therefore we don't use the usual shift of @f$ 2 \cdot 2d @f$ or @f$ 2 \cdot 2^d @f$
        // but only half of it.
        _actionSet0.createPersistentVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet0.touchVertexFirstTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        inVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet0.createHangingVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inVertexStack)
        and
        marker.isLocal()
      ) {
        _actionSet0.touchVertexFirstTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
    }
  }

  // Handle vertex ParticleSetCellStatistics
  {
    peano4::datamanagement::VertexMarker  marker(event);

    for (int i=0; i<TwoPowerD; i++) {
      int inVertexStack  = event.getVertexDataFrom(i);
      int pick          = event.getVertexDataTo(i);   // the vertex position

      marker.select(pick);

      if (
        inVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        // Take care about the coarse grid accesses: Faces and cells are not yet loaded.
        // Therefore we don't use the usual shift of @f$ 2 \cdot 2d @f$ or @f$ 2 \cdot 2^d @f$
        // but only half of it.
        _actionSet1.createPersistentVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet1.touchVertexFirstTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        inVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet1.createHangingVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inVertexStack)
        and
        marker.isLocal()
      ) {
        _actionSet1.touchVertexFirstTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
    }
  }

  // Handle vertex ParticleSetCellStatistics
  {
    peano4::datamanagement::VertexMarker  marker(event);

    for (int i=0; i<TwoPowerD; i++) {
      int inVertexStack  = event.getVertexDataFrom(i);
      int pick          = event.getVertexDataTo(i);   // the vertex position

      marker.select(pick);

      if (
        inVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        // Take care about the coarse grid accesses: Faces and cells are not yet loaded.
        // Therefore we don't use the usual shift of @f$ 2 \cdot 2d @f$ or @f$ 2 \cdot 2^d @f$
        // but only half of it.
        _actionSet2.createPersistentVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet2.touchVertexFirstTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        inVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet2.createHangingVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inVertexStack)
        and
        marker.isLocal()
      ) {
        _actionSet2.touchVertexFirstTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
    }
  }

  // Handle vertex ParticleSetCellStatistics
  {
    peano4::datamanagement::VertexMarker  marker(event);

    for (int i=0; i<TwoPowerD; i++) {
      int inVertexStack  = event.getVertexDataFrom(i);
      int pick          = event.getVertexDataTo(i);   // the vertex position

      marker.select(pick);

      if (
        inVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        // Take care about the coarse grid accesses: Faces and cells are not yet loaded.
        // Therefore we don't use the usual shift of @f$ 2 \cdot 2d @f$ or @f$ 2 \cdot 2^d @f$
        // but only half of it.
        _actionSet3.createPersistentVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet3.touchVertexFirstTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        inVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet3.createHangingVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inVertexStack)
        and
        marker.isLocal()
      ) {
        _actionSet3.touchVertexFirstTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
    }
  }

  // Handle vertex ParticleSetCellStatistics
  {
    peano4::datamanagement::VertexMarker  marker(event);

    for (int i=0; i<TwoPowerD; i++) {
      int inVertexStack  = event.getVertexDataFrom(i);
      int pick          = event.getVertexDataTo(i);   // the vertex position

      marker.select(pick);

      if (
        inVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        // Take care about the coarse grid accesses: Faces and cells are not yet loaded.
        // Therefore we don't use the usual shift of @f$ 2 \cdot 2d @f$ or @f$ 2 \cdot 2^d @f$
        // but only half of it.
        _actionSet4.createPersistentVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet4.touchVertexFirstTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        inVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet4.createHangingVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inVertexStack)
        and
        marker.isLocal()
      ) {
        _actionSet4.touchVertexFirstTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
    }
  }

  // Invoke creational events on cell ParticleSetCellStatistics
  {
    peano4::datamanagement::CellMarker marker( event );
    if (
      event.getCellData()==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      marker.isLocal()
    ) {
      _actionSet0.createCell(
        marker,
        peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) ),
        
        repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) )
        
        ,repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }

  // Invoke creational events on cell ParticleSetCellStatistics
  {
    peano4::datamanagement::CellMarker marker( event );
    if (
      event.getCellData()==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      marker.isLocal()
    ) {
      _actionSet1.createCell(
        marker,
        peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) ),
        
        repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) )
        
        ,repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }

  // Invoke creational events on cell ParticleSetCellStatistics
  {
    peano4::datamanagement::CellMarker marker( event );
    if (
      event.getCellData()==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      marker.isLocal()
    ) {
      _actionSet2.createCell(
        marker,
        peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) ),
        
        repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) )
        
        ,repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }

  // Invoke creational events on cell ParticleSetCellStatistics
  {
    peano4::datamanagement::CellMarker marker( event );
    if (
      event.getCellData()==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      marker.isLocal()
    ) {
      _actionSet3.createCell(
        marker,
        peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) ),
        
        repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) )
        
        ,repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }

  // Invoke creational events on cell ParticleSetCellStatistics
  {
    peano4::datamanagement::CellMarker marker( event );
    if (
      event.getCellData()==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      marker.isLocal()
    ) {
      _actionSet4.createCell(
        marker,
        peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) ),
        
        repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) )
        
        ,repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }

  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet0.touchCellFirstTime(
         marker
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }

  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet1.touchCellFirstTime(
         marker
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }

  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet2.touchCellFirstTime(
         marker
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }

  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet3.touchCellFirstTime(
         marker
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }

  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet4.touchCellFirstTime(
         marker
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }

  logTraceOut( "enterCell(...)" );
}

void examples::particles::observers::ImposeBC::leaveCell( const peano4::grid::GridTraversalEvent&  event ) {
  logTraceInWith2Arguments( "leaveCell(...)", _spacetreeId, event.toString() );

  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet4.touchCellLastTime(
         marker
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }

  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet3.touchCellLastTime(
         marker
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }

  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet2.touchCellLastTime(
         marker
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }

  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet1.touchCellLastTime(
         marker
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }

  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet0.touchCellLastTime(
         marker
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }

  {
    peano4::datamanagement::CellMarker marker( event );
    if (
      event.getCellData()==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      marker.isLocal()
    ) {
    _actionSet4.destroyCell(
      marker
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )
        ,repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }

  {
    peano4::datamanagement::CellMarker marker( event );
    if (
      event.getCellData()==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      marker.isLocal()
    ) {
    _actionSet3.destroyCell(
      marker
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )
        ,repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }

  {
    peano4::datamanagement::CellMarker marker( event );
    if (
      event.getCellData()==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      marker.isLocal()
    ) {
    _actionSet2.destroyCell(
      marker
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )
        ,repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }

  {
    peano4::datamanagement::CellMarker marker( event );
    if (
      event.getCellData()==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      marker.isLocal()
    ) {
    _actionSet1.destroyCell(
      marker
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )
        ,repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }

  {
    peano4::datamanagement::CellMarker marker( event );
    if (
      event.getCellData()==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      marker.isLocal()
    ) {
    _actionSet0.destroyCell(
      marker
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )
        ,repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }

  // Handle vertex ParticleSetCellStatistics
  {
    peano4::datamanagement::VertexMarker  marker(event);

    for (int i=0; i<TwoPowerD; i++) {
      int outVertexStack        = event.getVertexDataTo(i);
      int pick                  = event.getVertexDataFrom(i);

      marker.select(pick);

      if (
        outVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet4.touchVertexLastTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet4.destroyPersistentVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        outVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet4.destroyHangingVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outVertexStack)
        and
        marker.isLocal()
      ) {
        _actionSet4.touchVertexLastTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
    }
  }

  // Handle vertex ParticleSetCellStatistics
  {
    peano4::datamanagement::VertexMarker  marker(event);

    for (int i=0; i<TwoPowerD; i++) {
      int outVertexStack        = event.getVertexDataTo(i);
      int pick                  = event.getVertexDataFrom(i);

      marker.select(pick);

      if (
        outVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet3.touchVertexLastTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet3.destroyPersistentVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        outVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet3.destroyHangingVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outVertexStack)
        and
        marker.isLocal()
      ) {
        _actionSet3.touchVertexLastTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
    }
  }

  // Handle vertex ParticleSetCellStatistics
  {
    peano4::datamanagement::VertexMarker  marker(event);

    for (int i=0; i<TwoPowerD; i++) {
      int outVertexStack        = event.getVertexDataTo(i);
      int pick                  = event.getVertexDataFrom(i);

      marker.select(pick);

      if (
        outVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet2.touchVertexLastTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet2.destroyPersistentVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        outVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet2.destroyHangingVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outVertexStack)
        and
        marker.isLocal()
      ) {
        _actionSet2.touchVertexLastTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
    }
  }

  // Handle vertex ParticleSetCellStatistics
  {
    peano4::datamanagement::VertexMarker  marker(event);

    for (int i=0; i<TwoPowerD; i++) {
      int outVertexStack        = event.getVertexDataTo(i);
      int pick                  = event.getVertexDataFrom(i);

      marker.select(pick);

      if (
        outVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet1.touchVertexLastTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet1.destroyPersistentVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        outVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet1.destroyHangingVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outVertexStack)
        and
        marker.isLocal()
      ) {
        _actionSet1.touchVertexLastTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
    }
  }

  // Handle vertex ParticleSetCellStatistics
  {
    peano4::datamanagement::VertexMarker  marker(event);

    for (int i=0; i<TwoPowerD; i++) {
      int outVertexStack        = event.getVertexDataTo(i);
      int pick                  = event.getVertexDataFrom(i);

      marker.select(pick);

      if (
        outVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet0.touchVertexLastTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet0.destroyPersistentVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        outVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet0.destroyHangingVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outVertexStack)
        and
        marker.isLocal()
      ) {
        _actionSet0.touchVertexLastTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet>( &repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
    }
  }

  logTraceOutWith1Argument( "leaveCell(...)", _spacetreeId );
}

void examples::particles::observers::ImposeBC::storeCell( const peano4::grid::GridTraversalEvent&  event ) {
  logTraceInWith2Arguments( "storeCell(...)", _spacetreeId, event.toString() );

  // Handle cell ParticleSetCellStatistics
  {
    const int inCellStack   = peano4::grid::PeanoCurve::CallStack;
    const int outCellStack  = event.getCellData();
    logDebug("storeCell(...)", "cell stack " << inCellStack << "->pos-" << outCellStack );

    peano4::datamanagement::CellMarker  marker(event);

    auto view = repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( 1 );

    if (
      outCellStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      outCellStack!=peano4::grid::TraversalObserver::NoData
      and
      (not peano4::grid::PeanoCurve::isInOutStack(outCellStack) or examples::particles::celldata::ParticleSetCellStatistics::storePersistently(marker))
    ) {
      repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,outCellStack))->push( view.get(0) );
    }
    else {
      logDebug( 
        "storeCell(...)", 
        "do not store cell ParticleSetCellStatistics with " << marker.x() << " x " << marker.h() << ":" << 
        "  destroy=" << (outCellStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity) <<
        ", no-data=" << (outCellStack==peano4::grid::TraversalObserver::NoData) <<
        ", is-in/out=" << peano4::grid::PeanoCurve::isInOutStack(outCellStack) <<
        ", store-persistently=" << examples::particles::celldata::ParticleSetCellStatistics::storePersistently(marker)
      );
    }
  }

  // Store vertex ParticleSet
  {
    auto view = repositories::DataRepository::_VertexDataParticleSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( TwoPowerD );
    for (int i=0; i<TwoPowerD; i++) {
      int inVertexStackPosition  = event.getVertexDataFrom(i);
      int outVertexStack         = event.getVertexDataTo(i);
      logDebug("storeCell(...)", "pos-" << inVertexStackPosition << "->vertex stack " << outVertexStack);

      peano4::datamanagement::VertexMarker  marker(event,inVertexStackPosition);

      examples::particles::vertexdata::ParticleSet data = view.get(inVertexStackPosition);

      if (
        outVertexStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        outVertexStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        outVertexStack!=peano4::grid::TraversalObserver::NoData
        and
        (not peano4::grid::PeanoCurve::isInOutStack(outVertexStack) or examples::particles::vertexdata::ParticleSet::storePersistently(marker))
      ) {
        logDebug( "storeCell(...)", "store vertex ParticleSet with " << marker.x(inVertexStackPosition) << " x " << marker.h() );
        repositories::DataRepository::_VertexDataParticleSetStack.getForPush(repositories::DataRepository::DataKey(_spacetreeId,outVertexStack))->push(data);
      }
      else {
        logDebug( "storeCell(...)", "do not store vertex ParticleSet with " << marker.x(inVertexStackPosition) << " x " << marker.h() );
      }
    }
  }

  logTraceOutWith1Argument( "storeCell(...)", _spacetreeId );
}

void examples::particles::observers::ImposeBC::exchangeAllVerticalDataExchangeStacks( int masterId ) {
  logTraceInWith2Arguments( "exchangeAllVerticalDataExchangeStacks(...)", masterId, _spacetreeId  );

  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(
    repositories::DataRepository::_CellDataParticleSetCellStatisticsStack,
    _spacetreeId,
    masterId
  );

  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(
    repositories::DataRepository::_VertexDataParticleSetStack,
    _spacetreeId,
    masterId
  );

  logTraceOut( "exchangeAllVerticalDataExchangeStacks(...)" );
}

void examples::particles::observers::ImposeBC::exchangeAllHorizontalDataExchangeStacks( bool symmetricDataCardinality ) {
  logTraceInWith2Arguments( "exchangeAllHorizontalDataExchangeStacks(...)", symmetricDataCardinality, _spacetreeId  );

  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(
    repositories::DataRepository::_CellDataParticleSetCellStatisticsStack,
    _spacetreeId,
    symmetricDataCardinality
  );

  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(
    repositories::DataRepository::_VertexDataParticleSetStack,
    _spacetreeId,
    symmetricDataCardinality
  );

  logTraceOut( "exchangeAllHorizontalDataExchangeStacks(...)" );
}

void examples::particles::observers::ImposeBC::exchangeAllPeriodicBoundaryDataStacks() {
  logTraceInWith1Argument( "exchangeAllPeriodicBoundaryDataStacks()", _spacetreeId  );

  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(
    repositories::DataRepository::_CellDataParticleSetCellStatisticsStack,
    _spacetreeId
  );

  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(
    repositories::DataRepository::_VertexDataParticleSetStack,
    _spacetreeId
  );

  logTraceOut( "exchangeAllPeriodicBoundaryDataStacks()" );
}

void examples::particles::observers::ImposeBC::streamDataFromSplittingTreeToNewTree(int newWorker) {
  logTraceInWith2Arguments( "streamDataFromSplittingTreeToNewTree(int)", _spacetreeId, newWorker );

  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(
    repositories::DataRepository::_CellDataParticleSetCellStatisticsStack,
    _spacetreeId,
    newWorker
  );

  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(
    repositories::DataRepository::_VertexDataParticleSetStack,
    _spacetreeId,
    newWorker
  );

  logTraceOut( "streamDataFromSplittingTreeToNewTree(int)");
}

void examples::particles::observers::ImposeBC::streamDataFromJoiningTreeToMasterTree(int master) {
  logTraceInWith2Arguments( "streamDataFromJoiningTreeToMasterTree(int)", _spacetreeId, master );

  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(
    repositories::DataRepository::_CellDataParticleSetCellStatisticsStack,
    _spacetreeId,
    master
  );

  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(
    repositories::DataRepository::_VertexDataParticleSetStack,
    _spacetreeId,
    master
  );

  logTraceOut( "streamDataFromJoiningTreeToMasterTree(int)");
}

void examples::particles::observers::ImposeBC::finishAllOutstandingSendsAndReceives() {
  logTraceInWith1Argument( "finishAllOutstandingSendsAndReceives()", _spacetreeId );

  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(
    repositories::DataRepository::_CellDataParticleSetCellStatisticsStack,
    _spacetreeId
  );

  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(
    repositories::DataRepository::_VertexDataParticleSetStack,
    _spacetreeId
  );

  logTraceOut( "finishAllOutstandingSendsAndReceives()");
}

void examples::particles::observers::ImposeBC::sendVertex(int position, int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent&  event) {
  logTraceInWith4Arguments( "sendVertex(int,int,int)", position, toStack, event.toString(), _spacetreeId );
  

  {
    peano4::datamanagement::VertexMarker  marker(event,position);
    if (
      examples::particles::vertexdata::ParticleSet::storePersistently(marker) 
      and
      (context==::peano4::grid::TraversalObserver::SendReceiveContext::Rebalancing or examples::particles::vertexdata::ParticleSet::send(marker))
     ) {
      examples::particles::vertexdata::ParticleSet data = repositories::DataRepository::_VertexDataParticleSetStack.getForPop( 
        repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack)
      )->top(TwoPowerD-1-position);
    
      logDebug( "sendVertex(...)", "send out " << data.toString() << " to stack " << toStack << " on tree " << _spacetreeId << " for marker " << marker.toString() );

      repositories::DataRepository::_VertexDataParticleSetStack.getForPush(
        _spacetreeId, toStack
      ) -> push(data);
    }
  }

  logTraceOut( "sendVertex(int,int,int)");
}

void examples::particles::observers::ImposeBC::sendFace(int position, int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent&  event) {
  logTraceInWith4Arguments( "sendFace(int,int,int)", position, toStack, event.toString(), _spacetreeId );


  logTraceOut( "sendFace(int,int,int)");
}

void examples::particles::observers::ImposeBC::sendCell(int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent&  event) {
  logTraceInWith3Arguments( "sendCell(int,int,int)", toStack, event.toString(), _spacetreeId );
  

  {
    peano4::datamanagement::CellMarker  marker(event);
    if (
      examples::particles::celldata::ParticleSetCellStatistics::storePersistently(marker) 
      and
      (context==::peano4::grid::TraversalObserver::SendReceiveContext::Rebalancing or examples::particles::celldata::ParticleSetCellStatistics::send(marker))
     ) {
      examples::particles::celldata::ParticleSetCellStatistics data = repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top();
    
      logDebug( "sendCell(...)", "send out " << data.toString() << " to stack " << toStack << " on tree " << _spacetreeId );

      repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPush(
        _spacetreeId, toStack
      ) -> push(data);
    }
  }

  logTraceOut( "sendCell(int,int,int)");
}

void examples::particles::observers::ImposeBC::receiveAndMergeCell(int fromStack, SendReceiveContext context, const peano4::grid::GridTraversalEvent& event ) {
  logTraceInWith3Arguments( "receiveAndMergeCell(...)", fromStack, event.toString(), _spacetreeId );

  {
    peano4::datamanagement::CellMarker  marker(event);

    if ( 
      examples::particles::celldata::ParticleSetCellStatistics::loadPersistently(marker) 
      and
      (context==::peano4::grid::TraversalObserver::SendReceiveContext::Rebalancing or examples::particles::celldata::ParticleSetCellStatistics::receiveAndMerge(marker))
     ) {
      auto   incomingData = repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPop(
        _spacetreeId, fromStack
      )->pop();

      examples::particles::celldata::ParticleSetCellStatistics& data = repositories::DataRepository::_CellDataParticleSetCellStatisticsStack.getForPush( 
        repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack)
      )->top();

      logDebug( "receiveAndMergeCell(...)", "merge " << incomingData.toString() << " into " << data.toString() );

      if (context==::peano4::grid::TraversalObserver::SendReceiveContext::PeriodicBoundaryDataSwap) {
        assertion8(
          tarch::la::countEqualEntries(data.getDebugX(), incomingData.getDebugX())==Dimensions-1,
          data.getDebugX(), incomingData.getDebugX(),
          data.getDebugH(), incomingData.getDebugH(), 
          fromStack, event.toString(), marker.toString(), _spacetreeId );
        assertionVectorNumericalEquals6(
          data.getDebugH(), incomingData.getDebugH(),
          data.getDebugX(), incomingData.getDebugX(), fromStack, event.toString(), marker.toString(), _spacetreeId );
      }
      else {
        assertionVectorNumericalEquals6(
          data.getDebugX(), incomingData.getDebugX(),
          data.getDebugH(), incomingData.getDebugH(), fromStack, event.toString(), marker.toString(), _spacetreeId );
          assertionVectorNumericalEquals6(
          data.getDebugH(), incomingData.getDebugH(),
          data.getDebugX(), incomingData.getDebugX(), fromStack, event.toString(), marker.toString(), _spacetreeId );
      }

      data.merge(incomingData, marker);
    }
  }

  logTraceOut( "receiveAndMergeCell(...)");
}

void examples::particles::observers::ImposeBC::receiveAndMergeFace(int position, int fromStack, SendReceiveContext context, const peano4::grid::GridTraversalEvent& event) {
  logTraceInWith4Arguments( "receiveAndMergeFace(...)", position, fromStack, event.toString(), _spacetreeId );

  logTraceOut( "receiveAndMergeFace(...)");
}

void examples::particles::observers::ImposeBC::receiveAndMergeVertex(int position, int fromStack, SendReceiveContext context, const peano4::grid::GridTraversalEvent& event) {
  logTraceInWith4Arguments( "receiveAndMergeVertex(...)", position, fromStack, event.toString(), _spacetreeId );

  {
    peano4::datamanagement::VertexMarker  marker(event,position);

    if ( 
      examples::particles::vertexdata::ParticleSet::loadPersistently(marker) 
      and
      (context==::peano4::grid::TraversalObserver::SendReceiveContext::Rebalancing or examples::particles::vertexdata::ParticleSet::receiveAndMerge(marker))
     ) {
      auto   incomingData = repositories::DataRepository::_VertexDataParticleSetStack.getForPop(
        _spacetreeId, fromStack
      )->pop();

      examples::particles::vertexdata::ParticleSet& data = repositories::DataRepository::_VertexDataParticleSetStack.getForPush( 
        repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack)
      )->top(TwoPowerD-1-position);

      logDebug( "receiveAndMergeVertex(...)", "merge " << incomingData.toString() << " into " << data.toString() );

      if (context==::peano4::grid::TraversalObserver::SendReceiveContext::PeriodicBoundaryDataSwap) {
        assertion8(
          tarch::la::countEqualEntries(data.getDebugX(), incomingData.getDebugX())==Dimensions-1,
          data.getDebugX(), incomingData.getDebugX(),
          data.getDebugH(), incomingData.getDebugH(), 
          fromStack, event.toString(), marker.toString(), _spacetreeId );
        assertionVectorNumericalEquals6(
          data.getDebugH(), incomingData.getDebugH(),
          data.getDebugX(), incomingData.getDebugX(), fromStack, event.toString(), marker.toString(), _spacetreeId );
      }
      else {
        assertionVectorNumericalEquals6(
          data.getDebugX(), incomingData.getDebugX(),
          data.getDebugH(), incomingData.getDebugH(), fromStack, event.toString(), marker.toString(), _spacetreeId );
          assertionVectorNumericalEquals6(
          data.getDebugH(), incomingData.getDebugH(),
          data.getDebugX(), incomingData.getDebugX(), fromStack, event.toString(), marker.toString(), _spacetreeId );
      }

      data.merge(incomingData, marker);
    }
  }

  logTraceOut( "receiveAndMergeVertex(...)");
}

void examples::particles::observers::ImposeBC::deleteAllStacks() {
  logTraceInWith1Argument( "deleteAllStacks()", _spacetreeId );

  peano4::parallel::SpacetreeSet::deleteAllStacks(
    repositories::DataRepository::_CellDataParticleSetCellStatisticsStack,
    _spacetreeId
  );

  peano4::parallel::SpacetreeSet::deleteAllStacks(
    repositories::DataRepository::_VertexDataParticleSetStack,
    _spacetreeId
  );

  logTraceOut( "deleteAllStacks()");
}
