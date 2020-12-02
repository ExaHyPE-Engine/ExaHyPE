
#include "TimeStep.h"
#include "DataRepository.h"

#include "peano4/grid/PeanoCurve.h"

#include "peano4/datamanagement/VertexEnumerator.h"
#include "peano4/datamanagement/VertexMarker.h"
#include "peano4/datamanagement/FaceEnumerator.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "peano4/datamanagement/CellMarker.h"

#include "peano4/parallel/SpacetreeSet.h"


tarch::logging::Log examples::exahype2::euler::observers::TimeStep::_log( "examples::exahype2::euler::observers::TimeStep" );


  
examples::exahype2::euler::observers::TimeStep::TimeStep(int spacetreeId):
  _spacetreeId( spacetreeId ) , _actionSet0(spacetreeId), _actionSet1(spacetreeId), _actionSet2(spacetreeId), _actionSet3(spacetreeId), _actionSet4(spacetreeId), _actionSet5(spacetreeId), _actionSet6(spacetreeId), _actionSet7(spacetreeId)
{
}
  

  
  
peano4::grid::TraversalObserver* examples::exahype2::euler::observers::TimeStep::clone(int spacetreeId) {
  return new TimeStep(spacetreeId);
}
  
  
  
std::vector< peano4::grid::GridControlEvent > examples::exahype2::euler::observers::TimeStep::getGridControlEvents() const {
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

  {
    const std::vector< peano4::grid::GridControlEvent > actionResult = _actionSet5.getGridControlEvents();
    result.insert(result.begin(),actionResult.begin(),actionResult.end());
  }

  {
    const std::vector< peano4::grid::GridControlEvent > actionResult = _actionSet6.getGridControlEvents();
    result.insert(result.begin(),actionResult.begin(),actionResult.end());
  }

  {
    const std::vector< peano4::grid::GridControlEvent > actionResult = _actionSet7.getGridControlEvents();
    result.insert(result.begin(),actionResult.begin(),actionResult.end());
  }

  return result;
}
  

  
void examples::exahype2::euler::observers::TimeStep::beginTraversal( const tarch::la::Vector<Dimensions,double>&  x, const tarch::la::Vector<Dimensions,double>&  h ) {
  logTraceInWith2Arguments( "beginTraversal(...)", x, h );
  //
  // Invoke beginTraversal() on the actions
  //
  _actionSet0.beginTraversal();
  _actionSet1.beginTraversal();
  _actionSet2.beginTraversal();
  _actionSet3.beginTraversal();
  _actionSet4.beginTraversal();
  _actionSet5.beginTraversal();
  _actionSet6.beginTraversal();
  _actionSet7.beginTraversal();


  //
  // Fill call stacks with dummies which represent level -1 such that we can 
  // call standard action routines on level 0 with parents. Without these 
  // statements, a top(1) call would raise an assertion. 
  //
  DataRepository::_CellDataEulerQStack.getForPush( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( examples::exahype2::euler::celldata::EulerQ() );
  DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPush( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( examples::exahype2::euler::celldata::EulerCellSemaphoreLabel() );
  for (int i=0; i<TwoTimesD; i++) {
    DataRepository::_FaceDataEulerQStack.getForPush( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( examples::exahype2::euler::facedata::EulerQ() );
  }
  for (int i=0; i<TwoTimesD; i++) {
    DataRepository::_FaceDataEulerQNewStack.getForPush( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( examples::exahype2::euler::facedata::EulerQNew() );
  }
  for (int i=0; i<TwoTimesD; i++) {
    DataRepository::_FaceDataLabelStack.getForPush( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( examples::exahype2::euler::facedata::Label() );
  }

  logTraceOutWith2Arguments( "beginTraversal(...)", x, h );
}
  
  
  
void examples::exahype2::euler::observers::TimeStep::endTraversal( const tarch::la::Vector<Dimensions,double>&  x, const tarch::la::Vector<Dimensions,double>&  h ) {
  logTraceInWith2Arguments( "endTraversal(...)", x, h );
  _actionSet0.endTraversal();
  _actionSet1.endTraversal();
  _actionSet2.endTraversal();
  _actionSet3.endTraversal();
  _actionSet4.endTraversal();
  _actionSet5.endTraversal();
  _actionSet6.endTraversal();
  _actionSet7.endTraversal();

  DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();
  DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();
  for (int i=0; i<TwoTimesD; i++) {
    DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();
  }
  for (int i=0; i<TwoTimesD; i++) {
    DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();
  }
  for (int i=0; i<TwoTimesD; i++) {
    DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();
  }

  logTraceOutWith2Arguments( "endTraversal(...)", x, h );
}
  
    
void examples::exahype2::euler::observers::TimeStep::enterCell( const peano4::grid::GridTraversalEvent&  event ) {
  logTraceInWith2Arguments( "enterCell(...)", _spacetreeId, event.toString() );
  
  // Load face EulerQ
  {
    auto view = DataRepository::_FaceDataEulerQStack.getForPush( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStack          = event.getFaceDataFrom(i);
      int outFaceStackPosition = event.getFaceDataTo(i);
      logDebug("enterCell(...)", "face stack " << inFaceStack << "->pos-" << outFaceStackPosition );

      peano4::datamanagement::FaceMarker  marker(event,outFaceStackPosition);
      
      examples::exahype2::euler::facedata::EulerQ data ;
      if (
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::NoData
        and
        examples::exahype2::euler::facedata::EulerQ::loadPersistently(marker)
      ) {
        assertion4( not DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,inFaceStack))->empty(), event.toString(), peano4::datamanagement::FaceMarker(event).toString(), _spacetreeId,inFaceStack );
        data = DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,inFaceStack))->pop();
      }
      
      #if PeanoDebug>0  
      if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        or
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        or
        inFaceStack==peano4::grid::TraversalObserver::NoData
        or
        not examples::exahype2::euler::facedata::EulerQ::loadPersistently(marker)
      ) {
        data.setDebugX( marker.x(outFaceStackPosition) );
        data.setDebugH( marker.h() );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
      ) {
        assertionVectorNumericalEquals5( data.getDebugX(), marker.x(outFaceStackPosition), data.getDebugX(), data.getDebugH(), marker.toString(), outFaceStackPosition, _spacetreeId );
        assertionVectorNumericalEquals5( data.getDebugH(), marker.h(),                     data.getDebugX(), data.getDebugH(), marker.toString(), outFaceStackPosition, _spacetreeId );
      }
      #endif

      view.set(outFaceStackPosition,data);
    }
  }
  
  // Load face EulerQNew
  {
    auto view = DataRepository::_FaceDataEulerQNewStack.getForPush( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStack          = event.getFaceDataFrom(i);
      int outFaceStackPosition = event.getFaceDataTo(i);
      logDebug("enterCell(...)", "face stack " << inFaceStack << "->pos-" << outFaceStackPosition );

      peano4::datamanagement::FaceMarker  marker(event,outFaceStackPosition);
      
      examples::exahype2::euler::facedata::EulerQNew data ;
      if (
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::NoData
        and
        examples::exahype2::euler::facedata::EulerQNew::loadPersistently(marker)
      ) {
        assertion4( not DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,inFaceStack))->empty(), event.toString(), peano4::datamanagement::FaceMarker(event).toString(), _spacetreeId,inFaceStack );
        data = DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,inFaceStack))->pop();
      }
      
      #if PeanoDebug>0  
      if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        or
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        or
        inFaceStack==peano4::grid::TraversalObserver::NoData
        or
        not examples::exahype2::euler::facedata::EulerQNew::loadPersistently(marker)
      ) {
        data.setDebugX( marker.x(outFaceStackPosition) );
        data.setDebugH( marker.h() );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
      ) {
        assertionVectorNumericalEquals5( data.getDebugX(), marker.x(outFaceStackPosition), data.getDebugX(), data.getDebugH(), marker.toString(), outFaceStackPosition, _spacetreeId );
        assertionVectorNumericalEquals5( data.getDebugH(), marker.h(),                     data.getDebugX(), data.getDebugH(), marker.toString(), outFaceStackPosition, _spacetreeId );
      }
      #endif

      view.set(outFaceStackPosition,data);
    }
  }
  
  // Load face Label
  {
    auto view = DataRepository::_FaceDataLabelStack.getForPush( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStack          = event.getFaceDataFrom(i);
      int outFaceStackPosition = event.getFaceDataTo(i);
      logDebug("enterCell(...)", "face stack " << inFaceStack << "->pos-" << outFaceStackPosition );

      peano4::datamanagement::FaceMarker  marker(event,outFaceStackPosition);
      
      examples::exahype2::euler::facedata::Label data ;
      if (
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::NoData
        and
        examples::exahype2::euler::facedata::Label::loadPersistently(marker)
      ) {
        assertion4( not DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,inFaceStack))->empty(), event.toString(), peano4::datamanagement::FaceMarker(event).toString(), _spacetreeId,inFaceStack );
        data = DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,inFaceStack))->pop();
      }
      
      #if PeanoDebug>0  
      if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        or
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        or
        inFaceStack==peano4::grid::TraversalObserver::NoData
        or
        not examples::exahype2::euler::facedata::Label::loadPersistently(marker)
      ) {
        data.setDebugX( marker.x(outFaceStackPosition) );
        data.setDebugH( marker.h() );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
      ) {
        assertionVectorNumericalEquals5( data.getDebugX(), marker.x(outFaceStackPosition), data.getDebugX(), data.getDebugH(), marker.toString(), outFaceStackPosition, _spacetreeId );
        assertionVectorNumericalEquals5( data.getDebugH(), marker.h(),                     data.getDebugX(), data.getDebugH(), marker.toString(), outFaceStackPosition, _spacetreeId );
      }
      #endif

      view.set(outFaceStackPosition,data);
    }
  }
  
  // Handle face Label
  {
    peano4::datamanagement::FaceMarker marker( event );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStack = event.getFaceDataFrom(i);
      int pick        = event.getFaceDataTo(i);

      marker.select(pick);
      
      assertion3( marker.isLocal() or not event.getIsCellLocal(), marker.toString(), event.toString(), i );
            
      if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet0.createPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet0.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet0.createHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet0.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet0.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
    }
  }
  
  // Handle face Label
  {
    peano4::datamanagement::FaceMarker marker( event );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStack = event.getFaceDataFrom(i);
      int pick        = event.getFaceDataTo(i);

      marker.select(pick);
      
      assertion3( marker.isLocal() or not event.getIsCellLocal(), marker.toString(), event.toString(), i );
            
      if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet1.createPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet1.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet1.createHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet1.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet1.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
    }
  }
  
  // Handle face Label
  {
    peano4::datamanagement::FaceMarker marker( event );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStack = event.getFaceDataFrom(i);
      int pick        = event.getFaceDataTo(i);

      marker.select(pick);
      
      assertion3( marker.isLocal() or not event.getIsCellLocal(), marker.toString(), event.toString(), i );
            
      if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet2.createPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet2.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet2.createHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet2.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet2.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
    }
  }
  
  // Handle face Label
  {
    peano4::datamanagement::FaceMarker marker( event );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStack = event.getFaceDataFrom(i);
      int pick        = event.getFaceDataTo(i);

      marker.select(pick);
      
      assertion3( marker.isLocal() or not event.getIsCellLocal(), marker.toString(), event.toString(), i );
            
      if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet3.createPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet3.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet3.createHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet3.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet3.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
    }
  }
  
  // Handle face Label
  {
    peano4::datamanagement::FaceMarker marker( event );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStack = event.getFaceDataFrom(i);
      int pick        = event.getFaceDataTo(i);

      marker.select(pick);
      
      assertion3( marker.isLocal() or not event.getIsCellLocal(), marker.toString(), event.toString(), i );
            
      if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet4.createPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet4.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet4.createHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet4.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet4.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
    }
  }
  
  // Handle face Label
  {
    peano4::datamanagement::FaceMarker marker( event );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStack = event.getFaceDataFrom(i);
      int pick        = event.getFaceDataTo(i);

      marker.select(pick);
      
      assertion3( marker.isLocal() or not event.getIsCellLocal(), marker.toString(), event.toString(), i );
            
      if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet5.createPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet5.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet5.createHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet5.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet5.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
    }
  }
  
  // Handle face Label
  {
    peano4::datamanagement::FaceMarker marker( event );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStack = event.getFaceDataFrom(i);
      int pick        = event.getFaceDataTo(i);

      marker.select(pick);
      
      assertion3( marker.isLocal() or not event.getIsCellLocal(), marker.toString(), event.toString(), i );
            
      if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet6.createPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet6.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet6.createHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet6.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet6.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
    }
  }
  
  // Handle face Label
  {
    peano4::datamanagement::FaceMarker marker( event );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStack = event.getFaceDataFrom(i);
      int pick        = event.getFaceDataTo(i);

      marker.select(pick);
      
      assertion3( marker.isLocal() or not event.getIsCellLocal(), marker.toString(), event.toString(), i );
            
      if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet7.createPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet7.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet7.createHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet7.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet7.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
    }
  }
  
  // Load cell EulerQ
  {
    auto view = DataRepository::_CellDataEulerQStack.getForPush( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( 1 );
    
    peano4::datamanagement::CellMarker  marker(event);

    const int inCellStack  = event.getCellData();
    const int outCellStack = peano4::grid::PeanoCurve::CallStack;
    logDebug("enterCell(...)", "cell stack " << inCellStack << "->pos-" << outCellStack << "(" << examples::exahype2::euler::celldata::EulerQ::loadPersistently(marker) << ")" );
        
    examples::exahype2::euler::celldata::EulerQ& data = view.get(0);
    if (
      inCellStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      inCellStack!=peano4::grid::TraversalObserver::NoData
      and
      examples::exahype2::euler::celldata::EulerQ::loadPersistently(marker)
    ) {
      assertion3( not DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,inCellStack))->empty(), event.toString(), _spacetreeId, inCellStack);
      data = DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,inCellStack))->pop();
    }
    
    #if PeanoDebug>0  
    if (
      inCellStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      or
      inCellStack==peano4::grid::TraversalObserver::NoData
      or
      not examples::exahype2::euler::celldata::EulerQ::loadPersistently(marker)
    ) {
      data.setDebugX( marker.x() );
      data.setDebugH( marker.h() );
    }
    else {
      assertionVectorNumericalEquals3( data.getDebugX(), marker.x(), data.getDebugX(), marker.toString(), _spacetreeId );
      assertionVectorNumericalEquals3( data.getDebugH(), marker.h(), data.getDebugX(), marker.toString(), _spacetreeId );
    }
    #endif
  }
  
  // Load cell EulerCellSemaphoreLabel
  {
    auto view = DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPush( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( 1 );
    
    peano4::datamanagement::CellMarker  marker(event);

    const int inCellStack  = event.getCellData();
    const int outCellStack = peano4::grid::PeanoCurve::CallStack;
    logDebug("enterCell(...)", "cell stack " << inCellStack << "->pos-" << outCellStack << "(" << examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::loadPersistently(marker) << ")" );
        
    examples::exahype2::euler::celldata::EulerCellSemaphoreLabel& data = view.get(0);
    if (
      inCellStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      inCellStack!=peano4::grid::TraversalObserver::NoData
      and
      examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::loadPersistently(marker)
    ) {
      assertion3( not DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,inCellStack))->empty(), event.toString(), _spacetreeId, inCellStack);
      data = DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,inCellStack))->pop();
    }
    
    #if PeanoDebug>0  
    if (
      inCellStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      or
      inCellStack==peano4::grid::TraversalObserver::NoData
      or
      not examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::loadPersistently(marker)
    ) {
      data.setDebugX( marker.x() );
      data.setDebugH( marker.h() );
    }
    else {
      assertionVectorNumericalEquals3( data.getDebugX(), marker.x(), data.getDebugX(), marker.toString(), _spacetreeId );
      assertionVectorNumericalEquals3( data.getDebugH(), marker.h(), data.getDebugX(), marker.toString(), _spacetreeId );
    }
    #endif
  }
  
  // Invoke creational events on cell EulerCellSemaphoreLabel
  {
    peano4::datamanagement::CellMarker marker( event );
    if (
      event.getCellData()==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and 
      marker.isLocal()
    ) {
      _actionSet0.createCell(
        marker,
        
        peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),
        DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )
        ,DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }  
  }
  
  // Invoke creational events on cell EulerCellSemaphoreLabel
  {
    peano4::datamanagement::CellMarker marker( event );
    if (
      event.getCellData()==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and 
      marker.isLocal()
    ) {
      _actionSet1.createCell(
        marker,
        
        peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),
        DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )
        ,DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }  
  }
  
  // Invoke creational events on cell EulerCellSemaphoreLabel
  {
    peano4::datamanagement::CellMarker marker( event );
    if (
      event.getCellData()==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and 
      marker.isLocal()
    ) {
      _actionSet2.createCell(
        marker,
        
        peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),
        DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )
        ,DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }  
  }
  
  // Invoke creational events on cell EulerCellSemaphoreLabel
  {
    peano4::datamanagement::CellMarker marker( event );
    if (
      event.getCellData()==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and 
      marker.isLocal()
    ) {
      _actionSet3.createCell(
        marker,
        
        peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),
        DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )
        ,DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }  
  }
  
  // Invoke creational events on cell EulerCellSemaphoreLabel
  {
    peano4::datamanagement::CellMarker marker( event );
    if (
      event.getCellData()==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and 
      marker.isLocal()
    ) {
      _actionSet4.createCell(
        marker,
        
        peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),
        DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )
        ,DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }  
  }
  
  // Invoke creational events on cell EulerCellSemaphoreLabel
  {
    peano4::datamanagement::CellMarker marker( event );
    if (
      event.getCellData()==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and 
      marker.isLocal()
    ) {
      _actionSet5.createCell(
        marker,
        
        peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),
        DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )
        ,DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }  
  }
  
  // Invoke creational events on cell EulerCellSemaphoreLabel
  {
    peano4::datamanagement::CellMarker marker( event );
    if (
      event.getCellData()==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and 
      marker.isLocal()
    ) {
      _actionSet6.createCell(
        marker,
        
        peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),
        DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )
        ,DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }  
  }
  
  // Invoke creational events on cell EulerCellSemaphoreLabel
  {
    peano4::datamanagement::CellMarker marker( event );
    if (
      event.getCellData()==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and 
      marker.isLocal()
    ) {
      _actionSet7.createCell(
        marker,
        
        peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),
        DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )
        ,DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }  
  }
  
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet0.touchCellFirstTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet1.touchCellFirstTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet2.touchCellFirstTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet3.touchCellFirstTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet4.touchCellFirstTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet5.touchCellFirstTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet6.touchCellFirstTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet7.touchCellFirstTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }

  logTraceOut( "enterCell(...)" );
}
  
void examples::exahype2::euler::observers::TimeStep::leaveCell( const peano4::grid::GridTraversalEvent&  event ) {
  logTraceInWith2Arguments( "leaveCell(...)", _spacetreeId, event.toString() );
  
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet7.touchCellLastTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet6.touchCellLastTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet5.touchCellLastTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet4.touchCellLastTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet3.touchCellLastTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet2.touchCellLastTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet1.touchCellLastTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet0.touchCellLastTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
    _actionSet7.destroyCell(
      marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )
        ,DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
    _actionSet6.destroyCell(
      marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )
        ,DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
    _actionSet5.destroyCell(
      marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )
        ,DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )
        ,DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )
        ,DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )
        ,DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )
        ,DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )
        ,DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      ); 
    }
  }
  
  // Handle cell EulerQ
  {
    const int inCellStack   = peano4::grid::PeanoCurve::CallStack;
    const int outCellStack  = event.getCellData();
    logDebug("leaveCell(...)", "cell stack " << inCellStack << "->pos-" << outCellStack );

    peano4::datamanagement::CellMarker  marker(event);

    auto view = DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( 1 );

    if (
      outCellStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      outCellStack!=peano4::grid::TraversalObserver::NoData
      and
      examples::exahype2::euler::celldata::EulerQ::storePersistently(marker)
    ) {
      DataRepository::_CellDataEulerQStack.getForPush( DataRepository::DataKey(_spacetreeId,outCellStack))->push( view.get(0) );
    }
  }
  
  // Handle cell EulerCellSemaphoreLabel
  {
    const int inCellStack   = peano4::grid::PeanoCurve::CallStack;
    const int outCellStack  = event.getCellData();
    logDebug("leaveCell(...)", "cell stack " << inCellStack << "->pos-" << outCellStack );

    peano4::datamanagement::CellMarker  marker(event);

    auto view = DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( 1 );

    if (
      outCellStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      outCellStack!=peano4::grid::TraversalObserver::NoData
      and
      examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::storePersistently(marker)
    ) {
      DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPush( DataRepository::DataKey(_spacetreeId,outCellStack))->push( view.get(0) );
    }
  }

  // Handle face EulerCellSemaphoreLabel
  {
    peano4::datamanagement::FaceMarker  marker(event);
    
    for (int i=0; i<TwoTimesD; i++) {
      int outFaceStack      = event.getFaceDataTo(i);
      int pick              = event.getFaceDataFrom(i);

      marker.select(pick);

      if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet7.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet7.destroyPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet7.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet7.destroyHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet7.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
    }
  }

  // Handle face EulerCellSemaphoreLabel
  {
    peano4::datamanagement::FaceMarker  marker(event);
    
    for (int i=0; i<TwoTimesD; i++) {
      int outFaceStack      = event.getFaceDataTo(i);
      int pick              = event.getFaceDataFrom(i);

      marker.select(pick);

      if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet6.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet6.destroyPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet6.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet6.destroyHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet6.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
    }
  }

  // Handle face EulerCellSemaphoreLabel
  {
    peano4::datamanagement::FaceMarker  marker(event);
    
    for (int i=0; i<TwoTimesD; i++) {
      int outFaceStack      = event.getFaceDataTo(i);
      int pick              = event.getFaceDataFrom(i);

      marker.select(pick);

      if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet5.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet5.destroyPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet5.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet5.destroyHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet5.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
    }
  }

  // Handle face EulerCellSemaphoreLabel
  {
    peano4::datamanagement::FaceMarker  marker(event);
    
    for (int i=0; i<TwoTimesD; i++) {
      int outFaceStack      = event.getFaceDataTo(i);
      int pick              = event.getFaceDataFrom(i);

      marker.select(pick);

      if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet4.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet4.destroyPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet4.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet4.destroyHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet4.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
    }
  }

  // Handle face EulerCellSemaphoreLabel
  {
    peano4::datamanagement::FaceMarker  marker(event);
    
    for (int i=0; i<TwoTimesD; i++) {
      int outFaceStack      = event.getFaceDataTo(i);
      int pick              = event.getFaceDataFrom(i);

      marker.select(pick);

      if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet3.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet3.destroyPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet3.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet3.destroyHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet3.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
    }
  }

  // Handle face EulerCellSemaphoreLabel
  {
    peano4::datamanagement::FaceMarker  marker(event);
    
    for (int i=0; i<TwoTimesD; i++) {
      int outFaceStack      = event.getFaceDataTo(i);
      int pick              = event.getFaceDataFrom(i);

      marker.select(pick);

      if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet2.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet2.destroyPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet2.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet2.destroyHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet2.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
    }
  }

  // Handle face EulerCellSemaphoreLabel
  {
    peano4::datamanagement::FaceMarker  marker(event);
    
    for (int i=0; i<TwoTimesD; i++) {
      int outFaceStack      = event.getFaceDataTo(i);
      int pick              = event.getFaceDataFrom(i);

      marker.select(pick);

      if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet1.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet1.destroyPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet1.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet1.destroyHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet1.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
    }
  }

  // Handle face EulerCellSemaphoreLabel
  {
    peano4::datamanagement::FaceMarker  marker(event);
    
    for (int i=0; i<TwoTimesD; i++) {
      int outFaceStack      = event.getFaceDataTo(i);
      int pick              = event.getFaceDataFrom(i);

      marker.select(pick);

      if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet0.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet0.destroyPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet0.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
        _actionSet0.destroyHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet0.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQ>( &DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::EulerQNew>( &DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler::facedata::Label>( &DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),DataRepository::_CellDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        );
      }
    }
  }

  // Store face EulerQ
  {
    auto view = DataRepository::_FaceDataEulerQStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStackPosition  = event.getFaceDataFrom(i);
      int outFaceStack         = event.getFaceDataTo(i);
      logDebug("leaveCell(...)", "pos-" << inFaceStackPosition << "->face stack " << outFaceStack );
      
      peano4::datamanagement::FaceMarker  marker(event,inFaceStackPosition);

      examples::exahype2::euler::facedata::EulerQ data = view.get(inFaceStackPosition);
      
      if (
        outFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        outFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        outFaceStack!=peano4::grid::TraversalObserver::NoData
        and
        examples::exahype2::euler::facedata::EulerQ::storePersistently(marker)
      ) {
        DataRepository::_FaceDataEulerQStack.getForPush( DataRepository::DataKey(_spacetreeId,outFaceStack))->push(data);
      }
    }
  }

  // Store face EulerQNew
  {
    auto view = DataRepository::_FaceDataEulerQNewStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStackPosition  = event.getFaceDataFrom(i);
      int outFaceStack         = event.getFaceDataTo(i);
      logDebug("leaveCell(...)", "pos-" << inFaceStackPosition << "->face stack " << outFaceStack );
      
      peano4::datamanagement::FaceMarker  marker(event,inFaceStackPosition);

      examples::exahype2::euler::facedata::EulerQNew data = view.get(inFaceStackPosition);
      
      if (
        outFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        outFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        outFaceStack!=peano4::grid::TraversalObserver::NoData
        and
        examples::exahype2::euler::facedata::EulerQNew::storePersistently(marker)
      ) {
        DataRepository::_FaceDataEulerQNewStack.getForPush( DataRepository::DataKey(_spacetreeId,outFaceStack))->push(data);
      }
    }
  }

  // Store face Label
  {
    auto view = DataRepository::_FaceDataLabelStack.getForPop( DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStackPosition  = event.getFaceDataFrom(i);
      int outFaceStack         = event.getFaceDataTo(i);
      logDebug("leaveCell(...)", "pos-" << inFaceStackPosition << "->face stack " << outFaceStack );
      
      peano4::datamanagement::FaceMarker  marker(event,inFaceStackPosition);

      examples::exahype2::euler::facedata::Label data = view.get(inFaceStackPosition);
      
      if (
        outFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        outFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        outFaceStack!=peano4::grid::TraversalObserver::NoData
        and
        examples::exahype2::euler::facedata::Label::storePersistently(marker)
      ) {
        DataRepository::_FaceDataLabelStack.getForPush( DataRepository::DataKey(_spacetreeId,outFaceStack))->push(data);
      }
    }
  }
  
  logTraceOut( "leaveCell(...)" );
}

void examples::exahype2::euler::observers::TimeStep::exchangeAllVerticalDataExchangeStacks( int masterId ) {
  logTraceInWith2Arguments( "exchangeAllVerticalDataExchangeStacks(...)", masterId, _spacetreeId  );

  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(
    DataRepository::_CellDataEulerQStack,
    _spacetreeId,
    masterId
  );

  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(
    DataRepository::_CellDataEulerCellSemaphoreLabelStack,
    _spacetreeId,
    masterId
  );

  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(
    DataRepository::_FaceDataEulerQStack,
    _spacetreeId,
    masterId
  );

  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(
    DataRepository::_FaceDataEulerQNewStack,
    _spacetreeId,
    masterId
  );

  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(
    DataRepository::_FaceDataLabelStack,
    _spacetreeId,
    masterId
  );

  logTraceOut( "exchangeAllVerticalDataExchangeStacks(...)" );
}

void examples::exahype2::euler::observers::TimeStep::exchangeAllHorizontalDataExchangeStacks( bool symmetricDataCardinality ) {
  logTraceInWith2Arguments( "exchangeAllHorizontalDataExchangeStacks(...)", symmetricDataCardinality, _spacetreeId  );

  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(
    DataRepository::_CellDataEulerQStack,
    _spacetreeId,
    symmetricDataCardinality
  );

  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(
    DataRepository::_CellDataEulerCellSemaphoreLabelStack,
    _spacetreeId,
    symmetricDataCardinality
  );

  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(
    DataRepository::_FaceDataEulerQStack,
    _spacetreeId,
    symmetricDataCardinality
  );

  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(
    DataRepository::_FaceDataEulerQNewStack,
    _spacetreeId,
    symmetricDataCardinality
  );

  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(
    DataRepository::_FaceDataLabelStack,
    _spacetreeId,
    symmetricDataCardinality
  );

  logTraceOut( "exchangeAllHorizontalDataExchangeStacks(...)" );
}

void examples::exahype2::euler::observers::TimeStep::exchangeAllPeriodicBoundaryDataStacks() {
  logTraceInWith1Argument( "exchangeAllPeriodicBoundaryDataStacks()", _spacetreeId  );

  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(
    DataRepository::_CellDataEulerQStack,
    _spacetreeId
  );

  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(
    DataRepository::_CellDataEulerCellSemaphoreLabelStack,
    _spacetreeId
  );

  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(
    DataRepository::_FaceDataEulerQStack,
    _spacetreeId
  );

  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(
    DataRepository::_FaceDataEulerQNewStack,
    _spacetreeId
  );

  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(
    DataRepository::_FaceDataLabelStack,
    _spacetreeId
  );

  logTraceOut( "exchangeAllPeriodicBoundaryDataStacks()" );
}

void examples::exahype2::euler::observers::TimeStep::streamDataFromSplittingTreeToNewTree(int newWorker) {
  logTraceInWith2Arguments( "streamDataFromSplittingTreeToNewTree(int)", _spacetreeId, newWorker );

  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(
    DataRepository::_CellDataEulerQStack,
    _spacetreeId,
    newWorker
  );

  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(
    DataRepository::_CellDataEulerCellSemaphoreLabelStack,
    _spacetreeId,
    newWorker
  );

  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(
    DataRepository::_FaceDataEulerQStack,
    _spacetreeId,
    newWorker
  );

  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(
    DataRepository::_FaceDataEulerQNewStack,
    _spacetreeId,
    newWorker
  );

  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(
    DataRepository::_FaceDataLabelStack,
    _spacetreeId,
    newWorker
  );

  logTraceOut( "streamDataFromSplittingTreeToNewTree(int)");
}

void examples::exahype2::euler::observers::TimeStep::streamDataFromJoiningTreeToMasterTree(int master) {
  logTraceInWith2Arguments( "streamDataFromJoiningTreeToMasterTree(int)", _spacetreeId, master );

  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(
    DataRepository::_CellDataEulerQStack,
    _spacetreeId,
    master
  );

  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(
    DataRepository::_CellDataEulerCellSemaphoreLabelStack,
    _spacetreeId,
    master
  );

  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(
    DataRepository::_FaceDataEulerQStack,
    _spacetreeId,
    master
  );

  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(
    DataRepository::_FaceDataEulerQNewStack,
    _spacetreeId,
    master
  );

  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(
    DataRepository::_FaceDataLabelStack,
    _spacetreeId,
    master
  );

  logTraceOut( "streamDataFromJoiningTreeToMasterTree(int)");
}

void examples::exahype2::euler::observers::TimeStep::finishAllOutstandingSendsAndReceives() {
  logTraceInWith1Argument( "finishAllOutstandingSendsAndReceives()", _spacetreeId );

  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(
    DataRepository::_CellDataEulerQStack,
    _spacetreeId
  );

  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(
    DataRepository::_CellDataEulerCellSemaphoreLabelStack,
    _spacetreeId
  );

  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(
    DataRepository::_FaceDataEulerQStack,
    _spacetreeId
  );

  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(
    DataRepository::_FaceDataEulerQNewStack,
    _spacetreeId
  );

  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(
    DataRepository::_FaceDataLabelStack,
    _spacetreeId
  );

  logTraceOut( "finishAllOutstandingSendsAndReceives()");
}

void examples::exahype2::euler::observers::TimeStep::sendVertex(int inOutStack, int relativePositionOnInOutStack, int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::datamanagement::VertexMarker& marker) {
  logTraceInWith4Arguments( "sendVertex(int,int,int)", inOutStack, relativePositionOnInOutStack, toStack, _spacetreeId );

  logTraceOut( "sendVertex(int,int,int)");
}

void examples::exahype2::euler::observers::TimeStep::sendFace(int inOutStack, int relativePositionOnInOutStack, int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::datamanagement::FaceMarker& marker) {
  logTraceInWith4Arguments( "sendFace(int,int,int)", inOutStack, relativePositionOnInOutStack, toStack, _spacetreeId );

  if ( context==::peano4::grid::TraversalObserver::SendReceiveContext::Rebalancing or examples::exahype2::euler::facedata::EulerQ::send(marker) ) {
    auto& data = DataRepository::_FaceDataEulerQStack.getForPop(
      _spacetreeId,inOutStack
    )->top(relativePositionOnInOutStack);
    logDebug( "sendXXX(...)", "send out " << data.toString() << " to stack " << toStack << " (relativePositionOnInOutStack=" << relativePositionOnInOutStack << ")" );
    
    DataRepository::_FaceDataEulerQStack.getForPush(
      _spacetreeId, toStack
    ) -> push(data);
  }

  if ( context==::peano4::grid::TraversalObserver::SendReceiveContext::Rebalancing or examples::exahype2::euler::facedata::EulerQNew::send(marker) ) {
    auto& data = DataRepository::_FaceDataEulerQNewStack.getForPop(
      _spacetreeId,inOutStack
    )->top(relativePositionOnInOutStack);
    logDebug( "sendXXX(...)", "send out " << data.toString() << " to stack " << toStack << " (relativePositionOnInOutStack=" << relativePositionOnInOutStack << ")" );
    
    DataRepository::_FaceDataEulerQNewStack.getForPush(
      _spacetreeId, toStack
    ) -> push(data);
  }

  if ( context==::peano4::grid::TraversalObserver::SendReceiveContext::Rebalancing or examples::exahype2::euler::facedata::Label::send(marker) ) {
    auto& data = DataRepository::_FaceDataLabelStack.getForPop(
      _spacetreeId,inOutStack
    )->top(relativePositionOnInOutStack);
    logDebug( "sendXXX(...)", "send out " << data.toString() << " to stack " << toStack << " (relativePositionOnInOutStack=" << relativePositionOnInOutStack << ")" );
    
    DataRepository::_FaceDataLabelStack.getForPush(
      _spacetreeId, toStack
    ) -> push(data);
  }

  logTraceOut( "sendFace(int,int,int)");
}

void examples::exahype2::euler::observers::TimeStep::sendCell(int inOutStack, int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::datamanagement::CellMarker& marker) {
  logTraceInWith3Arguments( "sendCell(int,int)", inOutStack, toStack, _spacetreeId );
  const int relativePositionOnInOutStack = 0;

  if ( context==::peano4::grid::TraversalObserver::SendReceiveContext::Rebalancing or examples::exahype2::euler::celldata::EulerQ::send(marker) ) {
    auto& data = DataRepository::_CellDataEulerQStack.getForPop(
      _spacetreeId,inOutStack
    )->top(relativePositionOnInOutStack);
    logDebug( "sendXXX(...)", "send out " << data.toString() << " to stack " << toStack << " (relativePositionOnInOutStack=" << relativePositionOnInOutStack << ")" );
    
    DataRepository::_CellDataEulerQStack.getForPush(
      _spacetreeId, toStack
    ) -> push(data);
  }

  if ( context==::peano4::grid::TraversalObserver::SendReceiveContext::Rebalancing or examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::send(marker) ) {
    auto& data = DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPop(
      _spacetreeId,inOutStack
    )->top(relativePositionOnInOutStack);
    logDebug( "sendXXX(...)", "send out " << data.toString() << " to stack " << toStack << " (relativePositionOnInOutStack=" << relativePositionOnInOutStack << ")" );
    
    DataRepository::_CellDataEulerCellSemaphoreLabelStack.getForPush(
      _spacetreeId, toStack
    ) -> push(data);
  }

  logTraceOut( "sendCell(int,int)");
}

void examples::exahype2::euler::observers::TimeStep::receiveAndMergeVertex(const peano4::grid::GridTraversalEvent&  event, int positionWithinCell, int inOutStack, int relativePositionOnInOutStack, int fromStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::datamanagement::VertexMarker& marker) {
  logTraceInWith7Arguments( "receiveAndMergeVertex(...)", event.toString(), positionWithinCell, inOutStack, relativePositionOnInOutStack, fromStack, marker.toString(), _spacetreeId );

  logTraceOut( "receiveAndMergeVertex(...)");
}

void examples::exahype2::euler::observers::TimeStep::receiveAndMergeFace(const peano4::grid::GridTraversalEvent&  event, int positionWithinCell, int inOutStack, int relativePositionOnInOutStack, int fromStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::datamanagement::FaceMarker& marker) {
  logTraceInWith7Arguments( "receiveAndMergeFace(...)", event.toString(), positionWithinCell, inOutStack, relativePositionOnInOutStack, fromStack, marker.toString(), _spacetreeId ); 

  if ( context==::peano4::grid::TraversalObserver::SendReceiveContext::Rebalancing or examples::exahype2::euler::facedata::EulerQ::receiveAndMerge(marker) ) {
    auto   incomingData = DataRepository::_FaceDataEulerQStack.getForPop(
      _spacetreeId, fromStack
    )->pop();
  
    auto&  data = DataRepository::_FaceDataEulerQStack.getForPush( _spacetreeId, inOutStack )->top(
      relativePositionOnInOutStack
    );
    
    assertionVectorNumericalEquals7( 
      data.getDebugX(), incomingData.getDebugX(), 
      data.getDebugH(), incomingData.getDebugH(), fromStack, inOutStack, relativePositionOnInOutStack, marker.toString(), _spacetreeId );
    assertionVectorNumericalEquals7( 
      data.getDebugH(), incomingData.getDebugH(), 
      data.getDebugX(), incomingData.getDebugX(), fromStack, inOutStack, relativePositionOnInOutStack, marker.toString(), _spacetreeId );
    
    data.merge(incomingData, marker);
  }

  if ( context==::peano4::grid::TraversalObserver::SendReceiveContext::Rebalancing or examples::exahype2::euler::facedata::EulerQNew::receiveAndMerge(marker) ) {
    auto   incomingData = DataRepository::_FaceDataEulerQNewStack.getForPop(
      _spacetreeId, fromStack
    )->pop();
  
    auto&  data = DataRepository::_FaceDataEulerQNewStack.getForPush( _spacetreeId, inOutStack )->top(
      relativePositionOnInOutStack
    );
    
    assertionVectorNumericalEquals7( 
      data.getDebugX(), incomingData.getDebugX(), 
      data.getDebugH(), incomingData.getDebugH(), fromStack, inOutStack, relativePositionOnInOutStack, marker.toString(), _spacetreeId );
    assertionVectorNumericalEquals7( 
      data.getDebugH(), incomingData.getDebugH(), 
      data.getDebugX(), incomingData.getDebugX(), fromStack, inOutStack, relativePositionOnInOutStack, marker.toString(), _spacetreeId );
    
    data.merge(incomingData, marker);
  }

  if ( context==::peano4::grid::TraversalObserver::SendReceiveContext::Rebalancing or examples::exahype2::euler::facedata::Label::receiveAndMerge(marker) ) {
    auto   incomingData = DataRepository::_FaceDataLabelStack.getForPop(
      _spacetreeId, fromStack
    )->pop();
  
    auto&  data = DataRepository::_FaceDataLabelStack.getForPush( _spacetreeId, inOutStack )->top(
      relativePositionOnInOutStack
    );
    
    assertionVectorNumericalEquals7( 
      data.getDebugX(), incomingData.getDebugX(), 
      data.getDebugH(), incomingData.getDebugH(), fromStack, inOutStack, relativePositionOnInOutStack, marker.toString(), _spacetreeId );
    assertionVectorNumericalEquals7( 
      data.getDebugH(), incomingData.getDebugH(), 
      data.getDebugX(), incomingData.getDebugX(), fromStack, inOutStack, relativePositionOnInOutStack, marker.toString(), _spacetreeId );
    
    data.merge(incomingData, marker);
  }

  logTraceOut( "receiveAndMergeFace(...)");
}

void examples::exahype2::euler::observers::TimeStep::deleteAllStacks() {
  logTraceInWith1Argument( "deleteAllStacks()", _spacetreeId );

  peano4::parallel::SpacetreeSet::deleteAllStacks(
    DataRepository::_CellDataEulerQStack,
    _spacetreeId
  );

  peano4::parallel::SpacetreeSet::deleteAllStacks(
    DataRepository::_CellDataEulerCellSemaphoreLabelStack,
    _spacetreeId
  );

  peano4::parallel::SpacetreeSet::deleteAllStacks(
    DataRepository::_FaceDataEulerQStack,
    _spacetreeId
  );

  peano4::parallel::SpacetreeSet::deleteAllStacks(
    DataRepository::_FaceDataEulerQNewStack,
    _spacetreeId
  );

  peano4::parallel::SpacetreeSet::deleteAllStacks(
    DataRepository::_FaceDataLabelStack,
    _spacetreeId
  );

  logTraceOut( "deleteAllStacks()");
}
