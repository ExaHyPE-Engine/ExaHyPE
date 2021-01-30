#include "PlotFilter.h"


void exahype2::PlotFilter::addFilter( FilterEntryTuple newEntry ) {
  FilterEntry entry;
  entry.offset    = std::get<0>(newEntry);
  entry.size      = std::get<1>(newEntry);
  entry.frequency = std::get<2>(newEntry);
  entry.nextPlot  = 0;
  _filterEntries.push_back( entry );
}


bool exahype2::PlotFilter::plotPatch(const peano4::datamanagement::CellMarker& marker) {
  bool result = _filterEntries.empty();

  for (const auto& p: _filterEntries) {
    if (p.nextPlot==0) {
      result |= marker.overlaps( p.offset, p.size );
    }
  }

  return result;
}


void exahype2::PlotFilter::finishPlottingStep() {
  for (auto& p: _filterEntries) {
    p.nextPlot--;
    if (p.nextPlot<0) {
      p.nextPlot = p.frequency-1;
    }
  }
}


