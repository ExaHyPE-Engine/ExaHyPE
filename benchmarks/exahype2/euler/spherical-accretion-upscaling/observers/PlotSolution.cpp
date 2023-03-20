
#include "PlotSolution.h"
#include "repositories/DataRepository.h"

#include "peano4/grid/PeanoCurve.h"

#include "peano4/datamanagement/VertexEnumerator.h"
#include "peano4/datamanagement/VertexMarker.h"
#include "peano4/datamanagement/FaceEnumerator.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "peano4/datamanagement/CellMarker.h"

#include "peano4/parallel/SpacetreeSet.h"


tarch::logging::Log benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::_log( "benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution" );


benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::PlotSolution(int spacetreeId):
  _spacetreeId( spacetreeId ) , _actionSet0(spacetreeId), _actionSet1(spacetreeId), _actionSet2(spacetreeId), _actionSet3(spacetreeId)
{}


  

peano4::grid::TraversalObserver* benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::clone(int spacetreeId) {
  return new PlotSolution(spacetreeId);
}

  

std::vector< peano4::grid::GridControlEvent > benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::getGridControlEvents() const {
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



void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::beginTraversal( const tarch::la::Vector<Dimensions,double>&  x, const tarch::la::Vector<Dimensions,double>&  h ) {
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

  

void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::endTraversal( const tarch::la::Vector<Dimensions,double>&  x, const tarch::la::Vector<Dimensions,double>&  h ) {
  logTraceInWith2Arguments( "endTraversal(...)", x, h );
    _actionSet0.endTraversal();
  _actionSet1.endTraversal();
  _actionSet2.endTraversal();
  _actionSet3.endTraversal();

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

  

void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::prepareTraversal() {
  logTraceIn( "prepareTraversal(...)" );
    observers::PlotSolution2exahype2_solvers_rkfd_actionsets_DynamicAMR0::prepareTraversal();
  observers::PlotSolution2exahype2_grid_FaceLabel1::prepareTraversal();
  observers::PlotSolution2exahype2_grid_CellLabel2::prepareTraversal();
  observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3::prepareTraversal();

  logTraceOut( "prepareTraversal(...)" );
}

  

void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::unprepareTraversal() {
  logTraceIn( "unprepareTraversal(...)" );
    observers::PlotSolution2exahype2_solvers_rkfd_actionsets_DynamicAMR0::unprepareTraversal();
  observers::PlotSolution2exahype2_grid_FaceLabel1::unprepareTraversal();
  observers::PlotSolution2exahype2_grid_CellLabel2::unprepareTraversal();
  observers::PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3::unprepareTraversal();

  logTraceOut( "unprepareTraversal(...)" );
}

  
void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::loadCell( const peano4::grid::GridTraversalEvent&  event ) {
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::enterCell( const peano4::grid::GridTraversalEvent&  event ) {
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
  logTraceOut( "enterCell(...)" );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::leaveCell( const peano4::grid::GridTraversalEvent&  event ) {
  logTraceInWith2Arguments( "leaveCell(...)", _spacetreeId, event.toString() );
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::storeCell( const peano4::grid::GridTraversalEvent&  event ) {
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::exchangeAllVerticalDataExchangeStacks( int masterId ) {
  logTraceInWith2Arguments( "exchangeAllVerticalDataExchangeStacks(...)", masterId, _spacetreeId  );
  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(
    repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack,
    _spacetreeId,
    masterId
  );
  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(
    repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack,
    _spacetreeId,
    masterId
  );
  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(
    repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack,
    _spacetreeId,
    masterId
  );
  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack,
    _spacetreeId,
    masterId
  );
  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack,
    _spacetreeId,
    masterId
  );
  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack,
    _spacetreeId,
    masterId
  );
  peano4::parallel::SpacetreeSet::exchangeAllVerticalDataExchangeStacks(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack,
    _spacetreeId,
    masterId
  );
  logTraceOut( "exchangeAllVerticalDataExchangeStacks(...)" );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::exchangeAllHorizontalDataExchangeStacks( bool symmetricDataCardinality ) {
  logTraceInWith2Arguments( "exchangeAllHorizontalDataExchangeStacks(...)", symmetricDataCardinality, _spacetreeId  );
  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(
    repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack,
    _spacetreeId,
    symmetricDataCardinality
  );
  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(
    repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack,
    _spacetreeId,
    symmetricDataCardinality
  );
  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(
    repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack,
    _spacetreeId,
    symmetricDataCardinality
  );
  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack,
    _spacetreeId,
    symmetricDataCardinality
  );
  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack,
    _spacetreeId,
    symmetricDataCardinality
  );
  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack,
    _spacetreeId,
    symmetricDataCardinality
  );
  peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack,
    _spacetreeId,
    symmetricDataCardinality
  );
  logTraceOut( "exchangeAllHorizontalDataExchangeStacks(...)" );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::exchangeAllPeriodicBoundaryDataStacks() {
  logTraceInWith1Argument( "exchangeAllPeriodicBoundaryDataStacks()", _spacetreeId  );
  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(
    repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(
    repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(
    repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack,
    _spacetreeId
  );
  logTraceOut( "exchangeAllPeriodicBoundaryDataStacks()" );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::streamDataFromSplittingTreeToNewTree(int newWorker) {
  logTraceInWith2Arguments( "streamDataFromSplittingTreeToNewTree(int)", _spacetreeId, newWorker );
  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(
    repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack,
    _spacetreeId,
    newWorker
  );
  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(
    repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack,
    _spacetreeId,
    newWorker
  );
  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(
    repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack,
    _spacetreeId,
    newWorker
  );
  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack,
    _spacetreeId,
    newWorker
  );
  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack,
    _spacetreeId,
    newWorker
  );
  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack,
    _spacetreeId,
    newWorker
  );
  peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack,
    _spacetreeId,
    newWorker
  );
  logTraceOut( "streamDataFromSplittingTreeToNewTree(int)");
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::streamDataFromJoiningTreeToMasterTree(int master) {
  logTraceInWith2Arguments( "streamDataFromJoiningTreeToMasterTree(int)", _spacetreeId, master );
  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(
    repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack,
    _spacetreeId,
    master
  );
  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(
    repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack,
    _spacetreeId,
    master
  );
  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(
    repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack,
    _spacetreeId,
    master
  );
  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack,
    _spacetreeId,
    master
  );
  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack,
    _spacetreeId,
    master
  );
  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack,
    _spacetreeId,
    master
  );
  peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack,
    _spacetreeId,
    master
  );
  logTraceOut( "streamDataFromJoiningTreeToMasterTree(int)");
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::finishAllOutstandingSendsAndReceives() {
  logTraceInWith1Argument( "finishAllOutstandingSendsAndReceives()", _spacetreeId );
  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(
    repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(
    repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(
    repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack,
    _spacetreeId
  );
  logTraceOut( "finishAllOutstandingSendsAndReceives()");
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::sendVertex(int position, int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent&  event) {
  logTraceInWith4Arguments( "sendVertex(int,int,int)", position, toStack, event.toString(), _spacetreeId );
  
  logTraceOut( "sendVertex(int,int,int)");
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::sendFace(int position, int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent&  event) {
  logTraceInWith4Arguments( "sendFace(int,int,int)", position, toStack, event.toString(), _spacetreeId );

  {
    peano4::datamanagement::FaceMarker  marker(event,position);
    const benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld& data = repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop( 
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

      repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPush(
        _spacetreeId, toStack
      ) -> push(data);
    }
  }
  {
    peano4::datamanagement::FaceMarker  marker(event,position);
    const benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew& data = repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop( 
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

      repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPush(
        _spacetreeId, toStack
      ) -> push(data);
    }
  }
  {
    peano4::datamanagement::FaceMarker  marker(event,position);
    const benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate& data = repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop( 
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

      repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPush(
        _spacetreeId, toStack
      ) -> push(data);
    }
  }
  {
    peano4::datamanagement::FaceMarker  marker(event,position);
    const benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel& data = repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop( 
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

      repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPush(
        _spacetreeId, toStack
      ) -> push(data);
    }
  }
  logTraceOut( "sendFace(int,int,int)");
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::sendCell(int toStack, ::peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent&  event) {
  logTraceInWith3Arguments( "sendCell(int,int,int)", toStack, event.toString(), _spacetreeId );
  
  {
    peano4::datamanagement::CellMarker  marker(event);
    const benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q&  data = repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top();
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

      repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPush(
        _spacetreeId, toStack
      ) -> push(data);
    }
  }
  {
    peano4::datamanagement::CellMarker  marker(event);
    const benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates&  data = repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top();
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

      repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPush(
        _spacetreeId, toStack
      ) -> push(data);
    }
  }
  {
    peano4::datamanagement::CellMarker  marker(event);
    const benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel&  data = repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop( repositories::DataRepository::DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack))->top();
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

      repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPush(
        _spacetreeId, toStack
      ) -> push(data);
    }
  }
  logTraceOut( "sendCell(int,int,int)");
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::receiveAndMergeCell(int fromStack, peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent& event ) {
  logTraceInWith3Arguments( "receiveAndMergeCell(...)", fromStack, event.toString(), _spacetreeId );
  {
    peano4::datamanagement::CellMarker  marker(event);

    benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& data = repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPush( 
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
      auto   incomingData = std::move( repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack.getForPop(
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
  {
    peano4::datamanagement::CellMarker  marker(event);

    benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4QRhsEstimates& data = repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPush( 
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
      auto   incomingData = std::move( repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack.getForPop(
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
  {
    peano4::datamanagement::CellMarker  marker(event);

    benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4CellLabel& data = repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPush( 
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
      auto   incomingData = std::move( repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack.getForPop(
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::receiveAndMergeFace(int position, int fromStack, peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent& event) {
  logTraceInWith4Arguments( "receiveAndMergeFace(...)", position, fromStack, event.toString(), _spacetreeId );
  {
    peano4::datamanagement::FaceMarker  marker(event,position);
    benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QOld& data = repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPush( 
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
      auto   incomingData = std::move( repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack.getForPop(
        _spacetreeId, fromStack
      )->pop() );

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
    benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QNew& data = repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPush( 
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
      auto   incomingData = std::move( repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack.getForPop(
        _spacetreeId, fromStack
      )->pop() );

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
    benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate& data = repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPush( 
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
      auto   incomingData = std::move( repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack.getForPop(
        _spacetreeId, fromStack
      )->pop() );

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
    benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4FaceLabel& data = repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPush( 
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
      auto   incomingData = std::move( repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack.getForPop(
        _spacetreeId, fromStack
      )->pop() );

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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::receiveAndMergeVertex(int position, int fromStack, peano4::grid::TraversalObserver::SendReceiveContext context, const peano4::grid::GridTraversalEvent& event) {
  logTraceInWith4Arguments( "receiveAndMergeVertex(...)", position, fromStack, event.toString(), _spacetreeId );
  logTraceOut( "receiveAndMergeVertex(...)");
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::observers::PlotSolution::deleteAllStacks() {
  logTraceInWith1Argument( "deleteAllStacks()", _spacetreeId );
  peano4::parallel::SpacetreeSet::deleteAllStacks(
    repositories::DataRepository::_CellDataSelfSimilarInfallFD4QStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::deleteAllStacks(
    repositories::DataRepository::_CellDataSelfSimilarInfallFD4QRhsEstimatesStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::deleteAllStacks(
    repositories::DataRepository::_CellDataSelfSimilarInfallFD4CellLabelStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::deleteAllStacks(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QOldStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::deleteAllStacks(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QNewStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::deleteAllStacks(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4QUpdateStack,
    _spacetreeId
  );
  peano4::parallel::SpacetreeSet::deleteAllStacks(
    repositories::DataRepository::_FaceDataSelfSimilarInfallFD4FaceLabelStack,
    _spacetreeId
  );
  logTraceOut( "deleteAllStacks()");
}

