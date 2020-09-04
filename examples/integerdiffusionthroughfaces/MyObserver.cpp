#include "MyObserver.h"

#include "MyMapping.h"
#include "CompositeMapping.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "peano4/grid/PeanoCurve.h"
#include "peano4/parallel/Node.h"
#include "VTUCellDataPlotter.h"
#include "PeanoFormatCellDataPlotter.h"


/**
 * @todo docu
 */
//#pragma GCC diagnostic warning "-Wpragmas"
//#pragma GCC diagnostic ignored "-fpermissive"
//#pragma warning (disable: #num of the warning)

tarch::logging::Log  examples::integerdiffusionthroughfaces::MyObserver::_log( "examples::integerdiffusionthroughfaces::MyObserver" );


std::map< examples::integerdiffusionthroughfaces::MyObserver::DataKey, examples::integerdiffusionthroughfaces::MyObserver::FaceDataContainer >   examples::integerdiffusionthroughfaces::MyObserver::_faceData;
std::map< examples::integerdiffusionthroughfaces::MyObserver::DataKey, examples::integerdiffusionthroughfaces::MyObserver::CellDataContainer >   examples::integerdiffusionthroughfaces::MyObserver::_cellData;


examples::integerdiffusionthroughfaces::MyObserver::MyObserver():
  _spacetreeId(-1),
  _iterationCounter(-1),
  _mapping(nullptr) {
  #if PeanoDebug>0
  CompositeMapping* mapping = new CompositeMapping();
//  mapping->append( new PeanoFormatCellDataPlotter("marker",true));
  mapping->append( new MyMapping() );
//  mapping->append( new VTUCellDataPlotter("data",false) );
  mapping->append( new PeanoFormatCellDataPlotter("marker",false));
  _mapping = mapping;
  #else
  _mapping = new MyMapping();
  #endif
}


examples::integerdiffusionthroughfaces::MyObserver::MyObserver(int spacetreeId, int counter, Mapping* mapping):
  _spacetreeId(spacetreeId),
  _iterationCounter(counter),
  _mapping(mapping) {
}


examples::integerdiffusionthroughfaces::MyObserver::~MyObserver() {
  if (_spacetreeId<0) {
	assertion( _mapping!=nullptr );
    delete _mapping;
    _mapping = nullptr;
  }
}


void examples::integerdiffusionthroughfaces::MyObserver::beginTraversal(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h
) {
  assertion( _mapping!=nullptr );

  _iterationCounter++;
  _mapping->beginTraversal();

  int outCellStack = peano4::grid::PeanoCurve::CallStack;
  CellData data;
  #if PeanoDebug>=1
  data.x = x;
  data.h = h;
  #endif
  _cellData[ DataKey(_spacetreeId,outCellStack) ].push(data);
  assertion(false);
  // @todo
  // _facesCallStack.push( MyMapping::Faces(event) );
}


//
// @todo
//
// LOP: Wo ist Skeleton/Enclave Marker?
//      Wie identifizeren fuer parallelen Code lokale vs. andere Vertices, Faces und Zellen
//
//

void examples::integerdiffusionthroughfaces::MyObserver::endTraversal(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h
) {
  int cellStack = peano4::grid::PeanoCurve::CallStack;
  _cellData[ DataKey(_spacetreeId,cellStack) ].pop();

  _facesCallStack.pop();

  #if PeanoDebug>4
  for( auto& p: _cellData) {
	logDebug( "endTraversal()", "cell stack (" << p.first.first << "," << p.first.second << "): " << p.second.size() << " entries" );
  }
  #endif
  assertion( _mapping!=nullptr );

  _mapping->endTraversal();
}


void examples::integerdiffusionthroughfaces::MyObserver::enterCell(
  const peano4::grid::GridTraversalEvent&  event
) {
  logTraceInWith1Argument("enterCell(...)",event.toString());

  assertion(false);

  /*
  // @todo
  // Vertex processing -> nop here
  // =============================

  MyMapping::Faces fineGridFaces(event);
  MyMapping::Faces coarseGridFaces(event);

  // Face processing
  // ===============
  // @todo If there were vertices, then we would pass in vertices via an enumerator
  auto faceView = _faceData[ DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack) ].pushBlock(Dimensions*2);
  for (int i=0; i<Dimensions*2; i++) {
    int inFaceStack  = event.getFaceDataFrom(i);
	int outFaceStack = event.getFaceDataTo(i);
    FaceData data;

	logDebug("enterCell(...)", "face " << inFaceStack << "->pos-" << outFaceStack );
    if (inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity) {
      _mapping->createPersistentFace(fineGridFaces.x(outFaceStack),event.getH(),fineGridFaces.normal(outFaceStack),data);
      _mapping->touchFaceFirstTime(fineGridFaces.x(outFaceStack),event.getH(),fineGridFaces.normal(outFaceStack),data);
      #if PeanoDebug>=1
      data.x = fineGridFaces.x(outFaceStack);
      data.h = event.getH();
      #endif
    }
    else if (inFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity) {
      _mapping->createHangingFace(fineGridFaces.x(outFaceStack),event.getH(),fineGridFaces.normal(outFaceStack),data);
      _mapping->touchFaceFirstTime(fineGridFaces.x(outFaceStack),event.getH(),fineGridFaces.normal(outFaceStack),data);
      #if PeanoDebug>=1
      data.x = fineGridFaces.x(outFaceStack);
      data.h = event.getH();
      #endif
    }
    else {
      data = _faceData[ DataKey(_spacetreeId,inFaceStack) ].pop();
      assertionVectorNumericalEquals3(data.x,fineGridFaces.x(outFaceStack),event.toString(),fineGridFaces.toString(),data.x );
      assertionVectorNumericalEquals3(data.h,event.getH(),event.toString(),fineGridFaces.toString(),data.x );
      if (peano4::grid::PeanoCurve::isInOutStack(inFaceStack)) {
        _mapping->touchFaceFirstTime(fineGridFaces.x(outFaceStack),event.getH(),fineGridFaces.normal(outFaceStack),data);
      }
    }
    FaceData* pointer = faceView.set(outFaceStack,data);
    fineGridFaces.setPointer(outFaceStack, pointer );
  }
  _facesCallStack.push( fineGridFaces );

  // Cell processing
  // ===============
  int inCellStack  = event.getCellData();
  int outCellStack = peano4::grid::PeanoCurve::CallStack;
  logDebug("enterCell(...)", "cell " << inCellStack << "->pos-" << outCellStack );
  if (inCellStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity) {
    CellData data;
	  // @todo coarse info
    _mapping->createCell(
      event.getX(),event.getH(),data,
		_facesCallStack.top(0),
	    _cellData[ DataKey(_spacetreeId,outCellStack) ].top(1),
		_facesCallStack.top(1)
    );
    #if PeanoDebug>=1
    data.x = event.getX();
    data.h = event.getH();
    #endif
    assertionVectorNumericalEquals4(data.x,event.getX(),data.value,event.toString(),data.x,data.h);
    assertionVectorNumericalEquals4(data.h,event.getH(),data.value,event.toString(),data.x,data.h);
    _cellData[ DataKey(_spacetreeId,outCellStack) ].push(data);
  }
  else {
    CellData data;
    data = _cellData[ DataKey(_spacetreeId,inCellStack) ].pop();
    assertionVectorNumericalEquals4(data.x,event.getX(),data.value,event.toString(),data.x,data.h);
    assertionVectorNumericalEquals4(data.h,event.getH(),data.value,event.toString(),data.x,data.h);
    _cellData[ DataKey(_spacetreeId,outCellStack) ].push(data);
  }

  assertionNumericalEquals2(
    _cellData[ DataKey(_spacetreeId,outCellStack) ].top(0).h(0)*3.0,
	_cellData[ DataKey(_spacetreeId,outCellStack) ].top(1).h(0),
    _cellData[ DataKey(_spacetreeId,outCellStack) ].top(0).h,
	_cellData[ DataKey(_spacetreeId,outCellStack) ].top(1).h
  );
  _mapping->touchCellFirstTime(
    event.getX(), event.getH(),
    _cellData[ DataKey(_spacetreeId,outCellStack) ].top(0),
	_facesCallStack.top(0),
    _cellData[ DataKey(_spacetreeId,outCellStack) ].top(1),
	_facesCallStack.top(1)
  );
  */

  logTraceOutWith1Argument("enterCell(...)",event.toString());
}


void examples::integerdiffusionthroughfaces::MyObserver::leaveCell(
  const peano4::grid::GridTraversalEvent&  event
) {
  logTraceInWith1Argument("leaveCell(...)",event.toString());

  assertion(false);

  /*
  // @todo Es gibt noch kein inside/outside hier, oder?
  // @todo Enclaves fehlen halt auch noch
  int inCellStack   = peano4::grid::PeanoCurve::CallStack;
  int outCellStack  = event.getCellData();

  _mapping->touchCellLastTime(
    event.getX(), event.getH(),
    _cellData[ DataKey(_spacetreeId,inCellStack) ].top(0),
	_facesCallStack.top(0),
    _cellData[ DataKey(_spacetreeId,inCellStack) ].top(1),
	_facesCallStack.top(1)
  );

  logDebug("leaveCell(...)", "cell " << inCellStack << "->" << outCellStack );
  CellData data = _cellData[ DataKey(_spacetreeId,inCellStack) ].pop();
  assertionVectorNumericalEquals4(data.x,event.getX(),data.value,data.x,data.h,event.toString());
  assertionVectorNumericalEquals4(data.h,event.getH(),data.value,data.x,data.h,event.toString());

  if (outCellStack==TraversalObserver::CreateOrDestroyPersistentGridEntity) {
	  // @todo update
    _mapping->destroyCell(
      event.getX(),event.getH(),data,
      _facesCallStack.top(0),
      _cellData[ DataKey(_spacetreeId,outCellStack) ].top(1),
      _facesCallStack.top(1)
    );
  }
  else {
    _cellData[ DataKey(_spacetreeId,outCellStack) ].push(data);
  }

  auto faceView = _faceData[ DataKey(_spacetreeId,peano4::grid::PeanoCurve::CallStack) ].popBlock(Dimensions*2);
  for (int i=0; i<Dimensions*2; i++) {
    int inFaceStack  = event.getFaceDataFrom(i);
	int outFaceStack = event.getFaceDataTo(i);

	FaceData data = faceView.get(inFaceStack);
	logDebug("leaveCell(...)", "face pos-" << inFaceStack << "->" << outFaceStack << ": " << data.x << "x" << data.h << "x" << data.value );

    tarch::la::Vector<Dimensions,double> x = event.getX();
    int normal = inFaceStack % Dimensions;
    x(normal) += inFaceStack >= Dimensions ? event.getH(normal)/2.0 : -event.getH(normal)/2.0;

    assertionVectorNumericalEquals4(data.x,x,event.toString(),normal,data.x,x );
    assertionVectorNumericalEquals4(data.h,event.getH(),event.toString(),normal,data.x,x );

    if (outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyPersistentGridEntity) {
      _mapping->touchFaceLastTime(_facesCallStack.top(0).x(outFaceStack),event.getH(),_facesCallStack.top(0).normal(outFaceStack),data);
      _mapping->destroyPersistentFace(_facesCallStack.top(0).x(outFaceStack),event.getH(),_facesCallStack.top(0).normal(outFaceStack),data);
    }
    else if (outFaceStack==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity) {
      _mapping->touchFaceLastTime(_facesCallStack.top(0).x(outFaceStack),event.getH(),_facesCallStack.top(0).normal(outFaceStack),data);
      _mapping->destroyHangingFace(_facesCallStack.top(0).x(outFaceStack),event.getH(),_facesCallStack.top(0).normal(outFaceStack),data);
    }
    else {
      if (peano4::grid::PeanoCurve::isInOutStack(outFaceStack)) {
        _mapping->touchFaceLastTime(_facesCallStack.top(0).x(outFaceStack),event.getH(),_facesCallStack.top(0).normal(outFaceStack),data);
      }
      _faceData[ DataKey(_spacetreeId,outFaceStack) ].push(data);
    }
  }

  _facesCallStack.pop();
  */

  logTraceOutWith1Argument("leaveCell(...)",event.toString());
}


peano4::grid::TraversalObserver* examples::integerdiffusionthroughfaces::MyObserver::clone(int spacetreeId) {
  return new MyObserver( spacetreeId, _iterationCounter, _mapping );
}


std::vector< peano4::grid::GridControlEvent > examples::integerdiffusionthroughfaces::MyObserver::getGridControlEvents() {
	// @todoo Das sollte natuerlich auch in mapping raus und muss dann dort u.U. gemerged werden
  std::vector< peano4::grid::GridControlEvent >  controlEvents;
  if (_iterationCounter<8) {
    peano4::grid::GridControlEvent newEvent;
    newEvent.setRefinementControl( peano4::grid::GridControlEvent::RefinementControl::Refine );
#if Dimensions==2
    newEvent.setOffset( {0.0,0.0} );
    newEvent.setWidth( {0.5,0.5} );
    newEvent.setH( {0.02,0.02} );
#elif Dimensions==3
    newEvent.setOffset( {0.0,0.0,0.0} );
    newEvent.setWidth( {0.5,0.5,0.5} );
    newEvent.setH( {0.02,0.02,0.02} );
#endif
    controlEvents.push_back(newEvent);
  }

  // @todo rein
/*
  if (_iterationCounter>12) {
    peano4::grid::GridControlEvent newEvent;
	newEvent.setRefinementControl( peano4::grid::GridControlEvent::RefinementControl::Erase );
#if Dimensions==2
	newEvent.setOffset( {0.01,0.01} );
	newEvent.setWidth( {0.99,0.99} );
	newEvent.setH( {0.4,0.4} );
#elif Dimensions==3
	newEvent.setOffset( {0.01,0.01,0.01} );
	newEvent.setWidth( {0.99,0.99,0.99} );
	newEvent.setH( {0.4,0.4,0.4} );
#endif
    controlEvents.push_back(newEvent);
  }
*/

  return controlEvents;
}
