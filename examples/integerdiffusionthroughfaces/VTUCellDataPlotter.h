// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAMPLES_INTEGER_DIFFUSION_THROUGH_FACES_VTU_CELL_DATA_PLOTTER_H_
#define _EXAMPLES_INTEGER_DIFFUSION_THROUGH_FACES_VTU_CELL_DATA_PLOTTER_H_


#include "Mapping.h"


#include "tarch/plotter/griddata/unstructured/vtk/VTUTextFileWriter.h"
#include "tarch/plotter/griddata/VTUTimeSeriesWriter.h"


namespace examples {
  namespace integerdiffusionthroughfaces {
    class VTUCellDataPlotter;
  }
}


/**
 * Observer which pipes the automaton transitions into a VTK file
 *
 * While we use the up-to-date vtk format, the observer plots the whole thing
 * as a discontinuous unstructured mesh. It is not particular sophisticated.
 *
 * The plotter can write whole time series. For this, you have to invoke
 * startNewSnapshot() prior to each plot.
 */
class examples::integerdiffusionthroughfaces::VTUCellDataPlotter: public examples::integerdiffusionthroughfaces::Mapping {
  private:
	const bool         _plotThroughoutDescent;
	const std::string  _fileNamePrefix;

    int                                                                              _counter;

    tarch::plotter::griddata::unstructured::vtk::VTUTextFileWriter*                  _writer;
    tarch::plotter::griddata::unstructured::UnstructuredGridWriter::VertexWriter*    _vertexWriter;
    tarch::plotter::griddata::unstructured::UnstructuredGridWriter::CellWriter*      _cellWriter;
    tarch::plotter::griddata::unstructured::UnstructuredGridWriter::CellDataWriter*  _dataWriter;


    tarch::plotter::griddata::VTUTimeSeriesWriter                             _timeSeriesWriter;

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
     * @param plotThroughoutDescent If this flag is set, we plot throughout the
     *   tree descent, i.e. in touch cell first time. Otherwise, we plot
     *   throughout the ascent.
     */
    VTUCellDataPlotter(const std::string&  fileNamePrefix, bool plotThroughoutDescent);
    virtual ~VTUCellDataPlotter();

	void beginTraversal() final;
	void endTraversal() final;

	void createPersistentFace(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    ) final;

	void createHangingFace(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    ) final;

	void destroyPersistentFace(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    ) final;

	void destroyHangingFace(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    ) final;

	void touchFaceFirstTime(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    ) final;

	void touchFaceLastTime(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    ) final;

	void createCell(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
	  CellData&                                    data,
	  Faces&                                       faces,
	  CellData&                                    coarseData,
	  Faces&                                       coarseFaces
    ) final;

	void destroyCell(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
	  CellData&                                    data,
	  Faces&                                       faces,
	  CellData&                                    coarseData,
	  Faces&                                       coarseFaces
    ) final;

	void touchCellFirstTime(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
	  CellData&                                    data,
	  Faces&                                       faces,
	  CellData&                                    coarseData,
	  Faces&                                       coarseFaces
    ) final;

	void touchCellLastTime(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
	  CellData&                                    data,
	  Faces&                                       faces,
	  CellData&                                    coarseData,
	  Faces&                                       coarseFaces
    ) final;
};

#endif
