#include "MyObserver.h"

#include "InitData.h"
#include "CompositeMapping.h"
#include "PeanoFormatCellDataPlotter.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "peano4/grid/PeanoCurve.h"
#include "peano4/parallel/Node.h"
#include "peano4/parallel/SpacetreeSet.h"



/**
 * @todo docu
 */
//#pragma GCC diagnostic warning "-Wpragmas"
//#pragma GCC diagnostic ignored "-fpermissive"
//#pragma warning (disable: #num of the warning)

tarch::logging::Log  examples::delta::MyObserver::_log( "examples::delta::MyObserver" );


std::map< examples::delta::MyObserver::DataKey, examples::delta::MyObserver::CellDataContainer >   examples::delta::MyObserver::_cellData;


examples::delta::MyObserver::MyObserver():
  _spacetreeId(-1),
  _iterationCounter(-1),
  _mapping(nullptr) {
//  #if PeanoDebug>0
  CompositeMapping* mapping = new CompositeMapping();
  mapping->append( new InitData() );
  mapping->append( new PeanoFormatCellDataPlotter("marker",false));
  _mapping = mapping;
/*
  #else
  _mapping = new InitData();
  #endif
*/
}


examples::delta::MyObserver::MyObserver(int spacetreeId, int counter, Mapping* mapping):
  _spacetreeId(spacetreeId),
  _iterationCounter(counter),
  _mapping(mapping) {
}


examples::delta::MyObserver::~MyObserver() {
  if (_spacetreeId<0) {
	assertion( _mapping!=nullptr );
    delete _mapping;
    _mapping = nullptr;
  }
}


void examples::delta::MyObserver::beginTraversal(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h
) {
  logTraceInWith2Arguments("beginTraversal(...)",x,h);

  assertion( _mapping!=nullptr );

  _iterationCounter++;
  _mapping->beginTraversal();

  int outCellStack = peano4::grid::PeanoCurve::CallStack;
  CellData data;
  #if PeanoDebug>=1
  data.x = x;
  data.h = h;
  data.spacetreeId = _spacetreeId;
  #endif
  _cellData[ DataKey(_spacetreeId,outCellStack) ].push(data);

  logTraceOutWith2Arguments("beginTraversal(...)",x,h);
}


//
// @todo
//
// LOP: Wo ist Skeleton/Enclave Marker?
//      Wie identifizeren fuer parallelen Code lokale vs. andere Vertices, Faces und Zellen
//
//




void examples::delta::MyObserver::endTraversal(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h
) {
  logTraceInWith2Arguments("endTraversal(...)",x,h);

  int cellStack = peano4::grid::PeanoCurve::CallStack;
  _cellData[ DataKey(_spacetreeId,cellStack) ].pop();

  #if PeanoDebug>4
  for( auto& p: _cellData) {
    logDebug( "endTraversal()", "cell stack (" << p.first.first << "," << p.first.second << "): " << p.second.size() << " entries" );
  }
  #endif
  assertion( _mapping!=nullptr );

  _mapping->endTraversal();

  logTraceOutWith2Arguments("endTraversal(...)",x,h);
}


void examples::delta::MyObserver::enterCell(
  const peano4::grid::GridTraversalEvent&  event
) {
  logTraceInWith1Argument("enterCell(...)",event.toString());

/*
  assertion2(
    event.getCellState()==peano4::grid::GridTraversalEvent::CellState::Local,
    event.toString(), "enterCell()"
  );
*/

  // @todo
  // Vertex processing -> nop here
  // =============================
  // @todo
  // Faces processing -> nop here
  // =============================


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
      _cellData[ DataKey(_spacetreeId,outCellStack) ].top(1)
    );
    #if PeanoDebug>=1
    data.x = event.getX();
    data.h = event.getH();
    data.spacetreeId = _spacetreeId;
    #endif
    assertionVectorNumericalEquals3(data.x,event.getX(),event.toString(),data.x,data.h);
    assertionVectorNumericalEquals3(data.h,event.getH(),event.toString(),data.x,data.h);
    _cellData[ DataKey(_spacetreeId,outCellStack) ].push(data);
    logDebug("enterCell(...)", "created new cell " << data.toString() );
  }
  else if (inCellStack==peano4::grid::TraversalObserver::NoData) {
    logDebug("enterCell(...)", "no data loaded from local stacks as this is a remote/non-existing cell" );
  }
  else {
    CellData data;
    assertion2(not _cellData[ DataKey(_spacetreeId,inCellStack) ].empty(),_spacetreeId,event.toString());
    data = _cellData[ DataKey(_spacetreeId,inCellStack) ].pop();
    assertionVectorNumericalEquals3(data.x,event.getX(),event.toString(),data.toString(),_spacetreeId);
    assertionVectorNumericalEquals3(data.h,event.getH(),event.toString(),data.toString(),_spacetreeId);
    _cellData[ DataKey(_spacetreeId,outCellStack) ].push(data);
  }

/*
  switch (event.getSendReceiveCellData()) {
    case peano4::grid::GridTraversalEvent::ExchangeHorizontally:
      assertionMsg( false, "may not happen" );
      break;
    case peano4::grid::GridTraversalEvent::StreamInOut:
      {
        const int streamSourceStack = peano4::parallel::Node::getInputStackNumberForForkJoinDataExchange( event.getSendReceiveCellDataRank() );
        assertion3(
          not _cellData[ DataKey(_spacetreeId,streamSourceStack) ].empty(),
          _spacetreeId,streamSourceStack,event.toString()
        );
        assertion(inCellStack==peano4::grid::TraversalObserver::NoData);
        CellData data = _cellData[ DataKey(_spacetreeId,streamSourceStack) ].pop();
        logDebug("enterCell(...)", "streamed in cell " << data.toString() << " into stack " << outCellStack );
        assertionVectorNumericalEquals2(data.x,event.getX(),event.toString(),data.toString());
        assertionVectorNumericalEquals2(data.h,event.getH(),event.toString(),data.toString());
        #if PeanoDebug>=1
        data.spacetreeId = _spacetreeId;
        #endif
        _cellData[ DataKey(_spacetreeId,outCellStack) ].push( data );
      }
      break;
    case peano4::grid::GridTraversalEvent::ExchangeVerticallyWithMaster:
      // @todo Should be optional, isn't it?
//      assertionMsg( false, "may not happen" );
      break;
    case peano4::grid::GridTraversalEvent::ExchangeVerticallyWithWorker:
      // @todo Should be optional, isn't it?
//      assertionMsg( false, "may not happen" );
      break;
    case peano4::grid::GridTraversalEvent::None:
      break;
  }
*/


  if (
    inCellStack!=peano4::grid::TraversalObserver::NoData
  ) {
    // @todo Es gibt noch kein inside/outside hier, oder? Was ist remote?
    // @todo Coarse data stimmt net, oder?
    if (_spacetreeId==0) {
     assertionNumericalEquals2(
       _cellData[ DataKey(_spacetreeId,outCellStack) ].top(0).h(0)*3.0,
       _cellData[ DataKey(_spacetreeId,outCellStack) ].top(1).h(0),
       _cellData[ DataKey(_spacetreeId,outCellStack) ].top(0).toString(),
       _cellData[ DataKey(_spacetreeId,outCellStack) ].top(1).toString()
      );
    }
    _mapping->touchCellFirstTime(
      event.getX(), event.getH(),
      _cellData[ DataKey(_spacetreeId,outCellStack) ].top(0),
      _cellData[ DataKey(_spacetreeId,outCellStack) ].top(1)
    );
  }

/*
  if (event.getSendReceiveCellData()==peano4::grid::GridTraversalEvent::StreamInOut) {
    logDebug("leaveCell(...)", "stream cell to tree " << event.getSendReceiveCellDataRank() );
    const int streamTargetStack = peano4::parallel::Node::getOutputStackNumberForForkJoinDataExchange( event.getSendReceiveCellDataRank() );
    _cellData[ DataKey(_spacetreeId,streamTargetStack ) ].push( _cellData[ DataKey(_spacetreeId,outCellStack) ].top(0) );
  }
*/

  logTraceOutWith1Argument("enterCell(...)",event.toString());
}


void examples::delta::MyObserver::leaveCell(
  const peano4::grid::GridTraversalEvent&  event
) {
  logTraceInWith1Argument("leaveCell(...)",event.toString());

/*
  assertion2(
    event.getCellState()==peano4::grid::GridTraversalEvent::CellState::Local,
    event.toString(), "leaveCell"
  );
*/

  // @todo
/*
  Gibt es einen Overlap? Dann brauchen wir zwei Flags?
  Wer macht das Mapping von Dataflows auf User-Daten?
  Hier?
*/

  // @todo Es gibt noch kein inside/outside hier, oder?
  // @todo Enclaves fehlen halt auch noch
  int inCellStack   = peano4::grid::PeanoCurve::CallStack;
  int outCellStack  = event.getCellData();

  if (outCellStack!=peano4::grid::TraversalObserver::NoData) {
    _mapping->touchCellLastTime(
      event.getX(), event.getH(),
      _cellData[ DataKey(_spacetreeId,inCellStack) ].top(0),
      _cellData[ DataKey(_spacetreeId,inCellStack) ].top(1)
    );
  }

/*
  switch (event.getSendReceiveCellData()) {
    case peano4::grid::GridTraversalEvent::ExchangeHorizontally:
      assertionMsg( false, "may not happen" );
      break;
    case peano4::grid::GridTraversalEvent::StreamInOut:
      assertionMsg( false, "may not happen" );
      break;
    case peano4::grid::GridTraversalEvent::ExchangeVerticallyWithMaster:
      // @todo Should be optional, isn't it?
//      assertionMsg( false, "may not happen" );
      break;
    case peano4::grid::GridTraversalEvent::ExchangeVerticallyWithWorker:
      // @todo Should be optional, isn't it?
//      assertionMsg( false, "may not happen" );
      break;
    case peano4::grid::GridTraversalEvent::None:
      break;
  }*/

  if (outCellStack==TraversalObserver::CreateOrDestroyPersistentGridEntity) {
    logDebug("leaveCell(...)", "destroy cell" );
    CellData data = _cellData[ DataKey(_spacetreeId,inCellStack) ].pop();
    assertionVectorNumericalEquals3(data.x,event.getX(),data.x,data.h,event.toString());
    assertionVectorNumericalEquals3(data.h,event.getH(),data.x,data.h,event.toString());
    _mapping->destroyCell(
      event.getX(),event.getH(),data,
      _cellData[ DataKey(_spacetreeId,outCellStack) ].top(1)
    );
  }
  else if (outCellStack==peano4::grid::TraversalObserver::NoData) {
  }
  else {
    logDebug("leaveCell(...)", "cell " << inCellStack << "->" << outCellStack );
    // @todo raus
    logDebug("leaveCell(...)", _cellData[ DataKey(_spacetreeId,inCellStack) ].toString() );
    CellData data = _cellData[ DataKey(_spacetreeId,inCellStack) ].pop();
    assertionVectorNumericalEquals5(data.x,event.getX(),data.x,data.h,event.toString(),_spacetreeId,inCellStack);
    assertionVectorNumericalEquals5(data.h,event.getH(),data.x,data.h,event.toString(),_spacetreeId,inCellStack);
    _cellData[ DataKey(_spacetreeId,outCellStack) ].push(data);
  }

  logTraceOutWith1Argument("leaveCell(...)",event.toString());
}


peano4::grid::TraversalObserver* examples::delta::MyObserver::clone(int spacetreeId) {
  return new MyObserver( spacetreeId, _iterationCounter, _mapping );
}


std::vector< peano4::grid::GridControlEvent > examples::delta::MyObserver::getGridControlEvents() {
	// @todo Das sollte natuerlich auch in mapping raus und muss dann dort u.U. gemerged werden
  std::vector< peano4::grid::GridControlEvent >  controlEvents;
  if (_iterationCounter<8) {
    peano4::grid::GridControlEvent newEvent;
    newEvent.setRefinementControl( peano4::grid::GridControlEvent::RefinementControl::Refine );

    #if PeanoDebug>0
    const double minH = 0.1;
    #else
    const double minH = 0.02;
    #endif

    #if Dimensions==2
    newEvent.setOffset( {0.0,0.0} );
    newEvent.setWidth( {0.5,0.5} );
    newEvent.setH( {minH,minH} );
    #elif Dimensions==3
    newEvent.setOffset( {0.0,0.0,0.0} );
    newEvent.setWidth( {0.5,0.5,0.5} );
    newEvent.setH( {minH,minH,minH} );
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
