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

    std::list< tarch::la::Vector<Dimensions,double> > getParticlesWithinVoxel(
      const tarch::la::Vector<Dimensions,double>&  x,
      const tarch::la::Vector<Dimensions,double>&  h
    );

    void clear();

    bool empty() const;

    bool getNumberOfCoordinates() const;
};

#endif
