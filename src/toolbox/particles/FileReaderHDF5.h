// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "config.h"
#include "peano4/utils/Globals.h"
#include "peano4/datamanagement/CellMarker.h"

#include "tarch/logging/Log.h"

#include <list> 
#include <vector> 

#ifdef UseHDF5
#include "H5Cpp.h"
#endif

namespace toolbox {
  namespace particles {
    class FileReaderHDF5;
  }
}


/**
 * An HDF5 file reader
 * This class works if and only if you have compiled Peano using --with-hdf5.
 * 
 * Particle coordinates are stored in list containers. Importantly, this will be used by the
 * getParticlesWithinVoxel method to retrieve particles from each cell (and then
 * remove them if desired).
 * Other HDF5 groups are stored in vector containers so we can use subscripts to access them.
 * @TODO use templates to unify the readers for the different return types.
 */
class toolbox::particles::FileReaderHDF5 {
  private:
    static tarch::logging::Log                           _log;

    /* Coordinates */
    std::list< tarch::la::Vector<Dimensions,double> >    _Coordinates;

    /* Other fields in the HDF5 file */
    std::vector< tarch::la::Vector<Dimensions,double> >  _Velocity;
    std::vector<double>                                  _Mass;
    std::vector<double>                                  _SmoothingLength;
    std::vector<double>                                  _InternalEnergy;
    std::vector<int>                                     _ParticleIDs;

    /* If we are removing read-in particles on-the-fly while sorting
     * them into voxels, we need to keep track of their index to access
     * the other particle quantities correctly. This list keeps these indexes.*/
    std::list<int>                                       _ParticleIndex;

    /* Copies that hold the data only for particles within the voxel */
    /* If you are adding additional quantities, don't forget to update them
     * in FileReaderHDF5.clearVoxel() and FileReaderHDF5.isVoxelEmpty() 
     * as well */
    std::vector< tarch::la::Vector<Dimensions,double> >  _VelocityWithinVoxel;
    std::vector<double>                                  _MassWithinVoxel;
    std::vector<double>                                  _SmoothingLengthWithinVoxel;
    std::vector<double>                                  _InternalEnergyWithinVoxel;
    std::vector<int>                                     _ParticleIDsWithinVoxel;

  public:
    /**
     * Read HDF5 file in SWIFT format.
     * The coordinates are stored into lists, which allow efficient manipulation
     * for inserting/removing entries later on, see getParticlesWithinVoxel
     * below.
     * @param filename string with file name of the HDF5 file.
     */
    void readHDF5File( const std::string& filename );

#ifdef UseHDF5

    /**
     * Read a set of integers from an HDF5 file
     * We read the full dataset.
     * @TODO in future large-scale sims we might want to use hyperslabs to
     * optimize performance.
     *
     * @param file              H5File which has been opened.
     * @param groupName         name of the group where the relevant dataset lives.
     * @param datasetName       name of dataset that will be read.
     *
     */
    std::vector< int > readIntScalar( 
       const H5::H5File&   file, 
       const H5std_string& groupName, 
       const H5std_string& datasetName );

    /**
     * Read a set of doubles from an HDF5 file
     * We read the full dataset.
     * @TODO in future large-scale sims we might want to use hyperslabs to
     * optimize performance.
     *
     * @param file              H5File which has been opened.
     * @param groupName         name of the group where the relevant dataset lives.
     * @param datasetName       name of dataset that will be read.
     *
     */
    std::vector< double > readDoubleScalar( 
       const H5::H5File&   file, 
       const H5std_string& groupName, 
       const H5std_string& datasetName );

    /**
     * Read a Dimensions array of doubles from an HDF5 file
     * This is used for Coordinates, which are stored as a list.
     * We read the full dataset.
     * @TODO in future large-scale sims we might want to use hyperslabs to
     * optimize performance.
     *
     * @param file              H5File which has been opened.
     * @param groupName         name of the group where the relevant dataset lives.
     * @param datasetName       name of dataset that will be read.
     */
    std::list< tarch::la::Vector<Dimensions,double> > readDoubleArray( 
       const H5::H5File&   file, 
       const H5std_string& groupName, 
       const H5std_string& datasetName );

    /**
     * Read a Dimensions array of floats from an HDF5 file
     * This is used to read velocities, which are stored in a vector container.
     * We read the full dataset.
     * @TODO in future large-scale sims we might want to use hyperslabs to
     * optimize performance.
     *
     * @param file              H5File which has been opened.
     * @param groupName         name of the group where the relevant dataset lives.
     * @param datasetName       name of dataset that will be read.
     */
    std::vector< tarch::la::Vector<Dimensions,double> > readFloatArray( 
       const H5::H5File&   file, 
       const H5std_string& groupName, 
       const H5std_string& datasetName );

#endif   

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

    /**
     * Clear data of a voxel carried by this instance of FileReaderHDF5.
     **/
    void clearVoxel();

    /**
     * Check whether voxel data carried by this instance of FileReaderHDF5
     * is empty.
     **/
    bool isVoxelEmpty() const;

    /* 
     * Get methods for data other than Coordinates.
     */
    std::vector< tarch::la::Vector<Dimensions,double> > getVelocityWithinVoxel() const;
    std::vector<double>                                 getMassWithinVoxel() const;
    std::vector<double>                                 getSmoothingLengthWithinVoxel() const;
    std::vector<double>                                 getInternalEnergyWithinVoxel() const;
    std::vector<int>                                    getParticleIDsWithinVoxel() const;

};

