#include "FileReaderHDF5.h"


#include <stdio.h>
#include <string.h>
#include <numeric>


tarch::logging::Log  toolbox::particles::FileReaderHDF5::_log( "toolbox::particles::FileReaderHDF5" );


void toolbox::particles::FileReaderHDF5::readHDF5File( const std::string& filename ) {

#ifdef UseHDF5

  logInfo( "readHDF5File()", "Entering filereader..." );

  const H5std_string FILE_NAME   ( filename.c_str() );

  /* Group of SPH particles */
  const H5std_string GROUP_NAME  ( "/PartType0" );

  /* 
   * Datasets under the PartType group. 
   * Only PartType0 (SPH particles) is used for now. 
   */
  const H5std_string DATASET_NAME1( "/Coordinates" );
  const H5std_string DATASET_NAME2( "/Velocities" );
  const H5std_string DATASET_NAME3( "/Masses" );
  const H5std_string DATASET_NAME4( "/SmoothingLength" );
  const H5std_string DATASET_NAME5( "/InternalEnergy" );
  const H5std_string DATASET_NAME6( "/ParticleIDs" );

  /*
   * Open the specified file 
   */
  H5::H5File file( FILE_NAME, H5F_ACC_RDONLY );
  logInfo( "readHDF5File()", "File: " << FILE_NAME << " opened." ) ;

  // @TODO read header attributes (contains box size, part number, hydro dimensions, ...)

  /*
   * Now read the various datasets for this particle type.
   */

  // Read positions
  _Coordinates = toolbox::particles::FileReaderHDF5::readDoubleArray( file, GROUP_NAME, DATASET_NAME1 );
  
  // Read velocities
  _Velocity = toolbox::particles::FileReaderHDF5::readFloatArray( file, GROUP_NAME, DATASET_NAME2 );

  // Read masses
  _Mass = toolbox::particles::FileReaderHDF5::readDoubleScalar( file, GROUP_NAME, DATASET_NAME3 );
  
  // Read SmoothingLength (h)
  _SmoothingLength = toolbox::particles::FileReaderHDF5::readDoubleScalar( file, GROUP_NAME, DATASET_NAME4 );

  // Read Internal Energy (U)
  _InternalEnergy = toolbox::particles::FileReaderHDF5::readDoubleScalar( file, GROUP_NAME, DATASET_NAME5 );

  // Keep track of the particles' index in the vectors
  _ParticleIndex = std::list <int> (_SmoothingLength.size());
  std::iota(_ParticleIndex.begin(), _ParticleIndex.end(), 0);

  // Read ParticleIDs
  _ParticleIDs = toolbox::particles::FileReaderHDF5::readIntScalar( file, GROUP_NAME, DATASET_NAME6 );

  logInfo( "readHDF5File()", "Reading file: " << FILE_NAME << " complete." ) ;

#else
  logError( "readHDF5File()", "tried to use Peano's HDF5 reader though code has been compiled without --with-hdf5 (autotools)" );
  exit(-1);
#endif

}

#ifdef UseHDF5
std::vector< int > toolbox::particles::FileReaderHDF5::readIntScalar( 
       const H5::H5File&   file, 
       const H5std_string& groupName, 
       const H5std_string& datasetName 
){

  std::vector< int > result;

  /*
   * Open the specified group from file
   */
  H5::Group group = file.openGroup ( groupName );
  logInfo( "readIntScalar()", "Group: " << groupName << " opened." ) ;

  /* 
   * Open the specified dataset
   */
  H5::DataSet dataset = file.openDataSet( groupName + datasetName );
  logInfo( "readIntScalar()", "Dataset: " << groupName + datasetName << " opened." ) ;

  /*
   * Get dataspace (i.e. shape) of the dataset.
   */
  H5::DataSpace dataspace = dataset.getSpace();

  /*
   * Get the number of dimensions in the dataspace.
   */
  int rank = dataspace.getSimpleExtentNdims();

  /*
  * Get the dimension size of each dimension in the dataspace and
  * display them.
  */
  hsize_t    dims_out[1];
  int ndims = dataspace.getSimpleExtentDims( dims_out, NULL );

  logInfo( "readIntScalar()", 
           "Dataspace dimensions: rank " << rank << 
           ", dimensions " <<
           (unsigned long)(dims_out[0]) )

  /* 
   * Length of the input data that we want to read. By default we read all.
   */
  const int    LengthInputDataX = dims_out[0];

  // Output buffer initialization
  const int    LengthDataBufferX = LengthInputDataX;            
  const int    RankDataBuffer = rank;        // rank=1 for this case (scalar)

  int i; 
  int DataBuffer[LengthDataBufferX];
  for (i = 0; i < LengthDataBufferX; i++)
  {
    DataBuffer[i] = 0; 
  }

  dataspace.selectAll();
  H5::DataSpace memspace( 1 , dims_out );

  // Now read and store into data buffer
  dataset.read( DataBuffer, H5::PredType::NATIVE_INT, memspace, dataspace );
  dataspace.close();

  /* Copy data from buffer into output list or vector container */
  for (i = 0; i < LengthDataBufferX; i++)
  {
    result.push_back( DataBuffer[i] );
    logDebug( "readIntScalar()", "entry value: " << DataBuffer[i] );
  }

  logInfo( "readIntScalar()", "read " << result.size() << " entries. " );

  return result;
}


std::vector< double > toolbox::particles::FileReaderHDF5::readDoubleScalar( 
       const H5::H5File& file, 
       const H5std_string& groupName, 
       const H5std_string& datasetName 
){

  std::vector< double > result;

  /*
   * Open the specified group from file
   */
  H5::Group group = file.openGroup ( groupName );
  logInfo( "readDoubleScalar()", "Group: " << groupName << " opened." ) ;

  /* 
   * Open the specified dataset
   */
  H5::DataSet dataset = file.openDataSet( groupName + datasetName );
  logInfo( "readDoubleScalar()", "Dataset: " << groupName + datasetName << " opened." ) ;

  /*
   * Get dataspace (i.e. shape) of the dataset.
   */
  H5::DataSpace dataspace = dataset.getSpace();

  /*
   * Get the number of dimensions in the dataspace.
   */
  int rank = dataspace.getSimpleExtentNdims();

  /*
  * Get the dimension size of each dimension in the dataspace .
  */
  hsize_t    dims_out[1];
  int ndims = dataspace.getSimpleExtentDims( dims_out, NULL );

  logInfo( "readDoubleScalar()", 
           "Dataspace dimensions: rank " << rank << 
           ", dimensions " <<
           (unsigned long)(dims_out[0]) )

  /* 
   * Hyperslab dimensions. If needed, this can be a part of the data only, e.g.
   * for optimization.
   */
  const int    LengthInputDataX = dims_out[0];
  const int    RankDataBuffer = rank;        // rank=1 for this case.

  // Output buffer dimensions
  const int    LengthDataBufferX = LengthInputDataX;            

  int i; 
  float DataBuffer[LengthDataBufferX];
  for (i = 0; i < LengthDataBufferX; i++)
  {
    DataBuffer[i] = 0; 
  }

  dataspace.selectAll();

  /* Define memory space to do the copy */
  H5::DataSpace memspace( RankDataBuffer , dims_out );
  
  /* Now read and store into data buffer */
  dataset.read( DataBuffer, H5::PredType::NATIVE_FLOAT, memspace, dataspace );
  dataspace.close();
  
  /* Copy data from buffer into output list or vector container */
  for (i = 0; i < LengthDataBufferX; i++)
  {
    result.push_back( DataBuffer[i] );
    logDebug( "readDoubleScalar()", "entry value: " << DataBuffer[i] );
  }

  logInfo( "readDoubleScalar()", "read " << result.size() << " entries. " );

  return result;

}


std::list< tarch::la::Vector<Dimensions,double> > toolbox::particles::FileReaderHDF5::readDoubleArray( 
    const H5::H5File& file, 
    const H5std_string& groupName, 
    const H5std_string& datasetName 
){

  std::list< tarch::la::Vector<Dimensions,double> > result;

  /*
   * Open the specified group
   */
  H5::Group group = file.openGroup ( groupName );
  logInfo( "readHDF5File()", "Group: " << groupName << " opened." ) ;

  /* 
   * Open the specified dataset
   */
  H5::DataSet dataset = file.openDataSet( groupName+datasetName );
  logInfo( "readHDF5File()", "Dataset: " << groupName + datasetName << " opened." ) ;

  /*
   * Get dataspace (i.e. shape) of the dataset.
   */
  H5::DataSpace dataspace = dataset.getSpace();

  /*
   * Get the number of dimensions in the dataspace.
   */
  int rank = dataspace.getSimpleExtentNdims();

  /*
  * Get the dimension size of each dimension in the dataspace.
  */
  hsize_t    dims_out[2];
  int ndims = dataspace.getSimpleExtentDims( dims_out, NULL );

  logInfo( "readHDF5File()", 
           "Dataspace dimensions: rank " << rank << 
           ", dimensions " <<
           (unsigned long)(dims_out[0]) << " x " <<
           (unsigned long)(dims_out[1]) ) ;

  /* 
   * Hyperslab dimensions. If needed, this can be a part of the data only, e.g.
   * for optimization.
   */
  const int    LengthInputDataX = dims_out[0];
  const int    LengthInputDataY = dims_out[1];
  const int    RankDataBuffer = rank;        // the data will be stored in a table-like format 

  // Output buffer dimensions
  const int    LengthDataBufferX = LengthInputDataX;            
  const int    LengthDataBufferY = LengthInputDataY;

  /*
  * Output buffer initialization.
  */
  int i, j;
  double dataBuffer[LengthDataBufferX][LengthDataBufferY]; 
  for (i = 0; i < LengthDataBufferX; i++)
  {
    for (j = 0; j < LengthDataBufferY; j++)
    {
      dataBuffer[i][j] = 0; // @TODO Initialize array to sensible, default values
    }
  }

  /*
   * Define hyperslab in the dataset; implicitly giving strike and
   * block NULL.
   */
  hsize_t    offset[2];   // hyperslab offset in the file
  hsize_t    count[2];    // size of the hyperslab in the file
  offset[0] = 0;
  offset[1] = 0;
  count[0] = LengthInputDataX;
  count[1] = LengthInputDataY;

  dataspace.selectHyperslab( H5S_SELECT_SET, count, offset );

  /*
  * Define the memory dataspace.
  */
  hsize_t    dimsm[2];              /* memory space dimensions */
  dimsm[0] = LengthDataBufferX;
  dimsm[1] = LengthDataBufferY;

  H5::DataSpace memspace( RankDataBuffer, dimsm );

  /*
  * Define memory hyperslab.
  */
  hsize_t      offset_out[2];       // hyperslab offset in memory
  hsize_t      count_out[2];        // size of the hyperslab in memory
  offset_out[0] = 0;
  offset_out[1] = 0;
  count_out[0]  = LengthInputDataX;
  count_out[1]  = LengthInputDataY;

  memspace.selectHyperslab( H5S_SELECT_SET, count_out, offset_out );

  // Now read and store into data buffer
  H5::DataType Datatype;
  Datatype = H5::PredType::NATIVE_DOUBLE;

  dataset.read( dataBuffer, Datatype, memspace, dataspace );
  dataspace.close();

  /* Copy the data buffer into the output list or vector container. 
   * These support adding and removing elements, which is useful to identify and
   * keep particles within a cell while discarding the rest. That will be done
   * in a later step.
   */

  /* First construct a single row of the list */
  tarch::la::Vector<Dimensions,double> tmpDataArray;

  for (i = 0; i < LengthDataBufferX; i++)
  {
    for (j = 0; j < Dimensions; j++)
    {
      tmpDataArray[j] = dataBuffer[i][j];
    }
    /* Now append this row to the final list */
    result.push_back( tmpDataArray );

    logDebug( "readDoubleArray()", "tmpDataArray " << tmpDataArray << ", i = " << i );
  }

  logInfo( "readHDF5File()", "read " << result.size() << " entries. " );
  return result;

}


std::vector< tarch::la::Vector<Dimensions,double> > toolbox::particles::FileReaderHDF5::readFloatArray( 
    const H5::H5File& file, 
    const H5std_string& groupName, 
    const H5std_string& datasetName 
){

  std::vector< tarch::la::Vector<Dimensions,double> > result;

  /*
   * Open the specified group
   */
  H5::Group group = file.openGroup ( groupName );
  logInfo( "readHDF5File()", "Group: " << groupName << " opened." ) ;

  /* 
   * Open the specified dataset
   */
  H5::DataSet dataset = file.openDataSet( groupName+datasetName );
  logInfo( "readHDF5File()", "Dataset: " << groupName + datasetName << " opened." ) ;

  /*
   * Get dataspace (i.e. shape) of the dataset.
   */
  H5::DataSpace dataspace = dataset.getSpace();

  /*
   * Get the number of dimensions in the dataspace.
   */
  int rank = dataspace.getSimpleExtentNdims();

  /*
  * Get the dimension size of each dimension in the dataspace.
  */
  hsize_t    dims_out[2];
  int ndims = dataspace.getSimpleExtentDims( dims_out, NULL );

  logInfo( "readHDF5File()", 
           "Dataspace dimensions: rank " << rank << 
           ", dimensions " <<
           (unsigned long)(dims_out[0]) << " x " <<
           (unsigned long)(dims_out[1]) ) ;

  /* 
   * Hyperslab dimensions. If needed, this can be a part of the data only, e.g.
   * for optimization.
   */
  const int    LengthInputDataX = dims_out[0];
  const int    LengthInputDataY = dims_out[1];
  const int    RankDataBuffer = rank;        // the data will be stored in a table-like format 

  // Output buffer dimensions
  const int    LengthDataBufferX = LengthInputDataX;            
  const int    LengthDataBufferY = LengthInputDataY;

  /*
  * Output buffer initialization.
  */
  int i, j;
  double dataBuffer[LengthDataBufferX][LengthDataBufferY]; 
  for (i = 0; i < LengthDataBufferX; i++)
  {
    for (j = 0; j < LengthDataBufferY; j++)
    {
      dataBuffer[i][j] = 0; // @TODO Initialize array to sensible, default values
    }
  }

  /*
   * Define hyperslab in the dataset; implicitly giving strike and
   * block NULL.
   */
  hsize_t    offset[2];   // hyperslab offset in the file
  hsize_t    count[2];    // size of the hyperslab in the file
  offset[0] = 0;
  offset[1] = 0;
  count[0] = LengthInputDataX;
  count[1] = LengthInputDataY;

  dataspace.selectHyperslab( H5S_SELECT_SET, count, offset );

  /*
  * Define the memory dataspace.
  */
  hsize_t    dimsm[2];              /* memory space dimensions */
  dimsm[0] = LengthDataBufferX;
  dimsm[1] = LengthDataBufferY;

  H5::DataSpace memspace( RankDataBuffer, dimsm );

  /*
  * Define memory hyperslab.
  */
  hsize_t      offset_out[2];       // hyperslab offset in memory
  hsize_t      count_out[2];        // size of the hyperslab in memory
  offset_out[0] = 0;
  offset_out[1] = 0;
  count_out[0]  = LengthInputDataX;
  count_out[1]  = LengthInputDataY;

  memspace.selectHyperslab( H5S_SELECT_SET, count_out, offset_out );

  // Now read and store into data buffer
  H5::DataType Datatype = H5::PredType::NATIVE_FLOAT;

  dataset.read( dataBuffer, Datatype, memspace, dataspace );
  dataspace.close();

  /* Copy the data buffer into the output list or vector container. 
   * These support adding and removing elements, which is useful to identify and
   * keep particles within a cell while discarding the rest. That will be done
   * in a later step.
   */

  /* First construct a single row of the list */
  tarch::la::Vector<Dimensions,double> tmpDataArray;

  for (i = 0; i < LengthDataBufferX; i++)
  {
    for (j = 0; j < Dimensions; j++)
    {
      tmpDataArray[j] = dataBuffer[i][j];
    }
    /* Now append this row to the final list */
    result.push_back( tmpDataArray );

    logDebug( "readFloatArray()", "tmpDataArray " << tmpDataArray << ", i = " << i );
  }

  logInfo( "readHDF5File()", "read " << result.size() << " entries. " );
  return result;

}

#endif

void toolbox::particles::FileReaderHDF5::clearVoxel() {
  _VelocityWithinVoxel.clear();
  _MassWithinVoxel.clear();
  _SmoothingLengthWithinVoxel.clear();
  _InternalEnergyWithinVoxel.clear();
  _ParticleIDsWithinVoxel.clear();
}

bool toolbox::particles::FileReaderHDF5::isVoxelEmpty() const {
  return (_VelocityWithinVoxel.empty() &&
          _MassWithinVoxel.empty() &&
          _SmoothingLengthWithinVoxel.empty() &&
          _InternalEnergyWithinVoxel.empty() &&
          _ParticleIDsWithinVoxel.empty());
}

std::list< tarch::la::Vector<Dimensions,double> > toolbox::particles::FileReaderHDF5::getParticlesWithinVoxel(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  bool remove
) {
  std::list< tarch::la::Vector<Dimensions,double> > result;

  std::list< tarch::la::Vector<Dimensions,double> >::iterator p = _Coordinates.begin();
  std::list<int>::iterator pind = _ParticleIndex.begin();

  // Make sure we're starting with a clean slate.
  if (!isVoxelEmpty()) clearVoxel();

  /* Loop over all coordinates that have been read in. When you find a 
   * particle that belongs in this voxel, remove it from the list of particles
   * to loop over so they don't get checked again in other voxels. */
  while ( p!=_Coordinates.end() ) {

    bool overlaps = true;

    for (int d=0; d<Dimensions; d++) {
      overlaps &= tarch::la::smallerEquals( x(d) - h(d) / 2.0, (*p)(d) );
      overlaps &= tarch::la::greaterEquals( x(d) + h(d) / 2.0, (*p)(d) );
    }

    if (overlaps) {
      /* Store coordinates */
      result.push_back(*p);

      /* Also store other fields. */
      _VelocityWithinVoxel.push_back       ( _Velocity       [*pind] );
      _MassWithinVoxel.push_back           ( _Mass           [*pind] );
      _SmoothingLengthWithinVoxel.push_back( _SmoothingLength[*pind] );
      _InternalEnergyWithinVoxel.push_back ( _InternalEnergy [*pind] );
      _ParticleIDsWithinVoxel.push_back    ( _ParticleIDs    [*pind] );

      if (remove) {
        p = _Coordinates.erase(p);
        pind = _ParticleIndex.erase(pind);
      }
      else {
        p++;
        pind++;
      }
    } // overlaps == true
    else {
      p++;
      pind++;
    }
  } // loop over particle coordinates

  return result;
}


void toolbox::particles::FileReaderHDF5::clear() {
  _Coordinates.clear();
  _Mass.clear();
  _SmoothingLength.clear();
  _InternalEnergy.clear();
  _ParticleIDs.clear();
  _ParticleIndex.clear();
}


bool toolbox::particles::FileReaderHDF5::empty() const {
  return _Coordinates.empty();
}


bool toolbox::particles::FileReaderHDF5::getNumberOfCoordinates() const {
  return _Coordinates.size();
}


std::vector < tarch::la::Vector<Dimensions,double> > toolbox::particles::FileReaderHDF5::getVelocityWithinVoxel() const {
  return _VelocityWithinVoxel;
}


std::vector <double> toolbox::particles::FileReaderHDF5::getMassWithinVoxel() const {
  return _MassWithinVoxel;
}


std::vector <double> toolbox::particles::FileReaderHDF5::getSmoothingLengthWithinVoxel() const {
  return _SmoothingLengthWithinVoxel;
}


std::vector <double> toolbox::particles::FileReaderHDF5::getInternalEnergyWithinVoxel() const {
  return _InternalEnergyWithinVoxel;
}


std::vector <int> toolbox::particles::FileReaderHDF5::getParticleIDsWithinVoxel() const {
  return _ParticleIDsWithinVoxel;
}


