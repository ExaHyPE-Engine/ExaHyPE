// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TOOLBOX_PARTICLES_FILE_READER_
#define _TOOLBOX_PARTICLES_FILE_READER_


#include "peano4/utils/Globals.h"
#include "peano4/datamanagement/CellMarker.h"


#include <list>


namespace toolbox {
  namespace particles {
    class FileReader;
  }
}


/**
 * A file reader
 *
 * The file reader realises one big table
 */
class toolbox::particles::FileReader {
  private:
    static tarch::logging::Log                         _log;

    std::list< tarch::la::Vector<Dimensions,double> >  _data;
  public:
    /**
     * Read DAT file
     *
     * A dat file is a simple text file where we have one particle description per line. Each
     * line holds exactly Dimensions double values.
     */
    void readDatFile( const std::string& filename, const double scale );

    /**
     * Take particles from database which fall into given voxel
     *
     * <h2> Overlapping vs. non-overlapping domain decomposition </h2>
     *
     * In an SPMD environment, it is convenient to make each rank maintain its
     * own input database. As a consequence, ranks have redundant particles and
     * we will have to clean up eventually. To mirror this behaviour in the
     * multithreaded context, we can remove particles whenever we assign them
     * to an inner vertex, but we should also hand them out redundantly for
     * boundary vertices.
     *
     * @param x      Center of voxel. If you use this within
     *   touchVertexFirstTime(), you can use the vertex position, i.e. the x()
     *   operation of the marker. If you use this function within a cell event,
     *   you can use x() as well, as it returns the centre.
     * @param h      Size of voxel. If you use this function within a cell
     *   event, just pass in h(). If you use it within a vertex event, you
     *   should scale h with 0.5.
     * @param remove Remove a particle form the internal database if it is a
     *   match. In most cases, removing the particles immediately speeds up
     *   things, even though we then have a lot of (uneccessary) locks.
     *
     * @see peano4::datamanagement::CellMarker::x()
     * @see peano4::datamanagement::VertexMarker::x()
     */
    std::list< tarch::la::Vector<Dimensions,double> > getParticlesWithinVoxel(
      const tarch::la::Vector<Dimensions,double>&  x,
      const tarch::la::Vector<Dimensions,double>&  h,
      bool remove
    );

    void clear();

    bool empty() const;

    bool getNumberOfCoordinates() const;
};

#endif
