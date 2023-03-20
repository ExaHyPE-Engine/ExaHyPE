
#include "InitGrid.h"
#include "repositories/DataRepository.h"

#include "peano4/grid/PeanoCurve.h"

#include "peano4/datamanagement/VertexEnumerator.h"
#include "peano4/datamanagement/VertexMarker.h"
#include "peano4/datamanagement/FaceEnumerator.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "peano4/datamanagement/CellMarker.h"

#include "peano4/parallel/SpacetreeSet.h"


tarch::logging::Log benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid::_log( "benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid" );


benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid::InitGrid(int spacetreeId):
  _spacetreeId( spacetreeId ) , _actionSet0(spacetreeId), _actionSet1(spacetreeId), _actionSet2(spacetreeId), _actionSet3(spacetreeId), _actionSet4(spacetreeId), _actionSet5(spacetreeId), _actionSet6(spacetreeId), _actionSet7(spacetreeId), _actionSet8(spacetreeId)
{}


  

peano4::grid::TraversalObserver* benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid::clone(int spacetreeId) {
  return new InitGrid(spacetreeId);
}

  

std::vector< peano4::grid::GridControlEvent > benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid::getGridControlEvents() const {
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
  {
    const std::vector< peano4::grid::GridControlEvent > actionResult = _actionSet8.getGridControlEvents();
    result.insert(result.begin(),actionResult.begin(),actionResult.end());
  }
  return result;
}



void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid::beginTraversal( const tarch::la::Vector<Dimensions,double>&  x, const tarch::la::Vector<Dimensions,double>&  h ) {
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
  _actionSet8.beginTraversal();


  //
  // Fill call stacks with dummies which represent level 0 such that we can
  // call standard action routines on level 1 with parents. Without these
  // statements, a top(1) call would raise an assertion.
  //
  repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q() );
  repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates() );
  repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel() );
  for (int i=0; i<TwoTimesD; i++) {
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld() );
  }
  for (int i=0; i<TwoTimesD; i++) {
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew() );
  }
  for (int i=0; i<TwoTimesD; i++) {
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate() );
  }
  for (int i=0; i<TwoTimesD; i++) {
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->push( benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel() );
  }

  logTraceOutWith2Arguments( "beginTraversal(...)", x, h );
}

  

void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid::endTraversal( const tarch::la::Vector<Dimensions,double>&  x, const tarch::la::Vector<Dimensions,double>&  h ) {
  logTraceInWith2Arguments( "endTraversal(...)", x, h );
    _actionSet0.endTraversal();
  _actionSet1.endTraversal();
  _actionSet2.endTraversal();
  _actionSet3.endTraversal();
  _actionSet4.endTraversal();
  _actionSet5.endTraversal();
  _actionSet6.endTraversal();
  _actionSet7.endTraversal();
  _actionSet8.endTraversal();

    repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();
  repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();
  repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();
  for (int i=0; i<TwoTimesD; i++) {
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();
  }
  for (int i=0; i<TwoTimesD; i++) {
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();
  }
  for (int i=0; i<TwoTimesD; i++) {
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();
  }
  for (int i=0; i<TwoTimesD; i++) {
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pop();
  }

  logTraceOutWith2Arguments( "endTraversal(...)", x, h );
}

  

void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid::prepareTraversal() {
  logTraceIn( "prepareTraversal(...)" );
    observers::InitGrid2exahype2_solvers_rkfd_actionsets_DynamicAMR0::prepareTraversal();
  observers::InitGrid2peano4_toolbox_blockstructured_BackupPatchOverlap1::prepareTraversal();
  observers::InitGrid2exahype2_solvers_rkfd_actionsets_RollOverUpdatedFace2::prepareTraversal();
  observers::InitGrid2exahype2_solvers_rkfd_actionsets_InitialCondition3::prepareTraversal();
  observers::InitGrid2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces4::prepareTraversal();
  observers::InitGrid2exahype2_grid_FaceLabel5::prepareTraversal();
  observers::InitGrid2exahype2_grid_CellLabel6::prepareTraversal();
  observers::InitGrid2exahype2_solvers_rkfd_actionsets_AdaptivityCriterion7::prepareTraversal();
  observers::InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8::prepareTraversal();

  logTraceOut( "prepareTraversal(...)" );
}

  

void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid::unprepareTraversal() {
  logTraceIn( "unprepareTraversal(...)" );
    observers::InitGrid2exahype2_solvers_rkfd_actionsets_DynamicAMR0::unprepareTraversal();
  observers::InitGrid2peano4_toolbox_blockstructured_BackupPatchOverlap1::unprepareTraversal();
  observers::InitGrid2exahype2_solvers_rkfd_actionsets_RollOverUpdatedFace2::unprepareTraversal();
  observers::InitGrid2exahype2_solvers_rkfd_actionsets_InitialCondition3::unprepareTraversal();
  observers::InitGrid2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces4::unprepareTraversal();
  observers::InitGrid2exahype2_grid_FaceLabel5::unprepareTraversal();
  observers::InitGrid2exahype2_grid_CellLabel6::unprepareTraversal();
  observers::InitGrid2exahype2_solvers_rkfd_actionsets_AdaptivityCriterion7::unprepareTraversal();
  observers::InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8::unprepareTraversal();

  logTraceOut( "unprepareTraversal(...)" );
}

  
void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid::loadCell( const peano4::grid::GridTraversalEvent&  event ) {
  logTraceInWith2Arguments( "loadCell(...)", _spacetreeId, event.toString() );
  // Load face FaceDataSelfSimilarInfallFD4QOld
  {
    auto view = repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStack          = event.getFaceDataFrom(i);
      int outFaceStackPosition = event.getFaceDataTo(i);
      logDebug("loadCell(...)", "face stack " << inFaceStack << "->pos-" << outFaceStackPosition );

      peano4::datamanagement::FaceMarker  marker(event,outFaceStackPosition);

      if (
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::NoData
        and
        (not peano4::grid::PeanoCurve::isInOutStack(inFaceStack) or benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld::loadPersistently(
          marker
          
        ))
      ) {
        assertion4( not repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inFaceStack))->empty(), event.toString(), peano4::datamanagement::FaceMarker(event).toString(), _spacetreeId,inFaceStack );
        benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld data = std::move( repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inFaceStack))->pop() );
        #if PeanoDebug>0
        if ( peano4::grid::PeanoCurve::isInOutStack(inFaceStack) ) {
          assertionVectorNumericalEquals5( data.getDebugX(), marker.x(outFaceStackPosition), data.getDebugX(), data.getDebugH(), marker.toString(), outFaceStackPosition, _spacetreeId );
          assertionVectorNumericalEquals5( data.getDebugH(), marker.h(),                     data.getDebugX(), data.getDebugH(), marker.toString(), outFaceStackPosition, _spacetreeId );
        }
        #endif
        view.set(outFaceStackPosition,data);
      }
      else {
        benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld data;
        #if PeanoDebug>0
        logDebug( "loadCell(...)", "initialise meta data of face SelfSimilarInfallFD4QOld with " << marker.x(outFaceStackPosition) << " x " << marker.h() << ": load persistent data=" << benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld::loadPersistently(
          marker
          
        )  << " (" << marker.toString() << ")" );
        data.setDebugX( marker.x(outFaceStackPosition) );
        data.setDebugH( marker.h() );
        #endif
        view.set(outFaceStackPosition,data);
      }
    }
  }
  // Load face FaceDataSelfSimilarInfallFD4QNew
  {
    auto view = repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStack          = event.getFaceDataFrom(i);
      int outFaceStackPosition = event.getFaceDataTo(i);
      logDebug("loadCell(...)", "face stack " << inFaceStack << "->pos-" << outFaceStackPosition );

      peano4::datamanagement::FaceMarker  marker(event,outFaceStackPosition);

      if (
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::NoData
        and
        (not peano4::grid::PeanoCurve::isInOutStack(inFaceStack) or benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew::loadPersistently(
          marker
          
        ))
      ) {
        assertion4( not repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inFaceStack))->empty(), event.toString(), peano4::datamanagement::FaceMarker(event).toString(), _spacetreeId,inFaceStack );
        benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew data = std::move( repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inFaceStack))->pop() );
        #if PeanoDebug>0
        if ( peano4::grid::PeanoCurve::isInOutStack(inFaceStack) ) {
          assertionVectorNumericalEquals5( data.getDebugX(), marker.x(outFaceStackPosition), data.getDebugX(), data.getDebugH(), marker.toString(), outFaceStackPosition, _spacetreeId );
          assertionVectorNumericalEquals5( data.getDebugH(), marker.h(),                     data.getDebugX(), data.getDebugH(), marker.toString(), outFaceStackPosition, _spacetreeId );
        }
        #endif
        view.set(outFaceStackPosition,data);
      }
      else {
        benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew data;
        #if PeanoDebug>0
        logDebug( "loadCell(...)", "initialise meta data of face SelfSimilarInfallFD4QNew with " << marker.x(outFaceStackPosition) << " x " << marker.h() << ": load persistent data=" << benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew::loadPersistently(
          marker
          
        )  << " (" << marker.toString() << ")" );
        data.setDebugX( marker.x(outFaceStackPosition) );
        data.setDebugH( marker.h() );
        #endif
        view.set(outFaceStackPosition,data);
      }
    }
  }
  // Load face FaceDataSelfSimilarInfallFD4QUpdate
  {
    auto view = repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStack          = event.getFaceDataFrom(i);
      int outFaceStackPosition = event.getFaceDataTo(i);
      logDebug("loadCell(...)", "face stack " << inFaceStack << "->pos-" << outFaceStackPosition );

      peano4::datamanagement::FaceMarker  marker(event,outFaceStackPosition);

      if (
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::NoData
        and
        (not peano4::grid::PeanoCurve::isInOutStack(inFaceStack) or benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::loadPersistently(
          marker
          
        ))
      ) {
        assertion4( not repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inFaceStack))->empty(), event.toString(), peano4::datamanagement::FaceMarker(event).toString(), _spacetreeId,inFaceStack );
        benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate data = std::move( repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inFaceStack))->pop() );
        #if PeanoDebug>0
        if ( peano4::grid::PeanoCurve::isInOutStack(inFaceStack) ) {
          assertionVectorNumericalEquals5( data.getDebugX(), marker.x(outFaceStackPosition), data.getDebugX(), data.getDebugH(), marker.toString(), outFaceStackPosition, _spacetreeId );
          assertionVectorNumericalEquals5( data.getDebugH(), marker.h(),                     data.getDebugX(), data.getDebugH(), marker.toString(), outFaceStackPosition, _spacetreeId );
        }
        #endif
        view.set(outFaceStackPosition,data);
      }
      else {
        benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate data;
        #if PeanoDebug>0
        logDebug( "loadCell(...)", "initialise meta data of face SelfSimilarInfallFD4QUpdate with " << marker.x(outFaceStackPosition) << " x " << marker.h() << ": load persistent data=" << benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::loadPersistently(
          marker
          
        )  << " (" << marker.toString() << ")" );
        data.setDebugX( marker.x(outFaceStackPosition) );
        data.setDebugH( marker.h() );
        #endif
        view.set(outFaceStackPosition,data);
      }
    }
  }
  // Load face FaceDataSelfSimilarInfallFD4FaceLabel
  {
    auto view = repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStack          = event.getFaceDataFrom(i);
      int outFaceStackPosition = event.getFaceDataTo(i);
      logDebug("loadCell(...)", "face stack " << inFaceStack << "->pos-" << outFaceStackPosition );

      peano4::datamanagement::FaceMarker  marker(event,outFaceStackPosition);

      if (
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        inFaceStack!=peano4::grid::TraversalObserver::NoData
        and
        (not peano4::grid::PeanoCurve::isInOutStack(inFaceStack) or benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel::loadPersistently(
          marker
          
        ))
      ) {
        assertion4( not repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inFaceStack))->empty(), event.toString(), peano4::datamanagement::FaceMarker(event).toString(), _spacetreeId,inFaceStack );
        benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel data = std::move( repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inFaceStack))->pop() );
        #if PeanoDebug>0
        if ( peano4::grid::PeanoCurve::isInOutStack(inFaceStack) ) {
          assertionVectorNumericalEquals5( data.getDebugX(), marker.x(outFaceStackPosition), data.getDebugX(), data.getDebugH(), marker.toString(), outFaceStackPosition, _spacetreeId );
          assertionVectorNumericalEquals5( data.getDebugH(), marker.h(),                     data.getDebugX(), data.getDebugH(), marker.toString(), outFaceStackPosition, _spacetreeId );
        }
        #endif
        view.set(outFaceStackPosition,data);
      }
      else {
        benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel data;
        #if PeanoDebug>0
        logDebug( "loadCell(...)", "initialise meta data of face SelfSimilarInfallFD4FaceLabel with " << marker.x(outFaceStackPosition) << " x " << marker.h() << ": load persistent data=" << benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel::loadPersistently(
          marker
          
        )  << " (" << marker.toString() << ")" );
        data.setDebugX( marker.x(outFaceStackPosition) );
        data.setDebugH( marker.h() );
        #endif
        view.set(outFaceStackPosition,data);
      }
    }
  }
  // Load cell CellDataSelfSimilarInfallFD4Q
  {
    auto view = repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( 1 );

    peano4::datamanagement::CellMarker  marker(event);

    const int inCellStack  = event.getCellData();
    const int outCellStack = peano4::grid::PeanoCurve::CallStack;
    logDebug("loadCell(...)", "cell stack " << inCellStack << "->pos-" << outCellStack << "(" << benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::loadPersistently(
      marker
      
    ) << ")" );

    if (
      inCellStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      inCellStack!=peano4::grid::TraversalObserver::NoData
      and
      (not peano4::grid::PeanoCurve::isInOutStack(inCellStack) or benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::loadPersistently(
        marker
        
      ))
    ) {
      assertion3( not repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inCellStack))->empty(), event.toString(), _spacetreeId, inCellStack);
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q data = std::move( repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inCellStack))->pop() );
      
      #if PeanoDebug>0
      assertionVectorNumericalEquals4( data.getDebugX(), marker.x(), data.getDebugX(), data.getDebugH(), marker.toString(), _spacetreeId );
      assertionVectorNumericalEquals4( data.getDebugH(), marker.h(), data.getDebugX(), data.getDebugH(), marker.toString(), _spacetreeId );
      #endif
      
      view.set(0,data);
    }
    else {
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q data;
    
      #if PeanoDebug>0
      logDebug( "loadCell(...)", "initialise meta data of cell SelfSimilarInfallFD4Q with " << marker.x() << " x " << marker.h()  << " (" << marker.toString() << ")");
      data.setDebugX( marker.x() );
      data.setDebugH( marker.h() );
      #endif
      view.set(0,data);
    }
  }
  
  
  
  // Load cell CellDataSelfSimilarInfallFD4QRhsEstimates
  {
    auto view = repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( 1 );

    peano4::datamanagement::CellMarker  marker(event);

    const int inCellStack  = event.getCellData();
    const int outCellStack = peano4::grid::PeanoCurve::CallStack;
    logDebug("loadCell(...)", "cell stack " << inCellStack << "->pos-" << outCellStack << "(" << benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates::loadPersistently(
      marker
      
    ) << ")" );

    if (
      inCellStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      inCellStack!=peano4::grid::TraversalObserver::NoData
      and
      (not peano4::grid::PeanoCurve::isInOutStack(inCellStack) or benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates::loadPersistently(
        marker
        
      ))
    ) {
      assertion3( not repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inCellStack))->empty(), event.toString(), _spacetreeId, inCellStack);
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates data = std::move( repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inCellStack))->pop() );
      
      #if PeanoDebug>0
      assertionVectorNumericalEquals4( data.getDebugX(), marker.x(), data.getDebugX(), data.getDebugH(), marker.toString(), _spacetreeId );
      assertionVectorNumericalEquals4( data.getDebugH(), marker.h(), data.getDebugX(), data.getDebugH(), marker.toString(), _spacetreeId );
      #endif
      
      view.set(0,data);
    }
    else {
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates data;
    
      #if PeanoDebug>0
      logDebug( "loadCell(...)", "initialise meta data of cell SelfSimilarInfallFD4QRhsEstimates with " << marker.x() << " x " << marker.h()  << " (" << marker.toString() << ")");
      data.setDebugX( marker.x() );
      data.setDebugH( marker.h() );
      #endif
      view.set(0,data);
    }
  }
  
  
  
  // Load cell CellDataSelfSimilarInfallFD4CellLabel
  {
    auto view = repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->pushBlock( 1 );

    peano4::datamanagement::CellMarker  marker(event);

    const int inCellStack  = event.getCellData();
    const int outCellStack = peano4::grid::PeanoCurve::CallStack;
    logDebug("loadCell(...)", "cell stack " << inCellStack << "->pos-" << outCellStack << "(" << benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel::loadPersistently(
      marker
      
    ) << ")" );

    if (
      inCellStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      inCellStack!=peano4::grid::TraversalObserver::NoData
      and
      (not peano4::grid::PeanoCurve::isInOutStack(inCellStack) or benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel::loadPersistently(
        marker
        
      ))
    ) {
      assertion3( not repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inCellStack))->empty(), event.toString(), _spacetreeId, inCellStack);
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel data = std::move( repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,inCellStack))->pop() );
      
      #if PeanoDebug>0
      assertionVectorNumericalEquals4( data.getDebugX(), marker.x(), data.getDebugX(), data.getDebugH(), marker.toString(), _spacetreeId );
      assertionVectorNumericalEquals4( data.getDebugH(), marker.h(), data.getDebugX(), data.getDebugH(), marker.toString(), _spacetreeId );
      #endif
      
      view.set(0,data);
    }
    else {
      benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel data;
    
      #if PeanoDebug>0
      logDebug( "loadCell(...)", "initialise meta data of cell SelfSimilarInfallFD4CellLabel with " << marker.x() << " x " << marker.h()  << " (" << marker.toString() << ")");
      data.setDebugX( marker.x() );
      data.setDebugH( marker.h() );
      #endif
      view.set(0,data);
    }
  }
  
  
  
  logTraceOutWith1Argument( "loadCell(...)", _spacetreeId );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid::enterCell( const peano4::grid::GridTraversalEvent&  event ) {
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
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet0.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet0.createHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet0.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet1.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet1.createHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet1.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet2.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet2.createHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet2.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet3.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet3.createHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet3.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        _actionSet4.createPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet4.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet4.createHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet4.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        _actionSet5.createPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet5.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet5.createHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet5.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        _actionSet6.createPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet6.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet6.createHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet6.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        _actionSet7.createPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet7.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet7.createHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet7.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        _actionSet8.createPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet8.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet8.createHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(inFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet8.touchFaceFirstTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        
        peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),
        repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )
        ,repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        
        peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),
        repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )
        ,repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        
        peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),
        repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )
        ,repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        
        peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),
        repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )
        ,repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
      _actionSet4.createCell(
        marker,
        
        peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),
        repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )
        ,repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
      _actionSet5.createCell(
        marker,
        
        peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),
        repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )
        ,repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
      _actionSet6.createCell(
        marker,
        
        peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),
        repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )
        ,repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
      _actionSet7.createCell(
        marker,
        
        peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),
        repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )
        ,repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
      _actionSet8.createCell(
        marker,
        
        peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),
        repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) )
        ,repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet0.touchCellFirstTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet1.touchCellFirstTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet2.touchCellFirstTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet3.touchCellFirstTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet4.touchCellFirstTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet5.touchCellFirstTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet6.touchCellFirstTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet7.touchCellFirstTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet8.touchCellFirstTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  logTraceOut( "enterCell(...)" );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid::leaveCell( const peano4::grid::GridTraversalEvent&  event ) {
  logTraceInWith2Arguments( "leaveCell(...)", _spacetreeId, event.toString() );
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet8.touchCellLastTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet7.touchCellLastTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet6.touchCellLastTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet5.touchCellLastTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet4.touchCellLastTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet3.touchCellLastTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet2.touchCellLastTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet1.touchCellLastTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
      );
    }
  }
  {
    peano4::datamanagement::CellMarker marker( event );
    if (marker.isLocal()) {
      _actionSet0.touchCellLastTime(
         marker
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
    _actionSet8.destroyCell(
      marker
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )
        ,repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )
        ,repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )
        ,repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )
        ,repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )
        ,repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )
        ,repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )
        ,repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )
        ,repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )
        ,repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(0)
        ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        _actionSet8.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet8.destroyPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet8.destroyHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet8.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        _actionSet7.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet7.destroyPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet7.destroyHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet7.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        _actionSet6.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet6.destroyPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet6.destroyHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet6.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        _actionSet5.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet5.destroyPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet5.destroyHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet5.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        _actionSet4.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet4.destroyPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet4.destroyHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet4.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
        _actionSet3.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet3.destroyPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet3.destroyHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet3.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet2.destroyPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet2.destroyHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet2.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet1.destroyPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet1.destroyHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet1.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
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
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
        _actionSet0.destroyPersistentFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity
        and
        marker.isLocal()
      ) {
        _actionSet0.destroyHangingFace(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
      else if (
        peano4::grid::PeanoCurve::isInOutStack(outFaceStack)
        and
        marker.isLocal()
      ) {
        _actionSet0.touchFaceLastTime(
           marker
          
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD-1) )(pick)
          ,peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel>( &repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(TwoTimesD*2-1) ),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1),repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top(1)
        );
      }
    }
  }
  logTraceOutWith1Argument( "leaveCell(...)", _spacetreeId );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::InitGrid::storeCell( const peano4::grid::GridTraversalEvent&  event ) {
  logTraceInWith2Arguments( "storeCell(...)", _spacetreeId, event.toString() );
  // Handle cell CellDataSelfSimilarInfallFD4CellLabel
  {
    const int inCellStack   = peano4::grid::PeanoCurve::CallStack;
    const int outCellStack  = event.getCellData();
    logDebug("storeCell(...)", "cell stack " << inCellStack << "->pos-" << outCellStack );

    peano4::datamanagement::CellMarker  marker(event);

    auto view = repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( 1 );

    if (
      outCellStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      outCellStack!=peano4::grid::TraversalObserver::NoData
      and
      (not peano4::grid::PeanoCurve::isInOutStack(outCellStack) or view.get(0).storePersistently(
        marker
        
      ))
    ) {
      repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,outCellStack))->push( view.get(0) );
    }
    else {
      logDebug( 
        "storeCell(...)", 
        "do not store cell SelfSimilarInfallFD4CellLabel with " << marker.x() << " x " << marker.h() << ":" << 
        "  destroy=" << (outCellStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity) <<
        ", no-data=" << (outCellStack==peano4::grid::TraversalObserver::NoData) <<
        ", is-in/out=" << peano4::grid::PeanoCurve::isInOutStack(outCellStack) <<
        ", store-persistently=" << view.get(0).storePersistently(
          
          marker
        )
      );
    }
  }
  // Handle cell CellDataSelfSimilarInfallFD4QRhsEstimates
  {
    const int inCellStack   = peano4::grid::PeanoCurve::CallStack;
    const int outCellStack  = event.getCellData();
    logDebug("storeCell(...)", "cell stack " << inCellStack << "->pos-" << outCellStack );

    peano4::datamanagement::CellMarker  marker(event);

    auto view = repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( 1 );

    if (
      outCellStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      outCellStack!=peano4::grid::TraversalObserver::NoData
      and
      (not peano4::grid::PeanoCurve::isInOutStack(outCellStack) or view.get(0).storePersistently(
        marker
        
      ))
    ) {
      repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,outCellStack))->push( view.get(0) );
    }
    else {
      logDebug( 
        "storeCell(...)", 
        "do not store cell SelfSimilarInfallFD4QRhsEstimates with " << marker.x() << " x " << marker.h() << ":" << 
        "  destroy=" << (outCellStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity) <<
        ", no-data=" << (outCellStack==peano4::grid::TraversalObserver::NoData) <<
        ", is-in/out=" << peano4::grid::PeanoCurve::isInOutStack(outCellStack) <<
        ", store-persistently=" << view.get(0).storePersistently(
          
          marker
        )
      );
    }
  }
  // Handle cell CellDataSelfSimilarInfallFD4Q
  {
    const int inCellStack   = peano4::grid::PeanoCurve::CallStack;
    const int outCellStack  = event.getCellData();
    logDebug("storeCell(...)", "cell stack " << inCellStack << "->pos-" << outCellStack );

    peano4::datamanagement::CellMarker  marker(event);

    auto view = repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( 1 );

    if (
      outCellStack!=peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity
      and
      outCellStack!=peano4::grid::TraversalObserver::NoData
      and
      (not peano4::grid::PeanoCurve::isInOutStack(outCellStack) or view.get(0).storePersistently(
        marker
        
      ))
    ) {
      repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,outCellStack))->push( view.get(0) );
    }
    else {
      logDebug( 
        "storeCell(...)", 
        "do not store cell SelfSimilarInfallFD4Q with " << marker.x() << " x " << marker.h() << ":" << 
        "  destroy=" << (outCellStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity) <<
        ", no-data=" << (outCellStack==peano4::grid::TraversalObserver::NoData) <<
        ", is-in/out=" << peano4::grid::PeanoCurve::isInOutStack(outCellStack) <<
        ", store-persistently=" << view.get(0).storePersistently(
          
          marker
        )
      );
    }
  }
  // Store face FaceDataSelfSimilarInfallFD4FaceLabel
  {
    auto view = repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStackPosition  = event.getFaceDataFrom(i);
      int outFaceStack         = event.getFaceDataTo(i);
      logDebug("storeCell(...)", "pos-" << inFaceStackPosition << "->face stack " << outFaceStack );

      peano4::datamanagement::FaceMarker  marker(event,inFaceStackPosition);

      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel& data = view.get(inFaceStackPosition);

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
        logDebug( "storeCell(...)", "store face SelfSimilarInfallFD4FaceLabel with " << marker.x(inFaceStackPosition) << " x " << marker.h() );
        repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,outFaceStack))->push(data);
      }
      else {
        logDebug( "storeCell(...)", "do not store face SelfSimilarInfallFD4FaceLabel with " << marker.x(inFaceStackPosition) << " x " << marker.h() << ": store persistently=" << data.storePersistently(
          marker
          
        ) );
      }
    }
  }
  // Store face FaceDataSelfSimilarInfallFD4QUpdate
  {
    auto view = repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStackPosition  = event.getFaceDataFrom(i);
      int outFaceStack         = event.getFaceDataTo(i);
      logDebug("storeCell(...)", "pos-" << inFaceStackPosition << "->face stack " << outFaceStack );

      peano4::datamanagement::FaceMarker  marker(event,inFaceStackPosition);

      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate& data = view.get(inFaceStackPosition);

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
        logDebug( "storeCell(...)", "store face SelfSimilarInfallFD4QUpdate with " << marker.x(inFaceStackPosition) << " x " << marker.h() );
        repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,outFaceStack))->push(data);
      }
      else {
        logDebug( "storeCell(...)", "do not store face SelfSimilarInfallFD4QUpdate with " << marker.x(inFaceStackPosition) << " x " << marker.h() << ": store persistently=" << data.storePersistently(
          marker
          
        ) );
      }
    }
  }
  // Store face FaceDataSelfSimilarInfallFD4QNew
  {
    auto view = repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStackPosition  = event.getFaceDataFrom(i);
      int outFaceStack         = event.getFaceDataTo(i);
      logDebug("storeCell(...)", "pos-" << inFaceStackPosition << "->face stack " << outFaceStack );

      peano4::datamanagement::FaceMarker  marker(event,inFaceStackPosition);

      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew& data = view.get(inFaceStackPosition);

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
        logDebug( "storeCell(...)", "store face SelfSimilarInfallFD4QNew with " << marker.x(inFaceStackPosition) << " x " << marker.h() );
        repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,outFaceStack))->push(data);
      }
      else {
        logDebug( "storeCell(...)", "do not store face SelfSimilarInfallFD4QNew with " << marker.x(inFaceStackPosition) << " x " << marker.h() << ": store persistently=" << data.storePersistently(
          marker
          
        ) );
      }
    }
  }
  // Store face FaceDataSelfSimilarInfallFD4QOld
  {
    auto view = repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->popBlock( TwoTimesD );
    for (int i=0; i<TwoTimesD; i++) {
      int inFaceStackPosition  = event.getFaceDataFrom(i);
      int outFaceStack         = event.getFaceDataTo(i);
      logDebug("storeCell(...)", "pos-" << inFaceStackPosition << "->face stack " << outFaceStack );

      peano4::datamanagement::FaceMarker  marker(event,inFaceStackPosition);

      benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld& data = view.get(inFaceStackPosition);

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
        logDebug( "storeCell(...)", "store face SelfSimilarInfallFD4QOld with " << marker.x(inFaceStackPosition) << " x " << marker.h() );
        repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPush( repositories::DataRepository::DataKey(_spacetreeId,outFaceStack))->push(data);
      }
      else {
        logDebug( "storeCell(...)", "do not store face SelfSimilarInfallFD4QOld with " << marker.x(inFaceStackPosition) << " x " << marker.h() << ": store persistently=" << data.storePersistently(
          marker
          
        ) );
      }
    }
  }
  logTraceOutWith1Argument( "storeCell(...)", _spacetreeId );
}

