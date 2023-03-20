
#include "StephydroPartReduceGlobalQuantities_sweep11.h"
#include "repositories/DataRepository.h"

#include "peano4/grid/PeanoCurve.h"

#include "peano4/datamanagement/VertexEnumerator.h"
#include "peano4/datamanagement/VertexMarker.h"
#include "peano4/datamanagement/FaceEnumerator.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "peano4/datamanagement/CellMarker.h"

#include "peano4/parallel/SpacetreeSet.h"


tarch::logging::Log benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::_log( "benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11" );


benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::StephydroPartReduceGlobalQuantities_sweep11(int spacetreeId):
  _spacetreeId( spacetreeId ) , _actionSet0(spacetreeId), _actionSet1(spacetreeId)
{}


  

peano4::grid::TraversalObserver* benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::clone(int spacetreeId) {
  return new StephydroPartReduceGlobalQuantities_sweep11(spacetreeId);
}

  

std::vector< peano4::grid::GridControlEvent > benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::getGridControlEvents() const {
  std::vector< peano4::grid::GridControlEvent > result;
  {
    const std::vector< peano4::grid::GridControlEvent > actionResult = _actionSet0.getGridControlEvents();
    result.insert(result.begin(),actionResult.begin(),actionResult.end());
  }
  {
    const std::vector< peano4::grid::GridControlEvent > actionResult = _actionSet1.getGridControlEvents();
    result.insert(result.begin(),actionResult.begin(),actionResult.end());
  }
  return result;
}



void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::beginTraversal( const tarch::la::Vector<Dimensions,double>&  x, const tarch::la::Vector<Dimensions,double>&  h ) {
  logTraceInWith2Arguments( "beginTraversal(...)", x, h );
  //
  // Invoke beginTraversal() on the actions
  //
  _actionSet0.beginTraversal();
  _actionSet1.beginTraversal();


  //
  // Fill call stacks with dummies which represent level 0 such that we can
  // call standard action routines on level 1 with parents. Without these
  // statements, a top(1) call would raise an assertion.
  //
  repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics() );
  for (int i=0; i<TwoPowerD; i++) {
    repositories::DataRepository::_VertexDatahydroPartSetStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( benchmarks::swift2::noh::vertexdata::hydroPartSet() );
  }

  logTraceOutWith2Arguments( "beginTraversal(...)", x, h );
}

  

void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::endTraversal( const tarch::la::Vector<Dimensions,double>&  x, const tarch::la::Vector<Dimensions,double>&  h ) {
  logTraceInWith2Arguments( "endTraversal(...)", x, h );
    _actionSet0.endTraversal();
  _actionSet1.endTraversal();

    repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();
  for (int i=0; i<TwoPowerD; i++) {
    repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();
  }

  logTraceOutWith2Arguments( "endTraversal(...)", x, h );
}

  

void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::prepareTraversal() {
  logTraceIn( "prepareTraversal(...)" );
    observers::StephydroPartReduceGlobalQuantities_sweep112swift2_actionsets_UpdateParticleMarker0::prepareTraversal();
  observers::StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1::prepareTraversal();

  logTraceOut( "prepareTraversal(...)" );
}

  

void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::unprepareTraversal() {
  logTraceIn( "unprepareTraversal(...)" );
    observers::StephydroPartReduceGlobalQuantities_sweep112swift2_actionsets_UpdateParticleMarker0::unprepareTraversal();
  observers::StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1::unprepareTraversal();

  logTraceOut( "unprepareTraversal(...)" );
}

  
void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::loadCell( const peano4::grid::GridTraversalEvent&  event ) {
  logTraceInWith2Arguments( "loadCell(...)", _spacetreeId, event.toString() );
  // Load vertex hydroPartSet
  {
    auto view = repositories::DataRepository::_VertexDatahydroPartSetStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( TwoPowerD );
    for (int i=0; i<TwoPowerD; i++) {
      int inVertexStack          = event.getVertexDataFrom(i);
      int outVertexStackPosition = event.getVertexDataTo(i);
      logDebug("loadCell(...)", "vertex stack " << inVertexStack << "->pos-" << outVertexStackPosition );

      peano4::datamanagement::VertexMarker  marker(event,outVertexStackPosition);

      if (
        inVertexStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        inVertexStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        inVertexStack!=peano4::grid::TraversalObserver::NoData
        and
        (not peano4::grid::PeanoCurve::isInOutStack(inVertexStack) or benchmarks::swift2::noh::vertexdata::hydroPartSet::loadPersistently(
          marker
          
        ))
      ) {
        assertion4( not repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inVertexStack))->empty(), event.toString(), peano4::datamanagement::VertexMarker(event).toString(), _spacetreeId, inVertexStack);
        benchmarks::swift2::noh::vertexdata::hydroPartSet data = std::move( repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inVertexStack))->pop() );
        #if PeanoDebug>0
        if ( peano4::grid::PeanoCurve::isInOutStack(inVertexStack) ) {
          assertionVectorNumericalEquals7( data.getDebugX(), marker.x(outVertexStackPosition), event.toString(), data.getDebugX(), marker.toString(), inVertexStack, i, outVertexStackPosition, _spacetreeId );
          assertionVectorNumericalEquals6( data.getDebugH(), marker.h(),                       event.toString(), data.getDebugX(), marker.toString(), inVertexStack, i, _spacetreeId );
        }
        #endif      
        view.set(outVertexStackPosition,data);
      }
      else {
        benchmarks::swift2::noh::vertexdata::hydroPartSet data;
        #if PeanoDebug>0
        logDebug( "loadCell(...)", "initialise meta data of vertex hydroPartSet with " << marker.x(outVertexStackPosition) << " x " << marker.h() << ": load persistent data=" << benchmarks::swift2::noh::vertexdata::hydroPartSet::loadPersistently(
          marker
          
        ) << " (" << marker.toString() << ")" );
        data.setDebugX( marker.x(outVertexStackPosition) );
        data.setDebugH( marker.h() );
        #endif      
        view.set(outVertexStackPosition,data);
      }
    }
  }
  // Load cell CellDatahydroPartSetCellStatistics
  {
    auto view = repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( 1 );

    peano4::datamanagement::CellMarker  marker(event);

    const int inCellStack  = event.getCellData();
    const int outCellStack = peano4::grid::PeanoCurve::CallStack;
    logDebug("loadCell(...)", "cell stack " << inCellStack << "->pos-" << outCellStack << "(" << benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::loadPersistently(
      marker
      
    ) << ")" );

    if (
      inCellStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      inCellStack!=peano4::grid::TraversalObserver::NoData
      and
      (not peano4::grid::PeanoCurve::isInOutStack(inCellStack) or benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::loadPersistently(
        marker
        
      ))
    ) {
      assertion3( not repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inCellStack))->empty(), event.toString(), _spacetreeId, inCellStack);
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics data = std::move( repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inCellStack))->pop() );
      
      #if PeanoDebug>0
      assertionVectorNumericalEquals4( data.getDebugX(), marker.x(), data.getDebugX(), data.getDebugH(), marker.toString(), _spacetreeId );
      assertionVectorNumericalEquals4( data.getDebugH(), marker.h(), data.getDebugX(), data.getDebugH(), marker.toString(), _spacetreeId );
      #endif
      
      view.set(0,data);
    }
    else {
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics data;
    
      #if PeanoDebug>0
      logDebug( "loadCell(...)", "initialise meta data of cell hydroPartSetCellStatistics with " << marker.x() << " x " << marker.h()  << " (" << marker.toString() << ")");
      data.setDebugX( marker.x() );
      data.setDebugH( marker.h() );
      #endif
      view.set(0,data);
    }
  }
  
  
  
  logTraceOutWith1Argument( "loadCell(...)", _spacetreeId );
}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::enterCell( const peano4::grid::GridTraversalEvent&  event ) {
  logTraceInWith2Arguments( "enterCell(...)", _spacetreeId, event.toString() );
  // Handle vertex 
  {
    peano4::datamanagement::VertexMarker  marker(event);

    for (int i=0; i<TwoPowerD; i++) {
      int inVertexStack  = event.getVertexDataFrom(i);
      int pick          = event.getVertexDataTo(i);   // the vertex position

      marker.select(pick);

      logDebug( "enterCell(...)", inVertexStack << "->" << pick << " (is-local=" << marker.isLocal() << ")" );
      
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
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet0.touchVertexFirstTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        inVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet0.createHangingVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inVertexStack)
        and
        marker.isLocal()
      ) {
        _actionSet0.touchVertexFirstTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
    }
  }
  // Handle vertex 
  {
    peano4::datamanagement::VertexMarker  marker(event);

    for (int i=0; i<TwoPowerD; i++) {
      int inVertexStack  = event.getVertexDataFrom(i);
      int pick          = event.getVertexDataTo(i);   // the vertex position

      marker.select(pick);

      logDebug( "enterCell(...)", inVertexStack << "->" << pick << " (is-local=" << marker.isLocal() << ")" );
      
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
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet1.touchVertexFirstTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        inVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet1.createHangingVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inVertexStack)
        and
        marker.isLocal()
      ) {
        _actionSet1.touchVertexFirstTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
    }
  }
  // Invoke creational events on cell 
  {
    peano4::datamanagement::CellMarker marker( event );
    if (
      event.getCellData()==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      marker.isLocal()
    ) {
      _actionSet0.createCell(
        marker,
        peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) ),
        
        repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) )
        
        ,repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  // Invoke creational events on cell 
  {
    peano4::datamanagement::CellMarker marker( event );
    if (
      event.getCellData()==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      marker.isLocal()
    ) {
      _actionSet1.createCell(
        marker,
        peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) ),
        
        repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) )
        
        ,repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet0.touchCellFirstTime(
         marker
        ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet1.touchCellFirstTime(
         marker
        ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  logTraceOut( "enterCell(...)" );
}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::leaveCell( const peano4::grid::GridTraversalEvent&  event ) {
  logTraceInWith2Arguments( "leaveCell(...)", _spacetreeId, event.toString() );
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet1.touchCellLastTime(
         marker
        ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet0.touchCellLastTime(
         marker
        ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )
        ,repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )
        ,repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  // Handle vertex 
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
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet1.destroyPersistentVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        outVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet1.destroyHangingVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outVertexStack)
        and
        marker.isLocal()
      ) {
        _actionSet1.touchVertexLastTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
    }
  }
  // Handle vertex 
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
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet0.destroyPersistentVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        outVertexStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet0.destroyHangingVertex(
           marker
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outVertexStack)
        and
        marker.isLocal()
      ) {
        _actionSet0.touchVertexLastTime(
           marker
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD-1) )(pick)
          ,peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet>( &repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoPowerD*2-1) ),repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
    }
  }
  logTraceOutWith1Argument( "leaveCell(...)", _spacetreeId );
}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::storeCell( const peano4::grid::GridTraversalEvent&  event ) {
  logTraceInWith2Arguments( "storeCell(...)", _spacetreeId, event.toString() );
  // Handle cell CellDatahydroPartSetCellStatistics
  {
    const int inCellStack   = peano4::grid::PeanoCurve::CallStack;
    const int outCellStack  = event.getCellData();
    logDebug("storeCell(...)", "cell stack " << inCellStack << "->pos-" << outCellStack );

    peano4::datamanagement::CellMarker  marker(event);

    auto view = repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( 1 );

    if (
      outCellStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      outCellStack!=peano4::grid::TraversalObserver::NoData
      and
      (not peano4::grid::PeanoCurve::isInOutStack(outCellStack) or view.get(0).storePersistently(
        marker
        
      ))
    ) {
      repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,outCellStack))->push( view.get(0) );
    }
    else {
      logDebug( 
        "storeCell(...)", 
        "do not store cell hydroPartSetCellStatistics with " << marker.x() << " x " << marker.h() << ":" << 
        "  destroy=" << (outCellStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity) <<
        ", no-data=" << (outCellStack==peano4::grid::TraversalObserver::NoData) <<
        ", is-in/out=" << peano4::grid::PeanoCurve::isInOutStack(outCellStack) <<
        ", store-persistently=" << view.get(0).storePersistently(
          
          marker
        )
      );
    }
  }
  // Store vertex VertexDatahydroPartSet
  {
    auto view = repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( TwoPowerD );
    for (int i=0; i<TwoPowerD; i++) {
      int inVertexStackPosition  = event.getVertexDataFrom(i);
      int outVertexStack         = event.getVertexDataTo(i);
      logDebug("storeCell(...)", "pos-" << inVertexStackPosition << "->vertex stack " << outVertexStack);

      peano4::datamanagement::VertexMarker  marker(event,inVertexStackPosition);

      benchmarks::swift2::noh::vertexdata::hydroPartSet& data = view.get(inVertexStackPosition);

      if (
        outVertexStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        outVertexStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        outVertexStack!=peano4::grid::TraversalObserver::NoData
        and
        (not peano4::grid::PeanoCurve::isInOutStack(outVertexStack) or data.storePersistently(
          marker
          
        ))
      ) {
        logDebug( "storeCell(...)", "store vertex hydroPartSet with " << marker.x(inVertexStackPosition) << " x " << marker.h() );
        repositories::DataRepository::_VertexDatahydroPartSetStack.getForPush(repositories::DataRepository::DataKey(_spacetreeId,outVertexStack))->push(data);
      }
      else {
        logDebug( "storeCell(...)", "do not store vertex hydroPartSet with " << marker.x(inVertexStackPosition) << " x " << marker.h() );
      }
    }
  }
  logTraceOutWith1Argument( "storeCell(...)", _spacetreeId );
}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::exchangeAllVerticalDataExchangeStacks( int masterId ) {
  logTraceInWith2Arguments( "exchangeAllVerticalDataExchangeStacks(...)", masterId, _spacetreeId  );
  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(
    repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack,
    _spacetreeId,
    masterId
  );
  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(
    repositories::DataRepository::_VertexDatahydroPartSetStack,
    _spacetreeId,
    masterId
  );
  logTraceOut( "exchangeAllVerticalDataExchangeStacks(...)" );
}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::exchangeAllHorizontalDataExchangeStacks( bool symmetricDataCardinality ) {
  logTraceInWith2Arguments( "exchangeAllHorizontalDataExchangeStacks(...)", symmetricDataCardinality, _spacetreeId  );
  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(
    repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack,
    _spacetreeId,
    symmetricDataCardinality
  );
  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(
    repositories::DataRepository::_VertexDatahydroPartSetStack,
    _spacetreeId,
    symmetricDataCardinality
  );
  logTraceOut( "exchangeAllHorizontalDataExchangeStacks(...)" );
}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::exchangeAllPeriodicBoundaryDataStacks() {
  logTraceInWith1Argument( "exchangeAllPeriodicBoundaryDataStacks()", _spacetreeId  );
  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(
    repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(
    repositories::DataRepository::_VertexDatahydroPartSetStack,
    _spacetreeId
  );
  logTraceOut( "exchangeAllPeriodicBoundaryDataStacks()" );
}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::streamDataFromSplittingTreeToNewTree(int newWorker) {
  logTraceInWith2Arguments( "streamDataFromSplittingTreeToNewTree(int)", _spacetreeId, newWorker );
  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(
    repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack,
    _spacetreeId,
    newWorker
  );
  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(
    repositories::DataRepository::_VertexDatahydroPartSetStack,
    _spacetreeId,
    newWorker
  );
  logTraceOut( "streamDataFromSplittingTreeToNewTree(int)");
}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::streamDataFromJoiningTreeToMasterTree(int master) {
  logTraceInWith2Arguments( "streamDataFromJoiningTreeToMasterTree(int)", _spacetreeId, master );
  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(
    repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack,
    _spacetreeId,
    master
  );
  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(
    repositories::DataRepository::_VertexDatahydroPartSetStack,
    _spacetreeId,
    master
  );
  logTraceOut( "streamDataFromJoiningTreeToMasterTree(int)");
}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::finishAllOutstandingSendsAndReceives() {
  logTraceInWith1Argument( "finishAllOutstandingSendsAndReceives()", _spacetreeId );
  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(
    repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(
    repositories::DataRepository::_VertexDatahydroPartSetStack,
    _spacetreeId
  );
  logTraceOut( "finishAllOutstandingSendsAndReceives()");
}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::sendVertex(int position, int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent&  event) {
  logTraceInWith4Arguments( "sendVertex(int,int,int)", position, toStack, event.toString(), _spacetreeId );
  
  {
    peano4::datamanagement::VertexMarker  marker(event,position);

    const benchmarks::swift2::noh::vertexdata::hydroPartSet& data = repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop( 
      repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack)
    )->top(TwoPowerD-1-position);
    if ( 
      data.storePersistently(
        marker
        
      ) 
      and
      data.send(marker)
    ) {
      logDebug( "sendVertex(...)", "send out " << data.toString() << " to stack " << toStack << " on tree " << _spacetreeId << " for marker " << marker.toString() );

      repositories::DataRepository::_VertexDatahydroPartSetStack.getForPush(
        _spacetreeId, toStack
      ) -> push(data);
    }
  }
  logTraceOut( "sendVertex(int,int,int)");
}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::sendFace(int position, int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent&  event) {
  logTraceInWith4Arguments( "sendFace(int,int,int)", position, toStack, event.toString(), _spacetreeId );

  logTraceOut( "sendFace(int,int,int)");
}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::sendCell(int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent&  event) {
  logTraceInWith3Arguments( "sendCell(int,int,int)", toStack, event.toString(), _spacetreeId );
  
  {
    peano4::datamanagement::CellMarker  marker(event);
    const benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics&  data = repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top();
    if (
      data.storePersistently(
        marker
        
      ) 
      and
      data.send(
        marker
        
      )
    ) {
    
      logDebug( "sendCell(...)", "send out " << data.toString() << " to stack " << toStack << " on tree " << _spacetreeId );

      repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPush(
        _spacetreeId, toStack
      ) -> push(data);
    }
  }
  logTraceOut( "sendCell(int,int,int)");
}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::receiveAndMergeCell(int fromStack, peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent& event ) {
  logTraceInWith3Arguments( "receiveAndMergeCell(...)", fromStack, event.toString(), _spacetreeId );
  {
    peano4::datamanagement::CellMarker  marker(event);

    benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& data = repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPush( 
      repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack)
    )->top();

    if ( 
      data.loadPersistently(
        marker
        
      ) 
      and
      data.receiveAndMerge(
        marker
        
      )
    ) {
      auto   incomingData = std::move( repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack.getForPop(
        _spacetreeId, fromStack
      )->pop() );

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

      data.merge(context,incomingData, marker);
    }
  }
  logTraceOut( "receiveAndMergeCell(...)");
}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::receiveAndMergeFace(int position, int fromStack, peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent& event) {
  logTraceInWith4Arguments( "receiveAndMergeFace(...)", position, fromStack, event.toString(), _spacetreeId );
  logTraceOut( "receiveAndMergeFace(...)");
}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::receiveAndMergeVertex(int position, int fromStack, peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent& event) {
  logTraceInWith4Arguments( "receiveAndMergeVertex(...)", position, fromStack, event.toString(), _spacetreeId );
  {
    peano4::datamanagement::VertexMarker  marker(event,position);
    benchmarks::swift2::noh::vertexdata::hydroPartSet& data = repositories::DataRepository::_VertexDatahydroPartSetStack.getForPush( 
      repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack)
    )->top(TwoPowerD-1-position);

    if ( 
      data.loadPersistently(
        marker
        
      ) 
      and
      data.receiveAndMerge(
        marker
        
      )
    ) {
      assertion5(
        not repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop(_spacetreeId, fromStack)->empty(),
        _spacetreeId, fromStack, event.toString(), position, marker.toString()
      );
      
      auto   incomingData = std::move( repositories::DataRepository::_VertexDatahydroPartSetStack.getForPop(
        _spacetreeId, fromStack
      )->pop() );

      logDebug( "receiveAndMergeVertex(...)", "merge " << incomingData.toString() << " into " << data.toString() );

      if (context==::peano4::grid::TraversalObserver::SendReceiveContext::PeriodicBoundaryDataSwap) {
        // @todo Different to faces. As we can have diagonal exchange, too,
        //       we only know that it has to be unequal
        assertion8(
          data.getDebugX()!=incomingData.getDebugX(),
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

      data.merge(context, incomingData, marker);
    }
  }
  logTraceOut( "receiveAndMergeVertex(...)");
}


void benchmarks::swift2::noh::observers::StephydroPartReduceGlobalQuantities_sweep11::deleteAllStacks() {
  logTraceInWith1Argument( "deleteAllStacks()", _spacetreeId );
  peano4::parallel::SpacetreeSet::deleteAllStacks(
    repositories::DataRepository::_CellDatahydroPartSetCellStatisticsStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::deleteAllStacks(
    repositories::DataRepository::_VertexDatahydroPartSetStack,
    _spacetreeId
  );
  logTraceOut( "deleteAllStacks()");
}

