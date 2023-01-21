#include "GridControlEvents.h"
#include "peano4/grid/grid.h"


std::vector< peano4::grid::GridControlEvent >   swift2::committedGridControlEvents;


void swift2::commitGridControlEvents( const std::list< peano4::grid::GridControlEvent >& events ) {

/*
#if defined(Parallel)
#error not implemented yet
#endif
::swift2::committedGridControlEvents.clear();
::swift2::committedGridControlEvents = _newGridControlEvents;

merge
*/
  committedGridControlEvents.clear();
  committedGridControlEvents.reserve( events.size() );

  for (const auto& p: events) {
    committedGridControlEvents.push_back(p);
  }

  // @todo Kann man das nicht auf einer Liste auch offiereieren?
  committedGridControlEvents = peano4::grid::merge( committedGridControlEvents );
}
