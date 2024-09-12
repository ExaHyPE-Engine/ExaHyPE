// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TopologyParser.h"

#include <cstring>

#include "Constants.h"

applications::exahype2::swe::TopologyParser::TopologyParser(
  const char* bathymetryFilePath,
  const char* displacementFilePath,
  const char* ncBathymetryKey,
  const char* ncBathymetryKeyX,
  const char* ncBathymetryKeyY,
  const char* ncDisplacementKey,
  const char* ncDisplacementKeyX,
  const char* ncDisplacementKeyY
):
  _ncBathymetryKey(ncBathymetryKey),
  _ncBathymetryKeyX(ncBathymetryKeyX),
  _ncBathymetryKeyY(ncBathymetryKeyY),
  _ncDisplacementKey(ncDisplacementKey),
  _ncDisplacementKeyX(ncDisplacementKeyX),
  _ncDisplacementKeyY(ncDisplacementKeyY) {

  _lengthOfSimulationDomainInX = DomainSize(0);
  _lengthOfSimulationDomainInY = DomainSize(1);

  this->parseBathymetryFile(bathymetryFilePath);
  this->parseDisplacementFile(displacementFilePath);
}


applications::exahype2::swe::TopologyParser::TopologyParser(
  const char* bathymetryFilePath,
  double      lengthOfSimulationDomainInX,
  double      lengthOfSimulationDomainInY,
  const char* ncBathymetryKey,
  const char* ncBathymetryKeyX,
  const char* ncBathymetryKeyY
):
  _ncBathymetryKey(ncBathymetryKey),
  _ncBathymetryKeyX(ncBathymetryKeyX),
  _ncBathymetryKeyY(ncBathymetryKeyY),
  _lengthOfSimulationDomainInX(lengthOfSimulationDomainInX),
  _lengthOfSimulationDomainInY(lengthOfSimulationDomainInY) {

  this->parseBathymetryFile(bathymetryFilePath);
}


applications::exahype2::swe::TopologyParser::~TopologyParser() {
  delete[] this->_bathymetry;
  delete[] this->_displacement;
}


void applications::exahype2::swe::TopologyParser::parseBathymetryFile(
  const char* bathymetryFilePath
) {
  int ncid = -1;

  if ((ncid = _netCDFReader.open(bathymetryFilePath)) == -1) {
    std::stringstream ss;
    ss << "Bathymetry file " << bathymetryFilePath << " could not be opened.";
    throw std::runtime_error(ss.str());
  }

  // Read x-dimension of bathymetry
  if ((_lengthOfBathymetryInX = _netCDFReader.getDimension(ncid, _ncBathymetryKeyX)) == 0) {
    throw std::runtime_error("Bathymetry x-dimension could not be read.");
  }

  // Read y-dimension of bathymetry
  if ((_lengthOfBathymetryInY = _netCDFReader.getDimension(ncid, _ncBathymetryKeyY)) == 0) {
    throw std::runtime_error("Bathymetry y-dimension could not be read.");
  }

  // Get max and min x
  {
    double tmp[_lengthOfBathymetryInX];
    std::memset(tmp, 0, _lengthOfBathymetryInX * sizeof(double));
    if (_netCDFReader.readVariable1D(ncid, _ncBathymetryKeyX, tmp) == -1) {
      throw std::runtime_error("Bathymetry x-indices could not be read.");
    }
    _minXCoordinateInBathymetry = tmp[0];
    _maxXCoordinateInBathymetry = tmp[_lengthOfBathymetryInX - 1];
  }

  // Get max and min y
  {
    double tmp[_lengthOfBathymetryInY];
    std::memset(tmp, 0, _lengthOfBathymetryInY * sizeof(double));
    if (_netCDFReader.readVariable1D(ncid, _ncBathymetryKeyY, tmp) == -1) {
      throw std::runtime_error("Bathymetry y-indices could not be read.");
    }
    _minYCoordinateInBathymetry = tmp[0];
    _maxYCoordinateInBathymetry = tmp[_lengthOfBathymetryInY - 1];
  }

  // Read bathymetry
  _bathymetry = new double[_lengthOfBathymetryInX * _lengthOfBathymetryInY];
  if (_netCDFReader.readVariable2D(ncid, _ncBathymetryKey, _bathymetry) == -1) {
    throw std::runtime_error("Bathymetry could not be read.");
  }

  // Close bathymetry netCDF file
  if (_netCDFReader.close(ncid) == -1) {
    throw std::runtime_error("Bathymetry file could not be closed.");
  }
}


void applications::exahype2::swe::TopologyParser::parseDisplacementFile(
  const char* displacementFilePath
) {
  int ncid = -1;

  if ((ncid = _netCDFReader.open(displacementFilePath)) == -1) {
    std::stringstream ss;
    ss << "Displacement file " << displacementFilePath
       << " could not be opened.";
    throw std::runtime_error(ss.str());
  }

  // Read x-dimension of displacement
  if ((_lengthOfDisplacementInX = _netCDFReader.getDimension(ncid, _ncDisplacementKeyX)) == 0) {
    throw std::runtime_error("Displacement x-dimension could not be read.");
  }

  // Read y-dimension of displacement
  if ((_lengthOfDisplacementInY = _netCDFReader.getDimension(ncid, _ncDisplacementKeyY)) == 0) {
    throw std::runtime_error("Displacement y-dimension could not be read.");
  }

  // Get max and min x
  {
    double tmp[_lengthOfDisplacementInX];
    std::memset(tmp, 0, _lengthOfDisplacementInX * sizeof(double));
    if (_netCDFReader.readVariable1D(ncid, _ncDisplacementKeyX, tmp) == -1) {
      throw std::runtime_error("Displacement x-indices could not be read.");
    }
    _minXCoordinateInDisplacement = tmp[0];
    _maxXCoordinateInDisplacement = tmp[_lengthOfDisplacementInX - 1];
  }

  // Get max and min y
  {
    double tmp[_lengthOfDisplacementInY];
    std::memset(tmp, 0, _lengthOfDisplacementInY * sizeof(double));
    if (_netCDFReader.readVariable1D(ncid, _ncDisplacementKeyY, tmp) == -1) {
      throw std::runtime_error("Displacement y-indices could not be read.");
    }
    _minYCoordinateInDisplacement = tmp[0];
    _maxYCoordinateInDisplacement = tmp[_lengthOfDisplacementInY - 1];
  }

  // Read displacement
  _displacement = new double
    [_lengthOfDisplacementInX * _lengthOfDisplacementInY];
  if (_netCDFReader.readVariable2D(ncid, _ncDisplacementKey, _displacement) == -1) {
    throw std::runtime_error("Displacement could not be read.");
  }

  // Close displacement netCDF file
  if (_netCDFReader.close(ncid) == -1) {
    throw std::runtime_error("Displacement file could not be closed.");
  }
}


double applications::exahype2::swe::TopologyParser::sampleBathymetry(
  double x,
  double y
) {
  const auto xCDF = transformIndexSimulationToCDFRange(
    x,
    _lengthOfSimulationDomainInX,
    _minXCoordinateInBathymetry,
    _maxXCoordinateInBathymetry
  );
  const auto yCDF = transformIndexSimulationToCDFRange(
    y,
    _lengthOfSimulationDomainInY,
    _minYCoordinateInBathymetry,
    _maxYCoordinateInBathymetry
  );

  // No defined bathymetry
  if (
    tarch::la::greater(xCDF, _maxXCoordinateInBathymetry) or
    tarch::la::smaller(xCDF, _minXCoordinateInBathymetry) or
    tarch::la::greater(yCDF, _maxYCoordinateInBathymetry) or
    tarch::la::smaller(yCDF, _minYCoordinateInBathymetry)
  ) {
    return 0.0;
  }

  return _bathymetry[transformIndexCDFRangeToArray(
    xCDF,
    yCDF,
    _minXCoordinateInBathymetry,
    _minYCoordinateInBathymetry,
    _maxXCoordinateInBathymetry,
    _maxYCoordinateInBathymetry,
    _lengthOfBathymetryInX,
    _lengthOfBathymetryInY
  )];
}


double applications::exahype2::swe::TopologyParser::sampleDisplacement(
  double x,
  double y
) {
  const auto xCDF = transformIndexSimulationToCDFRange(
    x,
    _lengthOfSimulationDomainInX,
    _minXCoordinateInBathymetry,
    _maxXCoordinateInBathymetry
  );
  const auto yCDF = transformIndexSimulationToCDFRange(
    y,
    _lengthOfSimulationDomainInY,
    _minYCoordinateInBathymetry,
    _maxYCoordinateInBathymetry
  );

  // No defined displacement
  if (
    tarch::la::greater(xCDF, _maxXCoordinateInDisplacement) or
    tarch::la::smaller(xCDF, _minXCoordinateInDisplacement) or
    tarch::la::greater(yCDF, _maxYCoordinateInDisplacement) or
    tarch::la::smaller(yCDF, _minYCoordinateInDisplacement)
  ) {
    return 0.0;
  }

  return _displacement[transformIndexCDFRangeToArray(
    xCDF,
    yCDF,
    _minXCoordinateInDisplacement,
    _minYCoordinateInDisplacement,
    _maxXCoordinateInDisplacement,
    _maxYCoordinateInDisplacement,
    _lengthOfDisplacementInX,
    _lengthOfDisplacementInY
  )];
}


double applications::exahype2::swe::TopologyParser::
  transformIndexSimulationToCDFRange(
    double coordinateOfDimensionZInSimulationSpace,
    double sizeOfSimulationSpaceInZDirection,
    double minIndexOfCDFspaceInZDirection,
    double maxIndexOfCDFspaceInZDirection
  ) {
  // TODO: with an offset defined, minZ should probably be adjusted.
  const double minZ = 0.0;
  const double maxZ = sizeOfSimulationSpaceInZDirection;
  const double m
    = ((maxIndexOfCDFspaceInZDirection - minIndexOfCDFspaceInZDirection) / (maxZ - minZ));
  const double b = minIndexOfCDFspaceInZDirection - m * minZ; // Correction for
                                                              // y-intercept in
                                                              // index
                                                              // interpolation.
  return m * coordinateOfDimensionZInSimulationSpace + b;
}


int applications::exahype2::swe::TopologyParser::transformIndexCDFRangeToArray(
  double      xCoordinateInCDFMesh,
  double      yCoordinateInCDFMesh,
  double      minXCoordinateInCDFMesh,
  double      minYCoordinateInCDFMesh,
  double      maxXCoordinateInCDFMesh,
  double      maxYCoordinateInCDFMesh,
  std::size_t lengthOfXDimensionInCDFMesh,
  std::size_t lengthOfYDimensionInCDFMesh
) {
  int xIndex = -1;
  int yIndex = -1;

  {
    const std::size_t maxIndex = (lengthOfXDimensionInCDFMesh - 1);
    // Mesh width of data in x-dimension
    const double dx
      = ((minXCoordinateInCDFMesh - maxXCoordinateInCDFMesh) / lengthOfXDimensionInCDFMesh);
    // f(0) = b
    const double b = minXCoordinateInCDFMesh / dx;
    const double m = maxIndex
                     / (maxXCoordinateInCDFMesh - minXCoordinateInCDFMesh);
    xIndex = floor(m * xCoordinateInCDFMesh + b);
  }

  {
    const std::size_t maxIndex = (lengthOfYDimensionInCDFMesh - 1);
    // Mesh width of data in y-dimension
    const double dy
      = ((minYCoordinateInCDFMesh - maxYCoordinateInCDFMesh) / lengthOfYDimensionInCDFMesh);
    // f(0) = b
    const double b = minYCoordinateInCDFMesh / dy;
    const double m = maxIndex
                     / (maxYCoordinateInCDFMesh - minYCoordinateInCDFMesh);
    yIndex = floor(m * yCoordinateInCDFMesh + b);
  }

  return yIndex * lengthOfXDimensionInCDFMesh + xIndex;
}
