
#include "CreateGridButPostponeRefinement.h"
#include "repositories/DataRepository.h"

#include "peano4/grid/PeanoCurve.h"

#include "peano4/datamanagement/VertexEnumerator.h"
#include "peano4/datamanagement/VertexMarker.h"
#include "peano4/datamanagement/FaceEnumerator.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "peano4/datamanagement/CellMarker.h"

#include "peano4/parallel/SpacetreeSet.h"


tarch::logging::Log examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::_log( "examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement" );


examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::CreateGridButPostponeRefinement(int spacetreeId):
  _spacetreeId( spacetreeId ) , _actionSet0(spacetreeId), _actionSet1(spacetreeId), _actionSet2(spacetreeId), _actionSet3(spacetreeId)
{}


  

peano4::grid::TraversalObserver* examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::clone(int spacetreeId) {
  return new CreateGridButPostponeRefinement(spacetreeId);
}

  

std::vector< peano4::grid::GridControlEvent > examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::getGridControlEvents() const {
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
  return result;
}



void examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::beginTraversal( const tarch::la::Vector<Dimensions,double>&  x, const tarch::la::Vector<Dimensions,double>&  h ) {
  logTraceInWith2Arguments( "beginTraversal(...)", x, h );
  //
  // Invoke beginTraversal() on the actions
  //
  _actionSet0.beginTraversal();
  _actionSet1.beginTraversal();
  _actionSet2.beginTraversal();
  _actionSet3.beginTraversal();


  //
  // Fill call stacks with dummies which represent level 0 such that we can
  // call standard action routines on level 1 with parents. Without these
  // statements, a top(1) call would raise an assertion.
  //
  repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( examples::exahype2::euler2D::celldata::euler2DCellLabel() );
  repositories::DataRepository::_CellDataeuler2DQStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( examples::exahype2::euler2D::celldata::euler2DQ() );
  for (int i=0; i<TwoTimesD; i++) {
    repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( examples::exahype2::euler2D::facedata::euler2DFaceLabel() );
  }
  for (int i=0; i<TwoTimesD; i++) {
    repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( examples::exahype2::euler2D::facedata::euler2DQEstimates() );
  }
  for (int i=0; i<TwoTimesD; i++) {
    repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( examples::exahype2::euler2D::facedata::euler2DQFluxEstimates() );
  }

  logTraceOutWith2Arguments( "beginTraversal(...)", x, h );
}

  

void examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::endTraversal( const tarch::la::Vector<Dimensions,double>&  x, const tarch::la::Vector<Dimensions,double>&  h ) {
  logTraceInWith2Arguments( "endTraversal(...)", x, h );
    _actionSet0.endTraversal();
  _actionSet1.endTraversal();
  _actionSet2.endTraversal();
  _actionSet3.endTraversal();

    repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();
  repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();
  for (int i=0; i<TwoTimesD; i++) {
    repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();
  }
  for (int i=0; i<TwoTimesD; i++) {
    repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();
  }
  for (int i=0; i<TwoTimesD; i++) {
    repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();
  }

  logTraceOutWith2Arguments( "endTraversal(...)", x, h );
}

  

void examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::prepareTraversal() {
  logTraceIn( "prepareTraversal(...)" );
    observers::CreateGridButPostponeRefinement2exahype2_solvers_aderdg_actionsets_InitialCondition0::prepareTraversal();
  observers::CreateGridButPostponeRefinement2exahype2_grid_FaceLabel1::prepareTraversal();
  observers::CreateGridButPostponeRefinement2exahype2_grid_CellLabel2::prepareTraversal();
  observers::CreateGridButPostponeRefinement2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3::prepareTraversal();

  logTraceOut( "prepareTraversal(...)" );
}

  

void examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::unprepareTraversal() {
  logTraceIn( "unprepareTraversal(...)" );
    observers::CreateGridButPostponeRefinement2exahype2_solvers_aderdg_actionsets_InitialCondition0::unprepareTraversal();
  observers::CreateGridButPostponeRefinement2exahype2_grid_FaceLabel1::unprepareTraversal();
  observers::CreateGridButPostponeRefinement2exahype2_grid_CellLabel2::unprepareTraversal();
  observers::CreateGridButPostponeRefinement2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3::unprepareTraversal();

  logTraceOut( "unprepareTraversal(...)" );
}

  
void examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::loadCell( const peano4::grid::GridTraversalEvent&  event ) {
  logTraceInWith2Arguments( "loadCell(...)", _spacetreeId, event.toString() );
  // Load face FaceDataeuler2DFaceLabel
  {
    auto view = repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStack          = event.getFaceDataFrom(i);
      int outFaceStackPosition = event.getFaceDataTo(i);
      logDebug("loadCell(...)", "face stack " << inFaceStack << "->pos-" << outFaceStackPosition );

      peano4::datamanagement::FaceMarker  marker(event,outFaceStackPosition);

      examples::exahype2::euler2D::facedata::euler2DFaceLabel data ;
      if (
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::NoData
        and
        (not peano4::grid::PeanoCurve::isInOutStack(inFaceStack) or examples::exahype2::euler2D::facedata::euler2DFaceLabel::loadPersistently(
          marker
          
        ))
      ) {
        assertion4( not repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inFaceStack))->empty(), event.toString(), peano4::datamanagement::FaceMarker(event).toString(), _spacetreeId,inFaceStack );
        data = repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inFaceStack))->pop();
      }

      #if PeanoDebug>0
      if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        or
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        or
        inFaceStack==peano4::grid::TraversalObserver::NoData
        or
        not examples::exahype2::euler2D::facedata::euler2DFaceLabel::loadPersistently(
          marker
          
        )
      ) {
        logDebug( "loadCell(...)", "initialise meta data of face euler2DFaceLabel with " << marker.x(outFaceStackPosition) << " x " << marker.h() << ": load persistent data=" << examples::exahype2::euler2D::facedata::euler2DFaceLabel::loadPersistently(
          marker
          
        )  << " (" << marker.toString() << ")" );
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
  // Load face FaceDataeuler2DQEstimates
  {
    auto view = repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStack          = event.getFaceDataFrom(i);
      int outFaceStackPosition = event.getFaceDataTo(i);
      logDebug("loadCell(...)", "face stack " << inFaceStack << "->pos-" << outFaceStackPosition );

      peano4::datamanagement::FaceMarker  marker(event,outFaceStackPosition);

      examples::exahype2::euler2D::facedata::euler2DQEstimates data ;
      if (
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::NoData
        and
        (not peano4::grid::PeanoCurve::isInOutStack(inFaceStack) or examples::exahype2::euler2D::facedata::euler2DQEstimates::loadPersistently(
          marker
          
        ))
      ) {
        assertion4( not repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inFaceStack))->empty(), event.toString(), peano4::datamanagement::FaceMarker(event).toString(), _spacetreeId,inFaceStack );
        data = repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inFaceStack))->pop();
      }

      #if PeanoDebug>0
      if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        or
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        or
        inFaceStack==peano4::grid::TraversalObserver::NoData
        or
        not examples::exahype2::euler2D::facedata::euler2DQEstimates::loadPersistently(
          marker
          
        )
      ) {
        logDebug( "loadCell(...)", "initialise meta data of face euler2DQEstimates with " << marker.x(outFaceStackPosition) << " x " << marker.h() << ": load persistent data=" << examples::exahype2::euler2D::facedata::euler2DQEstimates::loadPersistently(
          marker
          
        )  << " (" << marker.toString() << ")" );
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
  // Load face FaceDataeuler2DQFluxEstimates
  {
    auto view = repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStack          = event.getFaceDataFrom(i);
      int outFaceStackPosition = event.getFaceDataTo(i);
      logDebug("loadCell(...)", "face stack " << inFaceStack << "->pos-" << outFaceStackPosition );

      peano4::datamanagement::FaceMarker  marker(event,outFaceStackPosition);

      examples::exahype2::euler2D::facedata::euler2DQFluxEstimates data ;
      if (
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::NoData
        and
        (not peano4::grid::PeanoCurve::isInOutStack(inFaceStack) or examples::exahype2::euler2D::facedata::euler2DQFluxEstimates::loadPersistently(
          marker
          
        ))
      ) {
        assertion4( not repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inFaceStack))->empty(), event.toString(), peano4::datamanagement::FaceMarker(event).toString(), _spacetreeId,inFaceStack );
        data = repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inFaceStack))->pop();
      }

      #if PeanoDebug>0
      if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        or
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        or
        inFaceStack==peano4::grid::TraversalObserver::NoData
        or
        not examples::exahype2::euler2D::facedata::euler2DQFluxEstimates::loadPersistently(
          marker
          
        )
      ) {
        logDebug( "loadCell(...)", "initialise meta data of face euler2DQFluxEstimates with " << marker.x(outFaceStackPosition) << " x " << marker.h() << ": load persistent data=" << examples::exahype2::euler2D::facedata::euler2DQFluxEstimates::loadPersistently(
          marker
          
        )  << " (" << marker.toString() << ")" );
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
  // Load cell CellDataeuler2DCellLabel
  {
    auto view = repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( 1 );

    peano4::datamanagement::CellMarker  marker(event);

    const int inCellStack  = event.getCellData();
    const int outCellStack = peano4::grid::PeanoCurve::CallStack;
    logDebug("loadCell(...)", "cell stack " << inCellStack << "->pos-" << outCellStack << "(" << examples::exahype2::euler2D::celldata::euler2DCellLabel::loadPersistently(
      marker
      
    ) << ")" );

    examples::exahype2::euler2D::celldata::euler2DCellLabel& data = view.get(0);
    if (
      inCellStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      inCellStack!=peano4::grid::TraversalObserver::NoData
      and
      (not peano4::grid::PeanoCurve::isInOutStack(inCellStack) or examples::exahype2::euler2D::celldata::euler2DCellLabel::loadPersistently(
        marker
        
      ))
    ) {
      assertion3( not repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inCellStack))->empty(), event.toString(), _spacetreeId, inCellStack);
      data = repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inCellStack))->pop();
    }

    #if PeanoDebug>0
    if (
      inCellStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      or
      inCellStack==peano4::grid::TraversalObserver::NoData
      or
      not examples::exahype2::euler2D::celldata::euler2DCellLabel::loadPersistently(
        marker
        
      )
    ) {
      logDebug( "loadCell(...)", "initialise meta data of cell euler2DCellLabel with " << marker.x() << " x " << marker.h()  << " (" << marker.toString() << ")");
      data.setDebugX( marker.x() );
      data.setDebugH( marker.h() );
    }
    else {
      assertionVectorNumericalEquals4( data.getDebugX(), marker.x(), data.getDebugX(), data.getDebugH(), marker.toString(), _spacetreeId );
      assertionVectorNumericalEquals4( data.getDebugH(), marker.h(), data.getDebugX(), data.getDebugH(), marker.toString(), _spacetreeId );
    }
    #endif
  }
  // Load cell CellDataeuler2DQ
  {
    auto view = repositories::DataRepository::_CellDataeuler2DQStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( 1 );

    peano4::datamanagement::CellMarker  marker(event);

    const int inCellStack  = event.getCellData();
    const int outCellStack = peano4::grid::PeanoCurve::CallStack;
    logDebug("loadCell(...)", "cell stack " << inCellStack << "->pos-" << outCellStack << "(" << examples::exahype2::euler2D::celldata::euler2DQ::loadPersistently(
      marker
      
    ) << ")" );

    examples::exahype2::euler2D::celldata::euler2DQ& data = view.get(0);
    if (
      inCellStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      inCellStack!=peano4::grid::TraversalObserver::NoData
      and
      (not peano4::grid::PeanoCurve::isInOutStack(inCellStack) or examples::exahype2::euler2D::celldata::euler2DQ::loadPersistently(
        marker
        
      ))
    ) {
      assertion3( not repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inCellStack))->empty(), event.toString(), _spacetreeId, inCellStack);
      data = repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inCellStack))->pop();
    }

    #if PeanoDebug>0
    if (
      inCellStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      or
      inCellStack==peano4::grid::TraversalObserver::NoData
      or
      not examples::exahype2::euler2D::celldata::euler2DQ::loadPersistently(
        marker
        
      )
    ) {
      logDebug( "loadCell(...)", "initialise meta data of cell euler2DQ with " << marker.x() << " x " << marker.h()  << " (" << marker.toString() << ")");
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


void examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::enterCell( const peano4::grid::GridTraversalEvent&  event ) {
  logTraceInWith2Arguments( "enterCell(...)", _spacetreeId, event.toString() );
  // Handle face 
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
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet0.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet0.createHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet0.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
    }
  }
  // Handle face 
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
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet1.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet1.createHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet1.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
    }
  }
  // Handle face 
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
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet2.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet2.createHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet2.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
    }
  }
  // Handle face 
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
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet3.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet3.createHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet3.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        
        peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),
        repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )
        ,repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        
        peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),
        repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )
        ,repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
      _actionSet2.createCell(
        marker,
        
        peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),
        repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )
        ,repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
      _actionSet3.createCell(
        marker,
        
        peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),
        repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )
        ,repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet0.touchCellFirstTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet1.touchCellFirstTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet2.touchCellFirstTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet3.touchCellFirstTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  logTraceOut( "enterCell(...)" );
}


void examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::leaveCell( const peano4::grid::GridTraversalEvent&  event ) {
  logTraceInWith2Arguments( "leaveCell(...)", _spacetreeId, event.toString() );
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet3.touchCellLastTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet2.touchCellLastTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet1.touchCellLastTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet0.touchCellLastTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )
        ,repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )
        ,repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )
        ,repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )
        ,repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  // Handle face 
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
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet3.destroyPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet3.destroyHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet3.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
    }
  }
  // Handle face 
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
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet2.destroyPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet2.destroyHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet2.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
    }
  }
  // Handle face 
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
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet1.destroyPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet1.destroyHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet1.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
    }
  }
  // Handle face 
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
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet0.destroyPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet0.destroyHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet0.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel>( &repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates>( &repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates>( &repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
    }
  }
  logTraceOutWith1Argument( "leaveCell(...)", _spacetreeId );
}


void examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::storeCell( const peano4::grid::GridTraversalEvent&  event ) {
  logTraceInWith2Arguments( "storeCell(...)", _spacetreeId, event.toString() );
  // Handle cell CellDataeuler2DQ
  {
    const int inCellStack   = peano4::grid::PeanoCurve::CallStack;
    const int outCellStack  = event.getCellData();
    logDebug("storeCell(...)", "cell stack " << inCellStack << "->pos-" << outCellStack );

    peano4::datamanagement::CellMarker  marker(event);

    auto view = repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( 1 );

    if (
      outCellStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      outCellStack!=peano4::grid::TraversalObserver::NoData
      and
      (not peano4::grid::PeanoCurve::isInOutStack(outCellStack) or view.get(0).storePersistently(
        marker
        
      ))
    ) {
      repositories::DataRepository::_CellDataeuler2DQStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,outCellStack))->push( view.get(0) );
    }
    else {
      logDebug( 
        "storeCell(...)", 
        "do not store cell euler2DQ with " << marker.x() << " x " << marker.h() << ":" << 
        "  destroy=" << (outCellStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity) <<
        ", no-data=" << (outCellStack==peano4::grid::TraversalObserver::NoData) <<
        ", is-in/out=" << peano4::grid::PeanoCurve::isInOutStack(outCellStack) <<
        ", store-persistently=" << view.get(0).storePersistently(
          
          marker
        )
      );
    }
  }
  // Handle cell CellDataeuler2DCellLabel
  {
    const int inCellStack   = peano4::grid::PeanoCurve::CallStack;
    const int outCellStack  = event.getCellData();
    logDebug("storeCell(...)", "cell stack " << inCellStack << "->pos-" << outCellStack );

    peano4::datamanagement::CellMarker  marker(event);

    auto view = repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( 1 );

    if (
      outCellStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      outCellStack!=peano4::grid::TraversalObserver::NoData
      and
      (not peano4::grid::PeanoCurve::isInOutStack(outCellStack) or view.get(0).storePersistently(
        marker
        
      ))
    ) {
      repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,outCellStack))->push( view.get(0) );
    }
    else {
      logDebug( 
        "storeCell(...)", 
        "do not store cell euler2DCellLabel with " << marker.x() << " x " << marker.h() << ":" << 
        "  destroy=" << (outCellStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity) <<
        ", no-data=" << (outCellStack==peano4::grid::TraversalObserver::NoData) <<
        ", is-in/out=" << peano4::grid::PeanoCurve::isInOutStack(outCellStack) <<
        ", store-persistently=" << view.get(0).storePersistently(
          
          marker
        )
      );
    }
  }
  // Store face FaceDataeuler2DQFluxEstimates
  {
    auto view = repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStackPosition  = event.getFaceDataFrom(i);
      int outFaceStack         = event.getFaceDataTo(i);
      logDebug("storeCell(...)", "pos-" << inFaceStackPosition << "->face stack " << outFaceStack );

      peano4::datamanagement::FaceMarker  marker(event,inFaceStackPosition);

      examples::exahype2::euler2D::facedata::euler2DQFluxEstimates data = view.get(inFaceStackPosition);

      if (
        outFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        outFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        outFaceStack!=peano4::grid::TraversalObserver::NoData
        and
        (not peano4::grid::PeanoCurve::isInOutStack(outFaceStack) or data.storePersistently(
          marker
          
        ))
      ) {
        logDebug( "storeCell(...)", "store face euler2DQFluxEstimates with " << marker.x(inFaceStackPosition) << " x " << marker.h() );
        repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,outFaceStack))->push(data);
      }
      else {
        logDebug( "storeCell(...)", "do not store face euler2DQFluxEstimates with " << marker.x(inFaceStackPosition) << " x " << marker.h() << ": store persistently=" << data.storePersistently(
          marker
          
        ) );
      }
    }
  }
  // Store face FaceDataeuler2DQEstimates
  {
    auto view = repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStackPosition  = event.getFaceDataFrom(i);
      int outFaceStack         = event.getFaceDataTo(i);
      logDebug("storeCell(...)", "pos-" << inFaceStackPosition << "->face stack " << outFaceStack );

      peano4::datamanagement::FaceMarker  marker(event,inFaceStackPosition);

      examples::exahype2::euler2D::facedata::euler2DQEstimates data = view.get(inFaceStackPosition);

      if (
        outFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        outFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        outFaceStack!=peano4::grid::TraversalObserver::NoData
        and
        (not peano4::grid::PeanoCurve::isInOutStack(outFaceStack) or data.storePersistently(
          marker
          
        ))
      ) {
        logDebug( "storeCell(...)", "store face euler2DQEstimates with " << marker.x(inFaceStackPosition) << " x " << marker.h() );
        repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,outFaceStack))->push(data);
      }
      else {
        logDebug( "storeCell(...)", "do not store face euler2DQEstimates with " << marker.x(inFaceStackPosition) << " x " << marker.h() << ": store persistently=" << data.storePersistently(
          marker
          
        ) );
      }
    }
  }
  // Store face FaceDataeuler2DFaceLabel
  {
    auto view = repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStackPosition  = event.getFaceDataFrom(i);
      int outFaceStack         = event.getFaceDataTo(i);
      logDebug("storeCell(...)", "pos-" << inFaceStackPosition << "->face stack " << outFaceStack );

      peano4::datamanagement::FaceMarker  marker(event,inFaceStackPosition);

      examples::exahype2::euler2D::facedata::euler2DFaceLabel data = view.get(inFaceStackPosition);

      if (
        outFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        outFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        outFaceStack!=peano4::grid::TraversalObserver::NoData
        and
        (not peano4::grid::PeanoCurve::isInOutStack(outFaceStack) or data.storePersistently(
          marker
          
        ))
      ) {
        logDebug( "storeCell(...)", "store face euler2DFaceLabel with " << marker.x(inFaceStackPosition) << " x " << marker.h() );
        repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,outFaceStack))->push(data);
      }
      else {
        logDebug( "storeCell(...)", "do not store face euler2DFaceLabel with " << marker.x(inFaceStackPosition) << " x " << marker.h() << ": store persistently=" << data.storePersistently(
          marker
          
        ) );
      }
    }
  }
  logTraceOutWith1Argument( "storeCell(...)", _spacetreeId );
}


void examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::exchangeAllVerticalDataExchangeStacks( int masterId ) {
  logTraceInWith2Arguments( "exchangeAllVerticalDataExchangeStacks(...)", masterId, _spacetreeId  );
  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(
    repositories::DataRepository::_CellDataeuler2DCellLabelStack,
    _spacetreeId,
    masterId
  );
  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(
    repositories::DataRepository::_CellDataeuler2DQStack,
    _spacetreeId,
    masterId
  );
  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(
    repositories::DataRepository::_FaceDataeuler2DFaceLabelStack,
    _spacetreeId,
    masterId
  );
  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(
    repositories::DataRepository::_FaceDataeuler2DQEstimatesStack,
    _spacetreeId,
    masterId
  );
  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(
    repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack,
    _spacetreeId,
    masterId
  );
  logTraceOut( "exchangeAllVerticalDataExchangeStacks(...)" );
}


void examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::exchangeAllHorizontalDataExchangeStacks( bool symmetricDataCardinality ) {
  logTraceInWith2Arguments( "exchangeAllHorizontalDataExchangeStacks(...)", symmetricDataCardinality, _spacetreeId  );
  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(
    repositories::DataRepository::_CellDataeuler2DCellLabelStack,
    _spacetreeId,
    symmetricDataCardinality
  );
  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(
    repositories::DataRepository::_CellDataeuler2DQStack,
    _spacetreeId,
    symmetricDataCardinality
  );
  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(
    repositories::DataRepository::_FaceDataeuler2DFaceLabelStack,
    _spacetreeId,
    symmetricDataCardinality
  );
  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(
    repositories::DataRepository::_FaceDataeuler2DQEstimatesStack,
    _spacetreeId,
    symmetricDataCardinality
  );
  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(
    repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack,
    _spacetreeId,
    symmetricDataCardinality
  );
  logTraceOut( "exchangeAllHorizontalDataExchangeStacks(...)" );
}


void examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::exchangeAllPeriodicBoundaryDataStacks() {
  logTraceInWith1Argument( "exchangeAllPeriodicBoundaryDataStacks()", _spacetreeId  );
  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(
    repositories::DataRepository::_CellDataeuler2DCellLabelStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(
    repositories::DataRepository::_CellDataeuler2DQStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(
    repositories::DataRepository::_FaceDataeuler2DFaceLabelStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(
    repositories::DataRepository::_FaceDataeuler2DQEstimatesStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(
    repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack,
    _spacetreeId
  );
  logTraceOut( "exchangeAllPeriodicBoundaryDataStacks()" );
}


void examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::streamDataFromSplittingTreeToNewTree(int newWorker) {
  logTraceInWith2Arguments( "streamDataFromSplittingTreeToNewTree(int)", _spacetreeId, newWorker );
  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(
    repositories::DataRepository::_CellDataeuler2DCellLabelStack,
    _spacetreeId,
    newWorker
  );
  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(
    repositories::DataRepository::_CellDataeuler2DQStack,
    _spacetreeId,
    newWorker
  );
  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(
    repositories::DataRepository::_FaceDataeuler2DFaceLabelStack,
    _spacetreeId,
    newWorker
  );
  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(
    repositories::DataRepository::_FaceDataeuler2DQEstimatesStack,
    _spacetreeId,
    newWorker
  );
  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(
    repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack,
    _spacetreeId,
    newWorker
  );
  logTraceOut( "streamDataFromSplittingTreeToNewTree(int)");
}


void examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::streamDataFromJoiningTreeToMasterTree(int master) {
  logTraceInWith2Arguments( "streamDataFromJoiningTreeToMasterTree(int)", _spacetreeId, master );
  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(
    repositories::DataRepository::_CellDataeuler2DCellLabelStack,
    _spacetreeId,
    master
  );
  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(
    repositories::DataRepository::_CellDataeuler2DQStack,
    _spacetreeId,
    master
  );
  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(
    repositories::DataRepository::_FaceDataeuler2DFaceLabelStack,
    _spacetreeId,
    master
  );
  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(
    repositories::DataRepository::_FaceDataeuler2DQEstimatesStack,
    _spacetreeId,
    master
  );
  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(
    repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack,
    _spacetreeId,
    master
  );
  logTraceOut( "streamDataFromJoiningTreeToMasterTree(int)");
}


void examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::finishAllOutstandingSendsAndReceives() {
  logTraceInWith1Argument( "finishAllOutstandingSendsAndReceives()", _spacetreeId );
  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(
    repositories::DataRepository::_CellDataeuler2DCellLabelStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(
    repositories::DataRepository::_CellDataeuler2DQStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(
    repositories::DataRepository::_FaceDataeuler2DFaceLabelStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(
    repositories::DataRepository::_FaceDataeuler2DQEstimatesStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(
    repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack,
    _spacetreeId
  );
  logTraceOut( "finishAllOutstandingSendsAndReceives()");
}


void examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::sendVertex(int position, int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent&  event) {
  logTraceInWith4Arguments( "sendVertex(int,int,int)", position, toStack, event.toString(), _spacetreeId );
  
  logTraceOut( "sendVertex(int,int,int)");
}


void examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::sendFace(int position, int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent&  event) {
  logTraceInWith4Arguments( "sendFace(int,int,int)", position, toStack, event.toString(), _spacetreeId );

  {
    peano4::datamanagement::FaceMarker  marker(event,position);
    const examples::exahype2::euler2D::facedata::euler2DFaceLabel& data = repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop( 
      repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack)
    )->top(TwoTimesD-1-position);
    if (
      data.storePersistently(marker) 
      and
      data.send(
        marker
        
      )
    ) {
      logDebug( "sendFace(...)", "send out " << data.toString() << " to stack " << toStack << " on tree " << _spacetreeId << " for marker " << marker.toString() );

      repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPush(
        _spacetreeId, toStack
      ) -> push(data);
    }
  }
  {
    peano4::datamanagement::FaceMarker  marker(event,position);
    const examples::exahype2::euler2D::facedata::euler2DQEstimates& data = repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop( 
      repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack)
    )->top(TwoTimesD-1-position);
    if (
      data.storePersistently(marker) 
      and
      data.send(
        marker
        
      )
    ) {
      logDebug( "sendFace(...)", "send out " << data.toString() << " to stack " << toStack << " on tree " << _spacetreeId << " for marker " << marker.toString() );

      repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPush(
        _spacetreeId, toStack
      ) -> push(data);
    }
  }
  {
    peano4::datamanagement::FaceMarker  marker(event,position);
    const examples::exahype2::euler2D::facedata::euler2DQFluxEstimates& data = repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop( 
      repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack)
    )->top(TwoTimesD-1-position);
    if (
      data.storePersistently(marker) 
      and
      data.send(
        marker
        
      )
    ) {
      logDebug( "sendFace(...)", "send out " << data.toString() << " to stack " << toStack << " on tree " << _spacetreeId << " for marker " << marker.toString() );

      repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPush(
        _spacetreeId, toStack
      ) -> push(data);
    }
  }
  logTraceOut( "sendFace(int,int,int)");
}


void examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::sendCell(int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent&  event) {
  logTraceInWith3Arguments( "sendCell(int,int,int)", toStack, event.toString(), _spacetreeId );
  
  {
    peano4::datamanagement::CellMarker  marker(event);
    const examples::exahype2::euler2D::celldata::euler2DCellLabel&  data = repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top();
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

      repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPush(
        _spacetreeId, toStack
      ) -> push(data);
    }
  }
  {
    peano4::datamanagement::CellMarker  marker(event);
    const examples::exahype2::euler2D::celldata::euler2DQ&  data = repositories::DataRepository::_CellDataeuler2DQStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top();
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

      repositories::DataRepository::_CellDataeuler2DQStack.getForPush(
        _spacetreeId, toStack
      ) -> push(data);
    }
  }
  logTraceOut( "sendCell(int,int,int)");
}


void examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::receiveAndMergeCell(int fromStack, peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent& event ) {
  logTraceInWith3Arguments( "receiveAndMergeCell(...)", fromStack, event.toString(), _spacetreeId );
  {
    peano4::datamanagement::CellMarker  marker(event);

    examples::exahype2::euler2D::celldata::euler2DCellLabel& data = repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPush( 
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
      auto   incomingData = repositories::DataRepository::_CellDataeuler2DCellLabelStack.getForPop(
        _spacetreeId, fromStack
      )->pop();

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
  {
    peano4::datamanagement::CellMarker  marker(event);

    examples::exahype2::euler2D::celldata::euler2DQ& data = repositories::DataRepository::_CellDataeuler2DQStack.getForPush( 
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
      auto   incomingData = repositories::DataRepository::_CellDataeuler2DQStack.getForPop(
        _spacetreeId, fromStack
      )->pop();

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


void examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::receiveAndMergeFace(int position, int fromStack, peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent& event) {
  logTraceInWith4Arguments( "receiveAndMergeFace(...)", position, fromStack, event.toString(), _spacetreeId );
  {
    peano4::datamanagement::FaceMarker  marker(event,position);
    examples::exahype2::euler2D::facedata::euler2DFaceLabel& data = repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPush( 
      repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack)
    )->top(TwoTimesD-1-position);

    if ( 
      data.loadPersistently(
        marker
        
      ) 
      and
      data.receiveAndMerge(
        marker
        
      )
    ) {
      auto   incomingData = repositories::DataRepository::_FaceDataeuler2DFaceLabelStack.getForPop(
        _spacetreeId, fromStack
      )->pop();

      logDebug( "receiveAndMergeFace(...)", "merge " << incomingData.toString() << " into " << data.toString() << " within marker " << marker.toString() );

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
  {
    peano4::datamanagement::FaceMarker  marker(event,position);
    examples::exahype2::euler2D::facedata::euler2DQEstimates& data = repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPush( 
      repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack)
    )->top(TwoTimesD-1-position);

    if ( 
      data.loadPersistently(
        marker
        
      ) 
      and
      data.receiveAndMerge(
        marker
        
      )
    ) {
      auto   incomingData = repositories::DataRepository::_FaceDataeuler2DQEstimatesStack.getForPop(
        _spacetreeId, fromStack
      )->pop();

      logDebug( "receiveAndMergeFace(...)", "merge " << incomingData.toString() << " into " << data.toString() << " within marker " << marker.toString() );

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
  {
    peano4::datamanagement::FaceMarker  marker(event,position);
    examples::exahype2::euler2D::facedata::euler2DQFluxEstimates& data = repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPush( 
      repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack)
    )->top(TwoTimesD-1-position);

    if ( 
      data.loadPersistently(
        marker
        
      ) 
      and
      data.receiveAndMerge(
        marker
        
      )
    ) {
      auto   incomingData = repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack.getForPop(
        _spacetreeId, fromStack
      )->pop();

      logDebug( "receiveAndMergeFace(...)", "merge " << incomingData.toString() << " into " << data.toString() << " within marker " << marker.toString() );

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
  logTraceOut( "receiveAndMergeFace(...)");
}


void examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::receiveAndMergeVertex(int position, int fromStack, peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent& event) {
  logTraceInWith4Arguments( "receiveAndMergeVertex(...)", position, fromStack, event.toString(), _spacetreeId );
  logTraceOut( "receiveAndMergeVertex(...)");
}


void examples::exahype2::euler2D::observers::CreateGridButPostponeRefinement::deleteAllStacks() {
  logTraceInWith1Argument( "deleteAllStacks()", _spacetreeId );
  peano4::parallel::SpacetreeSet::deleteAllStacks(
    repositories::DataRepository::_CellDataeuler2DCellLabelStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::deleteAllStacks(
    repositories::DataRepository::_CellDataeuler2DQStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::deleteAllStacks(
    repositories::DataRepository::_FaceDataeuler2DFaceLabelStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::deleteAllStacks(
    repositories::DataRepository::_FaceDataeuler2DQEstimatesStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::deleteAllStacks(
    repositories::DataRepository::_FaceDataeuler2DQFluxEstimatesStack,
    _spacetreeId
  );
  logTraceOut( "deleteAllStacks()");
}

