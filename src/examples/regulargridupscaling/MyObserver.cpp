#include "MyObserver.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/parallel/Node.h"
#include "peano4/parallel/Tasks.h"
#include "peano4/grid/GridTraversalEvent.h"

#include "tarch/la/MatrixOperations.h"
#include "tarch/multicore/Core.h"

#include "toolbox/finiteelements/ElementMatrix.h"
#include "toolbox/finiteelements/StencilFactory.h"


tarch::logging::Log examples::regulargridupscaling::MyObserver::_log( "examples::regulargridupscaling::MyObserver" );
double              examples::regulargridupscaling::MyObserver::FractionOfCellsYieldingIntegrationTask(0.0);
int                 examples::regulargridupscaling::MyObserver::IntegrationAccuracy(0);


examples::regulargridupscaling::MyObserver::MyObserver(int spacetreeId, double h, int flopsPerCell):
  _spacetreeId(spacetreeId),
  _h(h),
  _flopsPerCell(flopsPerCell),
  _accumulator(0.0) {
}


examples::regulargridupscaling::MyObserver::~MyObserver() {
}


void examples::regulargridupscaling::MyObserver::beginTraversal(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h
) {
  _accumulator     = 0.0;
  _taskAccumulator = 0.0;
}


void examples::regulargridupscaling::MyObserver::endTraversal(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h
) {
  tarch::multicore::processPendingTasks();
  logDebug( "endTraversal", _accumulator );
}


void examples::regulargridupscaling::MyObserver::enterCell(
  const peano4::grid::GridTraversalEvent&  event
) {
  if (
    event.getIsRefined()==false
    and
    event.getCellData() != TraversalObserver::NoData
  ) {
    for (int i=0; i<_flopsPerCell; i++) {
      _accumulator += 1.0;
    }

    if (_spacetreeId==0) {
      _taskAccumulator += std::abs(FractionOfCellsYieldingIntegrationTask) * tarch::multicore::Core::getInstance().getNumberOfThreads();
    }

    if (_taskAccumulator>=1.0) {
      _taskAccumulator -= 1.0;
      assertion(IntegrationAccuracy>=1);
      peano4::parallel::Tasks(
    	[&]() -> bool {
          tarch::la::Matrix<TwoPowerD,TwoPowerD,double>  localStiffnessMatrix =
            toolbox::finiteelements::getPoissonMatrixWithJumpingCoefficient(
              event.getX(), event.getH(), IntegrationAccuracy,
      	      [](const tarch::la::Vector<Dimensions,double>& x) -> double {
        	    const double Theta = 32.0;
                return 1.0
         	           + 0.3/Dimensions * std::exp(-Theta * x(0)) * std::cos( tarch::la::PI * x(0) * Theta )
                       + 0.3/Dimensions * std::exp(-Theta * x(1)) * std::cos( tarch::la::PI * x(1) * Theta );
              }
    	    );
          // just to ensure we cannot remove it.
          _accumulator += tarch::la::frobeniusNorm(localStiffnessMatrix);
    	  return false;
        },
        FractionOfCellsYieldingIntegrationTask<0 ? peano4::parallel::Tasks::TaskType::Sequential : peano4::parallel::Tasks::TaskType::Task,
        peano4::parallel::Tasks::getLocationIdentifier( "examples::regulargridupscaling::MyObserver::enterCell" )
      );
    }
  }
}


void examples::regulargridupscaling::MyObserver::leaveCell(
  const peano4::grid::GridTraversalEvent&  event
) {
}


peano4::grid::TraversalObserver* examples::regulargridupscaling::MyObserver::clone(int spacetreeId) {
  return new MyObserver( spacetreeId, _h, _flopsPerCell );
}


std::vector< peano4::grid::GridControlEvent > examples::regulargridupscaling::MyObserver::getGridControlEvents() {
  std::vector< peano4::grid::GridControlEvent >  controlEvents;

  peano4::grid::GridControlEvent newEvent;
  newEvent.setRefinementControl( peano4::grid::GridControlEvent::RefinementControl::Refine );
  #if Dimensions==2
    newEvent.setOffset( {0.0,0.0} );
    newEvent.setWidth( {1.0,1.0} );
    newEvent.setH( {_h,_h} );
  #elif Dimensions==3
    newEvent.setOffset( {0.0,0.0,0.0} );
    newEvent.setWidth( {1.0,1.0,1.0} );
    newEvent.setH( {_h,_h,_h} );
  #endif
  controlEvents.push_back(newEvent);

  return controlEvents;
}
