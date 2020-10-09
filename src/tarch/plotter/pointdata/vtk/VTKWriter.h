// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_PLOTTER_POINTDATA_VTK_WRITER_H_
#define _TARCH_PLOTTER_POINTDATA_VTK_WRITER_H_


#include "tarch/la/Vector.h"
#include "tarch/plotter/griddata/unstructured/vtk/VTKTextFileWriter.h"
#include "tarch/mpi/BooleanSemaphore.h"
#include "../PointWriter.h"


namespace tarch {
  namespace plotter {
    namespace pointdata {
      namespace vtk {
        class VTKWriter;
      }
    }
  }
}


/**
 * Implementation of the point data writer that basically maps the points onto
 * a vtk text file, i.e. it uses the unstructured grid writer.
 *
 *
 * <h2> Implementation </h2>
 *
 *
 *
 *
 * @author Tobias Weinzierl
 */
class tarch::plotter::pointdata::vtk::VTKWriter: public tarch::plotter::pointdata::PointWriter {
  private:
    /**
     * This is a global (mpi) semaphore that I use to update/modify the
     * index file over all files. The individual data writes are totally
     * concurrent.
     */
    static tarch::mpi::BooleanSemaphore _sempahore;

    tarch::plotter::griddata::unstructured::UnstructuredGridWriter*                 _vtkWriter;
    tarch::plotter::griddata::unstructured::UnstructuredGridWriter::VertexWriter*   _vertexWriter;
    tarch::plotter::griddata::unstructured::UnstructuredGridWriter::CellWriter*     _cellWriter;

    /**
     * Copy constructor
     *
     * A writer never should be copied. However, we have to make this operation
     * protected to allow implementations to hide their copy constructor as
     * well.
     */
    VTKWriter(const VTKWriter& writer) = delete;

    /**
     * Assignment operator.
     *
     * A writer never should be copied. However, we have to make this operation
     * protected to allow implementations to hide their copy constructor as
     * well.
     */
    VTKWriter& operator=(const VTKWriter& writer) = delete;

  public:
    VTKWriter(bool binaryFile, const std::string& indexFile, tarch::plotter::VTUTimeSeriesWriter::IndexFileMode modeFile);

    virtual ~VTKWriter();

    /**
     * Before we write to a file, we have to close the
     *
     * - vertex writer
     * - call writer
     *
     * However, we might have closed the point data plotters already and thus they
     * might be down already. So we close them only if they are still up.
     *
     * @see PointDataWriter::close()
     */
    bool writeToFile( const std::string& filename ) override;

    /**
     * @return Whether writer is ready to accept data.
     */
    virtual bool isOpen();

    /**
     * Clear the writer, i.e. erase all the data. However, as the writer does
     * not track how many vertex and cell writers you've created, it's up to
     * you to ensure that none of these instances is left.
     */
    virtual void clear();

    virtual int plotPoint(const tarch::la::Vector<2,double>& position);
    virtual int plotPoint(const tarch::la::Vector<3,double>& position);

    /**
     * A writer to assign points a value.
     */
    class PointDataWriter: public PointWriter::PointDataWriter {
      private:
        /**
         * Underlying VTK writer.
         */
        VTKWriter& _myWriter;

        tarch::plotter::griddata::unstructured::UnstructuredGridWriter::VertexDataWriter*  _particleDataWriter;
      public:
        PointDataWriter(VTKWriter& myWriter, const std::string& identifier, int recordsPerPoint);
        virtual ~PointDataWriter();

        /**
         * Write data for one point.
         *
         * @param index Index of the point. This index has to equal the index
         *              used for the point.
         * @param value Value for the point.
         */
        virtual void plot( int index, double value );
        virtual void plot( int index, const tarch::la::Vector<2,double>& value );
        virtual void plot( int index, const tarch::la::Vector<3,double>& value );

        /**
         * If you close your writer, each point has to be assigned a
         * value, i.e. you may not add less data than you have points.
         *
         * <h2> Implementation </h2>
         *
         * The point writer (master class) does hold a writer for cells and one for
         * vertices. This are used via plotPoints(). When we close the data writer,
         * we first have to close both vertex and cell writer. For unstructured grids,
         * we expect users to do this manually.
         *
         * Here, we work with a lazy close mechanism. We check whether the vertices
         * and cells are shutdown alerady. If not, we do it. Then we continue. This
         * lazy implicit closing implies that we need another check in the master
         * class, as there might situations where users dump particles without any
         * data, i.e. this close() is never invoked.
         */
        void close() override;

        /**
         * @see close()
         */
        virtual void assignRemainingPointsDefaultValues();
    };

    /**
     * Caller has to destroy this instance manually.
     */
    virtual PointWriter::PointDataWriter*    createPointDataWriter( const std::string& identifier, int recordsPerPoint );
};

#endif
