// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TopologyParser.h"

#include <cstring>

#include "Constants.h"

applications::exahype2::landslide::TopologyParser::TopologyParser(
  const char* elevationFilePath,
  const char* initVFilePath,
  const char* ncElevationKey,
  const char* ncElevationKeyX,
  const char* ncElevationKeyY,
  const char* ncInitVKey,
  const char* ncInitVKeyX,
  const char* ncInitVKeyY
):
  _ncElevationKey(ncElevationKey),
  _ncElevationKeyX(ncElevationKeyX),
  _ncElevationKeyY(ncElevationKeyY),
  _ncInitVKey(ncInitVKey),
  _ncInitVKeyX(ncInitVKeyX),
  _ncInitVKeyY(ncInitVKeyY) {

  _lengthOfSimulationDomainInX = DomainSize(0);
  _lengthOfSimulationDomainInY = DomainSize(1);

  this->parseElevationFile(elevationFilePath);
  this->parseInitVFile(initVFilePath);
}

applications::exahype2::landslide::TopologyParser::TopologyParser(
  const char* elevationFilePath,
  double lengthOfSimulationDomainInX,
  double lengthOfSimulationDomainInY,
  const char* ncElevationKey,
  const char* ncElevationKeyX,
  const char* ncElevationKeyY
):
  _ncElevationKey(ncElevationKey),
  _ncElevationKeyX(ncElevationKeyX),
  _ncElevationKeyY(ncElevationKeyY),
  _lengthOfSimulationDomainInX(lengthOfSimulationDomainInX),
  _lengthOfSimulationDomainInY(lengthOfSimulationDomainInY) {

  this->parseElevationFile(elevationFilePath);
}


applications::exahype2::landslide::TopologyParser::~TopologyParser() {
  delete[] this->_elevation;
  delete[] this->_initV;
}


void applications::exahype2::landslide::TopologyParser::parseElevationFile(
  const char* elevationFilePath
) {
  int ncid = -1;

  if ((ncid = _netCDFReader.open(elevationFilePath)) == -1) {
    std::stringstream ss;
    ss << "Elevation file " << elevationFilePath << " could not be opened.";
    throw std::runtime_error(ss.str());
  }

  // Read x-dimension of elevation
  if ((_lengthOfElevationInX = _netCDFReader.getDimension(ncid, _ncElevationKeyX)) == 0) {
    throw std::runtime_error("Elevation x-dimension could not be read.");
  }

  // Read y-dimension of elevation
  if ((_lengthOfElevationInY = _netCDFReader.getDimension(ncid, _ncElevationKeyY)) == 0) {
    throw std::runtime_error("Elevation y-dimension could not be read.");
  }

  // Get max and min x
  {
    double tmp[_lengthOfElevationInX];
    std::memset(tmp, 0, _lengthOfElevationInX * sizeof(double));
    if (_netCDFReader.readVariable1D(ncid, _ncElevationKeyX, tmp) ==-1) {  // e.g. x values; 150, 300, 450 etc.
      throw std::runtime_error("Elevation x-indices could not be read.");
    }
    _minXCoordinateInElevation = tmp[0]; // 150
    _maxXCoordinateInElevation = tmp[_lengthOfElevationInX - 1]; // 450
  }

  // Get max and min y
  {
    double tmp[_lengthOfElevationInY];
    std::memset(tmp, 0, _lengthOfElevationInY * sizeof(double));
    if (_netCDFReader.readVariable1D(ncid, _ncElevationKeyY, tmp) == -1) {
      throw std::runtime_error("Elevation y-indices could not be read.");
    }
    _minYCoordinateInElevation = tmp[0];
    _maxYCoordinateInElevation = tmp[_lengthOfElevationInY - 1];
  }

  // Read elevation
  _elevation = new double[_lengthOfElevationInX * _lengthOfElevationInY];
  if (_netCDFReader.readVariable2D(ncid, _ncElevationKey, _elevation) == -1) {
    throw std::runtime_error("Elevation could not be read.");
  }

  // Close elevation netCDF file
  if (_netCDFReader.close(ncid) == -1) {
    throw std::runtime_error("Elevation file could not be closed.");
  }
}

void applications::exahype2::landslide::TopologyParser::parseInitVFile(
  const char* initVFilePath
) {
  int ncid = -1;

  if ((ncid = _netCDFReader.open(initVFilePath)) == -1) {
    std::stringstream ss;
    ss << "Initial volume file " << initVFilePath << " could not be opened.";
    throw std::runtime_error(ss.str());
  }

  // Read x-dimension of initV
  if ((_lengthOfInitVInX = _netCDFReader.getDimension(ncid, _ncInitVKeyX)) == 0) {
    throw std::runtime_error("Initial volume x-dimension could not be read.");
  }

  // Read y-dimension of initV
  if ((_lengthOfInitVInY = _netCDFReader.getDimension(ncid, _ncInitVKeyY)) == 0) {
    throw std::runtime_error("Initial volume y-dimension could not be read.");
  }

  // Get max and min x
  {
    double tmp[_lengthOfInitVInX];
    std::memset(tmp, 0, _lengthOfInitVInX * sizeof(double));
    if (_netCDFReader.readVariable1D(ncid, _ncInitVKeyX, tmp) == -1) {
      throw std::runtime_error("Initial volume x-indices could not be read.");
    }
    _minXCoordinateInInitV = tmp[0];
    _maxXCoordinateInInitV = tmp[_lengthOfInitVInX - 1];
  }

  // Get max and min y
  {
    double tmp[_lengthOfInitVInY];
    std::memset(tmp, 0, _lengthOfInitVInY * sizeof(double));
    if (_netCDFReader.readVariable1D(ncid, _ncInitVKeyY, tmp) == -1) {
      throw std::runtime_error("Initial volume y-indices could not be read.");
    }
     _minYCoordinateInInitV = tmp[0];
    _maxYCoordinateInInitV = tmp[_lengthOfInitVInY - 1];
  }

  // Read initV
  _initV = new double[_lengthOfInitVInX * _lengthOfInitVInY];
  if (_netCDFReader.readVariable2D(ncid, _ncInitVKey, _initV) == -1) {
    throw std::runtime_error("Initial volume could not be read.");
  }

  // Close initV netCDF file
  if (_netCDFReader.close(ncid) == -1) {
    throw std::runtime_error("Initial volume file could not be closed.");
  }
}

double applications::exahype2::landslide::TopologyParser::sampleElevation(
  double x,
  double y
) {
  const auto xCDF = transformIndexSimulationToCDFRange(
    x,
    _lengthOfSimulationDomainInX,
    _minXCoordinateInElevation,
    _maxXCoordinateInElevation,
    0
  );
  const auto yCDF = transformIndexSimulationToCDFRange(
    y,
    _lengthOfSimulationDomainInY,
    _minYCoordinateInElevation,
    _maxYCoordinateInElevation,
    1
  );

    // No defined elevation
  if (
    tarch::la::greater(xCDF, _maxXCoordinateInElevation) or
    tarch::la::smaller(xCDF, _minXCoordinateInElevation) or
    tarch::la::greater(yCDF, _maxYCoordinateInElevation) or
    tarch::la::smaller(yCDF, _minYCoordinateInElevation)
  ) {
    return 0.0;
  }

  return _elevation[transformIndexCDFRangeToArray(
    xCDF,
    yCDF,
    _minXCoordinateInElevation,
    _minYCoordinateInElevation,
    _maxXCoordinateInElevation,
    _maxYCoordinateInElevation,
    _lengthOfElevationInX,
    _lengthOfElevationInY
  )];
}


double applications::exahype2::landslide::TopologyParser::sampleInitV(
  double x,
  double y
) {
  const auto xCDF = transformIndexSimulationToCDFRange(
    x,
    _lengthOfSimulationDomainInX,
    _minXCoordinateInInitV,
    _maxXCoordinateInInitV,
    0
  );
  const auto yCDF = transformIndexSimulationToCDFRange(
    y,
    _lengthOfSimulationDomainInY,
    _minYCoordinateInInitV,
    _maxYCoordinateInInitV,
    1
  );

  // No defined initV
  if (
    tarch::la::greater(xCDF, _maxXCoordinateInInitV) or
    tarch::la::smaller(xCDF, _minXCoordinateInInitV) or
    tarch::la::greater(yCDF, _maxYCoordinateInInitV) or
    tarch::la::smaller(yCDF, _minYCoordinateInInitV)
  ) {
    return 0.0;
  }

  return _initV[transformIndexCDFRangeToArray(
    xCDF,
    yCDF,
    _minXCoordinateInInitV,
    _minYCoordinateInInitV,
    _maxXCoordinateInInitV,
    _maxYCoordinateInInitV,
    _lengthOfInitVInX,
    _lengthOfInitVInY
  )];
}


double applications::exahype2::landslide::TopologyParser::transformIndexSimulationToCDFRange(
  double coordinateOfDimensionZInSimulationSpace,
  double sizeOfSimulationSpaceInZDirection,
  double minIndexOfCDFspaceInZDirection,
  double maxIndexOfCDFspaceInZDirection,
  double domainOffsetInZDirection
) {
  // TODO: with an offset defined, minZ should probably be adjusted.
  // Take the DomainOffset into consideration when computing minZ and maxZ
  const double minZ = DomainOffset(domainOffsetInZDirection);
  const double maxZ = sizeOfSimulationSpaceInZDirection + minZ;

  const double m = ((maxIndexOfCDFspaceInZDirection - minIndexOfCDFspaceInZDirection) / (maxZ - minZ));

  // Correction for y-intercept in index interpolation.
  const double b = minIndexOfCDFspaceInZDirection - m * minZ;

  return m * coordinateOfDimensionZInSimulationSpace + b;
}


int applications::exahype2::landslide::TopologyParser::transformIndexCDFRangeToArray(
  double xCoordinateInCDFMesh,
  double yCoordinateInCDFMesh,
  double minXCoordinateInCDFMesh,
  double minYCoordinateInCDFMesh,
  double maxXCoordinateInCDFMesh,
  double maxYCoordinateInCDFMesh,
  std::size_t lengthOfXDimensionInCDFMesh,
  std::size_t lengthOfYDimensionInCDFMesh
) {
  int xIndex = -1;
  int yIndex = -1;

  {
    const std::size_t maxIndex = (lengthOfXDimensionInCDFMesh - 1);
    // Mesh width of data in x-dimension
    const double dx = ((minXCoordinateInCDFMesh - maxXCoordinateInCDFMesh) / (lengthOfXDimensionInCDFMesh - 1)); // (-1): divide by the amount of intervals and not points
    // f(0) = b
    const double b = minXCoordinateInCDFMesh / dx;
    const double m = maxIndex / (maxXCoordinateInCDFMesh - minXCoordinateInCDFMesh);
    xIndex = floor(m * xCoordinateInCDFMesh + b);
  }

  {
    const std::size_t maxIndex = (lengthOfYDimensionInCDFMesh - 1);
    // Mesh width of data in y-dimension
    const double dy = ((minYCoordinateInCDFMesh - maxYCoordinateInCDFMesh) / (lengthOfYDimensionInCDFMesh - 1));
    // f(0) = b
    const double b = minYCoordinateInCDFMesh / dy;
    const double m = maxIndex / (maxYCoordinateInCDFMesh - minYCoordinateInCDFMesh);
    yIndex = floor(m * yCoordinateInCDFMesh + b);
  }

  return yIndex * lengthOfXDimensionInCDFMesh + xIndex;
}
