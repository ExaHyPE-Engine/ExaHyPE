// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_PLOT_FILTER_H_
#define _EXAHYPE2_PLOT_FILTER_H_


#include "peano4/datamanagement/CellMarker.h"


#include <vector>



namespace exahype2 {
  class PlotFilter;
}


class exahype2::PlotFilter {
  private:
    /**
     * One filter entry
     *
     * - offset
     * - size
     * - plot interval
     * - last plot
     */
    struct FilterEntry {
      tarch::la::Vector<Dimensions,double> offset;
      tarch::la::Vector<Dimensions,double> size;
      int                                  frequency;
      int                                  nextPlot;
    };
/*
    typedef std::tuple<
      tarch::la::Vector<Dimensions,double>,
      tarch::la::Vector<Dimensions,double>,
      int,
      int
    > FilterEntry;
    */
    
    std::vector< FilterEntry > _filterEntries;
  public:
    typedef std::tuple<
      tarch::la::Vector<Dimensions,double>,
      tarch::la::Vector<Dimensions,double>,
      int
    > FilterEntryTuple;

    void addFilter( FilterEntryTuple newEntry );
    
    bool plotPatch(const peano4::datamanagement::CellMarker& marker);
    
    void finishPlottingStep();
};
  
  
  
#endif
 