// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_PLOTTER_GRIDDATA_UNSTRUCTURED_UNSTRUCTURED_GRID_WRITER_H_
#define _TARCH_PLOTTER_GRIDDATA_UNSTRUCTURED_UNSTRUCTURED_GRID_WRITER_H_


#include "tarch/la/Vector.h"


namespace tarch {
  namespace plotter {
    namespace griddata {
      namespace unstructured {
        class UnstructuredGridWriter;
      }
    }
  }
}


class tarch::plotter::griddata::unstructured::UnstructuredGridWriter {
  public:

    /**
     * @return Write has been successful
     */
    virtual bool writeToFile() = 0;

    /**
     * @return Whether writer is ready to accept data.
     */
    virtual bool isOpen() = 0;

    /**
     * Clear the writer, i.e. erase all the data. However, as the writer does
     * not track how many vertex and cell writers you've created, it's up to
     * you to ensure that none of these instances is left.
     */
    virtual void clear() = 0;

    /**
     * A writer for data on elements.
     */
    class CellDataWriter {
   	  public:
        virtual ~CellDataWriter() {};

        /**
         * Write data for one cell.
         *
         * @param index Index of the cell. This index has to equal the index
         *              used for the cell within the VTKWriter class
         *              interface.
         * @param value Value for the cell.
         */
        virtual void plotCell( int index, double value ) = 0;
        virtual void plotCell( int index, const tarch::la::Vector<2,double>& value ) = 0;
        virtual void plotCell( int index, const tarch::la::Vector<3,double>& value ) = 0;
        virtual void plotCell( int index, double* values, int numberOfValues ) = 0;

        /**
         * If you close your writer, each cell/vertex has to be assigned a
         * value, i.e. you may not add less data than you have cells. See
         *
         */
        virtual void close() = 0;

        /**
         * @see close()
         */
        virtual void assignRemainingCellsDefaultValues() = 0;
    };

    /**
     * A writer for scalar data on points (vertices).
     */
    class VertexDataWriter {
      public:
        virtual ~VertexDataWriter() {};

        /**
         * Write data for one cell.
         *
         * @param index Index of the vertex. This index has to equal the index
         *              used for the cell within the VTKWriter class
         *              interface.
         * @param value Value for the cell.
         */
        virtual void plotVertex( int index, double value ) = 0;
        virtual void plotVertex( int index, const tarch::la::Vector<2,double>& value ) = 0;
        virtual void plotVertex( int index, const tarch::la::Vector<3,double>& value ) = 0;
        virtual void plotVertex( int index, double* values, int numberOfValues ) = 0;

        virtual void close() = 0;

        /**
         * @see close()
         */
        virtual void assignRemainingVerticesDefaultValues() = 0;
    };

    /**
     * Caller has to destroy this instance manually.
     */
    virtual CellDataWriter*    createCellDataWriter( const std::string& identifier, int recordsPerCell )   = 0;

    /**
     * Caller has to destroy this instance manually.
     */
    virtual VertexDataWriter*  createVertexDataWriter( const std::string& identifier, int recordsPerVertex ) = 0;

    /**
     * This is the vertex writer you have to create to plot the vertices.
     * Besides the pure syntax management, the writer also provides a number
     * generator which provides you with a unique id for each vertex.
     *
     * Please ensure that you call close() on the vertex writer before you
     * close the underlying VTKTextFileWriter.
     */
    class VertexWriter {
      public:
        virtual ~VertexWriter() {}

        virtual int plotVertex(const tarch::la::Vector<2,double>& position) = 0;
        virtual int plotVertex(const tarch::la::Vector<3,double>& position) = 0;

        virtual void close() = 0;
    };

    /**
     * Writes the cell.
     */
    class CellWriter {
      public:
        virtual ~CellWriter() {}

        virtual int plotHexahedron(int vertexIndex[8]) = 0;
        virtual int plotQuadrangle(int vertexIndex[4]) = 0;
        virtual int plotLine(int vertexIndex[2]) = 0;
        virtual int plotPoint(int vertexIndex) = 0;
        virtual int plotTriangle(int vertexIndex[3]) = 0;

        virtual void close() = 0;
    };

    /**
     * Caller has to destroy this instance manually. Do not create more than
     * one vertex writer.
     */
    virtual VertexWriter*   createVertexWriter() = 0;

    /**
     * Caller has to destroy this instance manually. Do not create more than one
     * cell writer.
     */
    virtual CellWriter*     createCellWriter() = 0;
};


#endif
