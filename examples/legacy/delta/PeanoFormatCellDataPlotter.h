// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAMPLES_DELTA_PEANO_FORMAT_CELL_DATA_PLOTTER_H_
#define _EXAMPLES_DELTA_PEANO_FORMAT_CELL_DATA_PLOTTER_H_


#include "Mapping.h"


#include "tarch/plotter/griddata/blockstructured/PeanoTextPatchFileWriter.h"
#include "tarch/multicore/BooleanSemaphore.h"


namespace examples {
  namespace delta {
    class PeanoFormatCellDataPlotter;
  }
}


/**
 * Observer which pipes the automaton transitions into a Peano patch file
 */
class examples::delta::PeanoFormatCellDataPlotter: public examples::delta::Mapping {
  private:
    const bool         _plotThroughoutDescent;
    const std::string  _fileNamePrefix;

    static tarch::multicore::BooleanSemaphore   _semaphore;
    static int                                  _instanceCounter;

    int                                                                                   _counter;

    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter*                  _writer;
    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::CellDataWriter*  _dataWriter;
    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::CellDataWriter*  _spacetreeWriter;

    void plotCell(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      CellData&                                    data
    );
  public:
    /**
     * You have to invoke startNewSnapshot() if you wanna have a pvd file
     * immediately after you've created this observer in the main code.
     *
     * If this guy is ran on the global master,
     */
    PeanoFormatCellDataPlotter(const std::string&  fileNamePrefix, bool plotThroughoutDescent);
    virtual ~PeanoFormatCellDataPlotter();

	void beginTraversal() final;
	void endTraversal() final;

  	void createCell(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      CellData&                                    data,
      CellData&                                    coarseData
    ) final;

    void destroyCell(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      CellData&                                    data,
      CellData&                                    coarseData
    ) final;

    void touchCellFirstTime(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      CellData&                                    data,
      CellData&                                    coarseData
    ) final;

    void touchCellLastTime(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      CellData&                                    data,
      CellData&                                    coarseData
    ) final;
};

#endif
