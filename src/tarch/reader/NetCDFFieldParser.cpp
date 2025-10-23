// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NetCDFFieldParser.h"

#ifdef UseNetCDF

#include "tarch/la/ScalarOperations.h"

#include <cstring>
#include <stdexcept>

tarch::logging::Log tarch::reader::NetCDFFieldParser::_log("tarch::reader::NetCDFFieldParser");

tarch::reader::NetCDFFieldParser::NetCDFFieldParser(
  const char* topologyFilePath,
  const char* displacementFilePath,
  double      lengthOfSimulationDomainInX,
  double      lengthOfSimulationDomainInY,
  double      offsetOfSimulationDomainInX,
  double      offsetOfSimulationDomainInY,
  const char* ncTopologyKey,
  const char* ncTopologyKeyX,
  const char* ncTopologyKeyY,
  const char* ncDisplacementKey,
  const char* ncDisplacementKeyX,
  const char* ncDisplacementKeyY
):
  _ncTopologyKey(ncTopologyKey),
  _ncTopologyKeyX(ncTopologyKeyX),
  _ncTopologyKeyY(ncTopologyKeyY),
  _ncDisplacementKey(ncDisplacementKey),
  _ncDisplacementKeyX(ncDisplacementKeyX),
  _ncDisplacementKeyY(ncDisplacementKeyY),
  _lengthOfSimulationDomainInX(lengthOfSimulationDomainInX),
  _lengthOfSimulationDomainInY(lengthOfSimulationDomainInY),
  _offsetOfSimulationDomainInX(offsetOfSimulationDomainInX),
  _offsetOfSimulationDomainInY(offsetOfSimulationDomainInY) {

  this->parseTopologyFile(topologyFilePath);
  this->parseDisplacementFile(displacementFilePath);
}

tarch::reader::NetCDFFieldParser::NetCDFFieldParser(
  const char* topologyFilePath,
  double      lengthOfSimulationDomainInX,
  double      lengthOfSimulationDomainInY,
  double      offsetOfSimulationDomainInX,
  double      offsetOfSimulationDomainInY,
  const char* ncTopologyKey,
  const char* ncTopologyKeyX,
  const char* ncTopologyKeyY
):
  _ncTopologyKey(ncTopologyKey),
  _ncTopologyKeyX(ncTopologyKeyX),
  _ncTopologyKeyY(ncTopologyKeyY),
  _lengthOfSimulationDomainInX(lengthOfSimulationDomainInX),
  _lengthOfSimulationDomainInY(lengthOfSimulationDomainInY),
  _offsetOfSimulationDomainInX(offsetOfSimulationDomainInX),
  _offsetOfSimulationDomainInY(offsetOfSimulationDomainInY) {

  this->parseTopologyFile(topologyFilePath);
}

tarch::reader::NetCDFFieldParser::~NetCDFFieldParser() {
  delete[] this->_topology;
  delete[] this->_displacement;
}

void tarch::reader::NetCDFFieldParser::parseTopologyFile(const char* topologyFilePath) {
  logDebug("parseTopologyFile()", "Parse topology file with keys (" << _ncTopologyKey << ", " << _ncTopologyKeyX << ", " << _ncTopologyKeyY << ")");

  int ncid = -1;

  if ((ncid = _netCDFFileReader.open(topologyFilePath)) == -1) {
    std::stringstream ss;
    ss << "Topology file " << topologyFilePath << " could not be opened.";
    throw std::runtime_error(ss.str());
  }

  // Read x-dimension of topology
  if ((_lengthOfTopologyInX = _netCDFFileReader.getDimension(ncid, _ncTopologyKeyX)) == 0) {
    throw std::runtime_error("Topology x-dimension could not be read.");
  }

  // Read y-dimension of topology
  if ((_lengthOfTopologyInY = _netCDFFileReader.getDimension(ncid, _ncTopologyKeyY)) == 0) {
    throw std::runtime_error("Topology y-dimension could not be read.");
  }

  // Get max and min x
  {
    double tmp[_lengthOfTopologyInX];
    std::memset(tmp, 0, _lengthOfTopologyInX * sizeof(double));
    if (_netCDFFileReader.readVariable1D(ncid, _ncTopologyKeyX, tmp) == -1) {
      throw std::runtime_error("Topology x-indices could not be read.");
    }
    _minXCoordinateInTopology = tmp[0];
    _maxXCoordinateInTopology = tmp[_lengthOfTopologyInX - 1];
  }

  // Get max and min y
  {
    double tmp[_lengthOfTopologyInY];
    std::memset(tmp, 0, _lengthOfTopologyInY * sizeof(double));
    if (_netCDFFileReader.readVariable1D(ncid, _ncTopologyKeyY, tmp) == -1) {
      throw std::runtime_error("Topology y-indices could not be read.");
    }
    _minYCoordinateInTopology = tmp[0];
    _maxYCoordinateInTopology = tmp[_lengthOfTopologyInY - 1];
  }

  // Read topology
  _topology = new double[_lengthOfTopologyInX * _lengthOfTopologyInY];
  if (_netCDFFileReader.readVariable2D(ncid, _ncTopologyKey, _topology) == -1) {
    throw std::runtime_error("Topology could not be read.");
  }

  // Close topology netCDF file
  if (_netCDFFileReader.close(ncid) == -1) {
    throw std::runtime_error("Topology file could not be closed.");
  }
}

void tarch::reader::NetCDFFieldParser::parseDisplacementFile(const char* displacementFilePath) {
  logDebug("parseDisplacementFile()", "Parse displacement file with keys (" << _ncDisplacementKey << ", " << _ncDisplacementKeyX << ", " << _ncDisplacementKeyY << ")");

  int ncid = -1;

  if ((ncid = _netCDFFileReader.open(displacementFilePath)) == -1) {
    std::stringstream ss;
    ss << "Displacement file " << displacementFilePath << " could not be opened.";
    throw std::runtime_error(ss.str());
  }

  // Read x-dimension of displacement
  if ((_lengthOfDisplacementInX = _netCDFFileReader.getDimension(ncid, _ncDisplacementKeyX)) == 0) {
    throw std::runtime_error("Displacement x-dimension could not be read.");
  }

  // Read y-dimension of displacement
  if ((_lengthOfDisplacementInY = _netCDFFileReader.getDimension(ncid, _ncDisplacementKeyY)) == 0) {
    throw std::runtime_error("Displacement y-dimension could not be read.");
  }

  // Get max and min x
  {
    double tmp[_lengthOfDisplacementInX];
    std::memset(tmp, 0, _lengthOfDisplacementInX * sizeof(double));
    if (_netCDFFileReader.readVariable1D(ncid, _ncDisplacementKeyX, tmp) == -1) {
      throw std::runtime_error("Displacement x-indices could not be read.");
    }
    _minXCoordinateInDisplacement = tmp[0];
    _maxXCoordinateInDisplacement = tmp[_lengthOfDisplacementInX - 1];
  }

  // Get max and min y
  {
    double tmp[_lengthOfDisplacementInY];
    std::memset(tmp, 0, _lengthOfDisplacementInY * sizeof(double));
    if (_netCDFFileReader.readVariable1D(ncid, _ncDisplacementKeyY, tmp) == -1) {
      throw std::runtime_error("Displacement y-indices could not be read.");
    }
    _minYCoordinateInDisplacement = tmp[0];
    _maxYCoordinateInDisplacement = tmp[_lengthOfDisplacementInY - 1];
  }

  // Read displacement
  _displacement = new double[_lengthOfDisplacementInX * _lengthOfDisplacementInY];
  if (_netCDFFileReader.readVariable2D(ncid, _ncDisplacementKey, _displacement) == -1) {
    throw std::runtime_error("Displacement could not be read.");
  }

  // Close displacement netCDF file
  if (_netCDFFileReader.close(ncid) == -1) {
    throw std::runtime_error("Displacement file could not be closed.");
  }
}

double tarch::reader::NetCDFFieldParser::sampleTopology(double x, double y) {
  const auto xCDF = transformIndexSimulationToCDFRange(
    x,
    _lengthOfSimulationDomainInX,
    _minXCoordinateInTopology,
    _maxXCoordinateInTopology,
    _offsetOfSimulationDomainInX
  );
  const auto yCDF = transformIndexSimulationToCDFRange(
    y,
    _lengthOfSimulationDomainInY,
    _minYCoordinateInTopology,
    _maxYCoordinateInTopology,
    _offsetOfSimulationDomainInY
  );

  // No defined topology
  if (
    tarch::la::greater(xCDF, _maxXCoordinateInTopology) or
    tarch::la::smaller(xCDF, _minXCoordinateInTopology) or
    tarch::la::greater(yCDF, _maxYCoordinateInTopology) or
    tarch::la::smaller(yCDF, _minYCoordinateInTopology)
  ) {
    return 0.0;
  }

  return _topology[transformIndexCDFRangeToArray(
    xCDF,
    yCDF,
    _minXCoordinateInTopology,
    _minYCoordinateInTopology,
    _maxXCoordinateInTopology,
    _maxYCoordinateInTopology,
    _lengthOfTopologyInX,
    _lengthOfTopologyInY
  )];
}

double tarch::reader::NetCDFFieldParser::sampleDisplacement(double x, double y) {
  const auto xCDF = transformIndexSimulationToCDFRange(
    x,
    _lengthOfSimulationDomainInX,
    _minXCoordinateInTopology,
    _maxXCoordinateInTopology,
    _offsetOfSimulationDomainInX
  );
  const auto yCDF = transformIndexSimulationToCDFRange(
    y,
    _lengthOfSimulationDomainInY,
    _minYCoordinateInTopology,
    _maxYCoordinateInTopology,
    _offsetOfSimulationDomainInY
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

double tarch::reader::NetCDFFieldParser::transformIndexSimulationToCDFRange(
  double coordinateOfDimensionZInSimulationSpace,
  double sizeOfSimulationSpaceInZDirection,
  double minIndexOfCDFspaceInZDirection,
  double maxIndexOfCDFspaceInZDirection,
  double offsetOfSimulationSpaceInZDirection
) {
  const double minZ = offsetOfSimulationSpaceInZDirection;
  const double maxZ = sizeOfSimulationSpaceInZDirection + minZ;

  const double m = ((maxIndexOfCDFspaceInZDirection - minIndexOfCDFspaceInZDirection) / (maxZ - minZ));

  // Correction for y-intercept in index interpolation.
  const double b = minIndexOfCDFspaceInZDirection - m * minZ;

  return m * coordinateOfDimensionZInSimulationSpace + b;
}

int tarch::reader::NetCDFFieldParser::transformIndexCDFRangeToArray(
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
    const double dx = ((minXCoordinateInCDFMesh - maxXCoordinateInCDFMesh) / (lengthOfXDimensionInCDFMesh - 1)); // (-1): divide by the amount of intervals and not points
    // f(0) = b
    const double b = minXCoordinateInCDFMesh / dx;
    const double m = maxIndex / (maxXCoordinateInCDFMesh - minXCoordinateInCDFMesh);
    xIndex         = std::floor(m * xCoordinateInCDFMesh + b);
  }

  {
    const std::size_t maxIndex = (lengthOfYDimensionInCDFMesh - 1);
    // Mesh width of data in y-dimension
    const double dy = ((minYCoordinateInCDFMesh - maxYCoordinateInCDFMesh) / (lengthOfYDimensionInCDFMesh - 1));
    // f(0) = b
    const double b = minYCoordinateInCDFMesh / dy;
    const double m = maxIndex / (maxYCoordinateInCDFMesh - minYCoordinateInCDFMesh);
    yIndex         = std::floor(m * yCoordinateInCDFMesh + b);
  }

  return yIndex * lengthOfXDimensionInCDFMesh + xIndex;
}

#endif
