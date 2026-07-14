// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NetCDFFieldSampler.h"

#ifdef PEANO_WITH_NETCDF

#include "tarch/instrumentation/Log.h"
#include "tarch/la/ScalarOperations.h"

tarch::reader::NetCDFFieldSampler::NetCDFFieldSampler(
  std::initializer_list<FieldDescription> fields,
  double                                  lengthOfSimulationDomainInX,
  double                                  lengthOfSimulationDomainInY,
  double                                  offsetOfSimulationDomainInX,
  double                                  offsetOfSimulationDomainInY
):
  _lengthOfSimulationDomainInX(lengthOfSimulationDomainInX),
  _lengthOfSimulationDomainInY(lengthOfSimulationDomainInY),
  _offsetOfSimulationDomainInX(offsetOfSimulationDomainInX),
  _offsetOfSimulationDomainInY(offsetOfSimulationDomainInY) {
  for (const auto& field : fields) {
    parseFieldFile(field);
  }
}

tarch::reader::NetCDFFieldSampler::NetCDFFieldSampler(
  const std::vector<FieldDescription>& fields,
  double                               lengthOfSimulationDomainInX,
  double                               lengthOfSimulationDomainInY,
  double                               offsetOfSimulationDomainInX,
  double                               offsetOfSimulationDomainInY
):
  _lengthOfSimulationDomainInX(lengthOfSimulationDomainInX),
  _lengthOfSimulationDomainInY(lengthOfSimulationDomainInY),
  _offsetOfSimulationDomainInX(offsetOfSimulationDomainInX),
  _offsetOfSimulationDomainInY(offsetOfSimulationDomainInY) {
  for (const auto& field : fields) {
    parseFieldFile(field);
  }
}

void tarch::reader::NetCDFFieldSampler::parseFieldFile(const FieldDescription& fieldDescription) {
  logDebug(
    "Parse field "
    << fieldDescription.name << " from " << fieldDescription.filePath << " with keys (" << fieldDescription.valueKey
    << ", " << fieldDescription.xKey << ", " << fieldDescription.yKey << ")"
  );

  if (fieldDescription.name.empty()) {
    throw std::runtime_error("Field name must not be empty.");
  }
  if (_fields.find(fieldDescription.name) != _fields.end()) {
    throw std::runtime_error("Field \"" + fieldDescription.name + "\" is configured multiple times.");
  }

  int ncid = -1;

  if ((ncid = _netCDFFileReader.open(fieldDescription.filePath)) == -1) {
    std::stringstream ss;
    ss << "Field file " << fieldDescription.filePath << " could not be opened.";
    throw std::runtime_error(ss.str());
  }

  Field field;

  if ((field.lengthInX = _netCDFFileReader.getDimension(ncid, fieldDescription.xKey.c_str())) == 0) {
    throw std::runtime_error("Field x-dimension could not be read.");
  }
  if ((field.lengthInY = _netCDFFileReader.getDimension(ncid, fieldDescription.yKey.c_str())) == 0) {
    throw std::runtime_error("Field y-dimension could not be read.");
  }

  {
    std::vector<double> tmp(field.lengthInX, 0.0);
    if (_netCDFFileReader.readVariable1D(ncid, fieldDescription.xKey.c_str(), tmp.data()) == -1) {
      throw std::runtime_error("Field x coordinates could not be read.");
    }
    field.firstX = tmp.front();
    field.lastX  = tmp.back();
  }

  {
    std::vector<double> tmp(field.lengthInY, 0.0);
    if (_netCDFFileReader.readVariable1D(ncid, fieldDescription.yKey.c_str(), tmp.data()) == -1) {
      throw std::runtime_error("Field y coordinates could not be read.");
    }
    field.firstY = tmp.front();
    field.lastY  = tmp.back();
  }

  field.values.resize(field.lengthInX * field.lengthInY, 0.0);
  if (_netCDFFileReader.readVariable2D(ncid, fieldDescription.valueKey.c_str(), field.values.data()) == -1) {
    throw std::runtime_error("Field values could not be read.");
  }

  if (_netCDFFileReader.close(ncid) == -1) {
    throw std::runtime_error("Field file could not be closed.");
  }

  _fields.emplace(fieldDescription.name, std::move(field));
}

double tarch::reader::NetCDFFieldSampler::sample(const std::string& fieldName, double x, double y) const {
  const auto fieldIterator = _fields.find(fieldName);
  if (fieldIterator == _fields.end()) {
    throw std::runtime_error("Unknown NetCDF field \"" + fieldName + "\".");
  }

  const Field& field = fieldIterator->second;

  const auto xCDF = transformSimulationCoordinateToCDFRange(
    x,
    _lengthOfSimulationDomainInX,
    field.firstX,
    field.lastX,
    _offsetOfSimulationDomainInX
  );
  const auto yCDF = transformSimulationCoordinateToCDFRange(
    y,
    _lengthOfSimulationDomainInY,
    field.firstY,
    field.lastY,
    _offsetOfSimulationDomainInY
  );

  if (not isCoordinateInRange(xCDF, field.firstX, field.lastX)
      or not isCoordinateInRange(yCDF, field.firstY, field.lastY)) {
    return 0.0;
  }

  return field.values[transformIndexCDFRangeToArray(
    xCDF,
    yCDF,
    field.firstX,
    field.firstY,
    field.lastX,
    field.lastY,
    field.lengthInX,
    field.lengthInY
  )];
}

double tarch::reader::NetCDFFieldSampler::transformSimulationCoordinateToCDFRange(
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

int tarch::reader::NetCDFFieldSampler::transformIndexCDFRangeToArray(
  double      xCoordinateInCDFMesh,
  double      yCoordinateInCDFMesh,
  double      firstXCoordinateInCDFMesh,
  double      firstYCoordinateInCDFMesh,
  double      lastXCoordinateInCDFMesh,
  double      lastYCoordinateInCDFMesh,
  std::size_t lengthOfXDimensionInCDFMesh,
  std::size_t lengthOfYDimensionInCDFMesh
) {
  const auto computeIndex =
    [](double coordinate, double firstCoordinate, double lastCoordinate, std::size_t length) -> int {
    if (length <= 1 or tarch::la::equals(firstCoordinate, lastCoordinate)) {
      return 0;
    }

    const double index = ((coordinate - firstCoordinate) / (lastCoordinate - firstCoordinate)) * (length - 1);
    return std::clamp(static_cast<int>(std::floor(index)), 0, static_cast<int>(length - 1));
  };

  const int xIndex = computeIndex(
    xCoordinateInCDFMesh,
    firstXCoordinateInCDFMesh,
    lastXCoordinateInCDFMesh,
    lengthOfXDimensionInCDFMesh
  );
  const int yIndex = computeIndex(
    yCoordinateInCDFMesh,
    firstYCoordinateInCDFMesh,
    lastYCoordinateInCDFMesh,
    lengthOfYDimensionInCDFMesh
  );

  return yIndex * lengthOfXDimensionInCDFMesh + xIndex;
}

bool tarch::reader::NetCDFFieldSampler::
  isCoordinateInRange(double coordinate, double firstCoordinate, double lastCoordinate) {
  const auto [lower, upper] = std::minmax(firstCoordinate, lastCoordinate);
  return not tarch::la::smaller(coordinate, lower) and not tarch::la::greater(coordinate, upper);
}

#endif
