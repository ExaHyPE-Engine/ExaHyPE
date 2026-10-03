// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NetCDFFieldSampler.h"

#ifdef PEANO_WITH_NETCDF

#include "tarch/Termination.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/la/ScalarOperations.h"

namespace {
  template <typename FieldDescription>
  std::string coordinateKeysToString(const FieldDescription& fieldDescription) {
    std::string result;
    for (const auto& key : fieldDescription.key) {
      if (not result.empty()) {
        result += ", ";
      }
      result += key;
    }
    return result;
  }
} // namespace

template <int FieldDimensions>
std::array<std::string, FieldDimensions>
tarch::reader::NetCDFFieldSampler<FieldDimensions>::getDefaultCoordinateKeys() {
  if constexpr (FieldDimensions == 3) {
    return {"x", "y", "z"};
  } else {
    return {"x", "y"};
  }
}

template <int FieldDimensions>
tarch::reader::NetCDFFieldSampler<FieldDimensions>::NetCDFFieldSampler(std::initializer_list<FieldDescription> fields) {
  for (const auto& field : fields) {
    parseFieldFile(field);
  }
}

/**
 * Maps the simulation domain onto the coordinate range of every field.
 */
template <int FieldDimensions>
tarch::reader::NetCDFFieldSampler<FieldDimensions>::NetCDFFieldSampler(
  std::initializer_list<FieldDescription>           fields,
  const tarch::la::Vector<FieldDimensions, double>& lengthOfSimulationDomain,
  const tarch::la::Vector<FieldDimensions, double>& offsetOfSimulationDomain
) :
  _lengthOfSimulationDomain(lengthOfSimulationDomain),
  _offsetOfSimulationDomain(offsetOfSimulationDomain) {
  for (const auto& field : fields) {
    parseFieldFile(field);
  }
}

template <int FieldDimensions>
tarch::reader::NetCDFFieldSampler<FieldDimensions>::NetCDFFieldSampler(const std::vector<FieldDescription>& fields) {
  for (const auto& field : fields) {
    parseFieldFile(field);
  }
}

template <int FieldDimensions>
tarch::reader::NetCDFFieldSampler<FieldDimensions>::NetCDFFieldSampler(
  const std::vector<FieldDescription>&              fields,
  const tarch::la::Vector<FieldDimensions, double>& lengthOfSimulationDomain,
  const tarch::la::Vector<FieldDimensions, double>& offsetOfSimulationDomain
) :
  _lengthOfSimulationDomain(lengthOfSimulationDomain),
  _offsetOfSimulationDomain(offsetOfSimulationDomain) {
  for (const auto& field : fields) {
    parseFieldFile(field);
  }
}

template <int FieldDimensions>
void
tarch::reader::NetCDFFieldSampler<FieldDimensions>::parseFieldFile(const FieldDescription& fieldDescription) {
  logDebug(
    "Parse field " << fieldDescription.name << " from " << fieldDescription.filePath << " with keys ("
                   << fieldDescription.valueKey << ", " << coordinateKeysToString(fieldDescription) << ")"
  );

  if (fieldDescription.name.empty()) {
    tarch::terminateWithError("Field name must not be empty.");
  }
  if (_fields.find(fieldDescription.name) != _fields.end()) {
    tarch::terminateWithError("Field \"" + fieldDescription.name + "\" is configured multiple times.");
  }
  for (int i = 0; i < FieldDimensions; i++) {
    if (fieldDescription.key[i].empty()) {
      tarch::terminateWithError(
        "Coordinate key " + std::to_string(i) + " of field \"" + fieldDescription.name + "\" is empty."
      );
    }
    // The default value key z is the conventional name of the third coordinate.
    if (fieldDescription.key[i] == fieldDescription.valueKey) {
      tarch::terminateWithError(
        "Field \"" + fieldDescription.name + "\" uses \"" + fieldDescription.valueKey
        + "\" both as its value key and as a coordinate key."
      );
    }
  }

  const int ncid = _netCDFFileReader.open(fieldDescription.filePath);
  if (ncid == -1) {
    tarch::terminateWithError("Field file " + fieldDescription.filePath + " could not be opened.");
  }

  Field field;

  for (int i = 0; i < FieldDimensions; i++) {
    field.length[i] = _netCDFFileReader.getDimension(ncid, fieldDescription.key[i].c_str());
    if (field.length[i] == 0) {
      tarch::terminateWithError("Field dimension \"" + fieldDescription.key[i] + "\" could not be read.");
    }

    {
      std::vector<double> coordinates(field.length[i], 0.0);
      if (_netCDFFileReader.readVariable(ncid, fieldDescription.key[i].c_str(), coordinates.data()) == -1) {
        tarch::terminateWithError("Field coordinates \"" + fieldDescription.key[i] + "\" could not be read.");
      }
      field.firstElement[i] = coordinates.front();
      field.lastElement[i]  = coordinates.back();
    }

    field.lengthOfSimulationDomain[i] = _lengthOfSimulationDomain.has_value()
      ? (*_lengthOfSimulationDomain)[i]
      : field.lastElement[i] - field.firstElement[i];
    field.offsetOfSimulationDomain[i] =
      _offsetOfSimulationDomain.has_value() ? (*_offsetOfSimulationDomain)[i] : field.firstElement[i];
  }

  std::size_t numberOfValues = 1;
  for (int i = 0; i < FieldDimensions; i++) {
    numberOfValues *= field.length[i];
  }
  field.values.resize(numberOfValues, 0.0);
  if (_netCDFFileReader.readVariable(ncid, fieldDescription.valueKey.c_str(), field.values.data()) == -1) {
    tarch::terminateWithError("Field values could not be read.");
  }

  if (_netCDFFileReader.close(ncid) == -1) {
    tarch::terminateWithError("Field file could not be closed.");
  }

  _fields.emplace(fieldDescription.name, std::move(field));
}

/**
 * Returns the value at the lower grid point along every axis, or zero outside the stored extent.
 * An unknown field name terminates.
 */
template <int FieldDimensions>
double
tarch::reader::NetCDFFieldSampler<FieldDimensions>::samplePiecewiseConstant(
  const std::string&                         fieldName,
  tarch::la::Vector<FieldDimensions, double> position
) const {
  const auto fieldIterator = _fields.find(fieldName);
  if (fieldIterator == _fields.end()) {
    tarch::terminateWithError("Unknown NetCDF field \"" + fieldName + "\".");
  }

  const Field& field = fieldIterator->second;

  tarch::la::Vector<FieldDimensions, double> positionInCDFRange;
  for (int i = 0; i < FieldDimensions; i++) {
    positionInCDFRange[i] = transformSimulationCoordinateToCDFRange(
      position[i],
      field.lengthOfSimulationDomain[i],
      field.firstElement[i],
      field.lastElement[i],
      field.offsetOfSimulationDomain[i]
    );
    if (not isCoordinateInRange(positionInCDFRange[i], field.firstElement[i], field.lastElement[i])) {
      return 0.0;
    }
  }

  return field
    .values[transformIndexCDFRangeToArray(positionInCDFRange, field.firstElement, field.lastElement, field.length)];
}

/**
 * Interpolates linearly between the surrounding grid points along every axis, which have to be
 * equidistant. A position outside the stored extent returns zero, and an unknown field name
 * terminates.
 */
template <int FieldDimensions>
double
tarch::reader::NetCDFFieldSampler<FieldDimensions>::sampleBilinear(
  const std::string&                         fieldName,
  tarch::la::Vector<FieldDimensions, double> position
) const {
  const auto fieldIterator = _fields.find(fieldName);
  if (fieldIterator == _fields.end()) {
    tarch::terminateWithError("Unknown NetCDF field \"" + fieldName + "\".");
  }

  const Field& field = fieldIterator->second;

  tarch::la::Vector<FieldDimensions, std::size_t> lowerIndex(std::size_t(0));
  tarch::la::Vector<FieldDimensions, std::size_t> upperIndex(std::size_t(0));
  tarch::la::Vector<FieldDimensions, double> fraction(0.0);

  for (int i = 0; i < FieldDimensions; i++) {
    const double coordinate = transformSimulationCoordinateToCDFRange(
      position[i],
      field.lengthOfSimulationDomain[i],
      field.firstElement[i],
      field.lastElement[i],
      field.offsetOfSimulationDomain[i]
    );
    if (not isCoordinateInRange(coordinate, field.firstElement[i], field.lastElement[i])) {
      return 0.0;
    }

    // The axes are equidistant between their first and last stored coordinate, so the
    // continuous index follows from the coordinate alone.
    double continuousIndex = 0.0;
    if (field.length[i] > 1 and not tarch::la::equals(field.firstElement[i], field.lastElement[i])) {
      continuousIndex = ((coordinate - field.firstElement[i]) / (field.lastElement[i] - field.firstElement[i]))
        * static_cast<double>(field.length[i] - 1);
      continuousIndex = std::clamp(continuousIndex, 0.0, static_cast<double>(field.length[i] - 1));
    }

    lowerIndex[i] = static_cast<std::size_t>(std::floor(continuousIndex));
    upperIndex[i] = std::min(lowerIndex[i] + 1, field.length[i] - 1);
    fraction[i]   = continuousIndex - static_cast<double>(lowerIndex[i]);
  }

  // The interpolation blends the 2^d surrounding grid points. The first coordinate varies fastest in
  // the stored array, so the flat index accumulates from the last dimension downwards.
  double result = 0.0;
  for (int corner = 0; corner < (1 << FieldDimensions); corner++) {
    double      weight    = 1.0;
    std::size_t flatIndex = 0;
    for (int i = FieldDimensions - 1; i >= 0; i--) {
      const bool takeUpper = ((corner >> i) & 1) == 1;
      weight *= takeUpper ? fraction[i] : 1.0 - fraction[i];
      flatIndex = flatIndex * field.length[i] + (takeUpper ? upperIndex[i] : lowerIndex[i]);
    }
    result += weight * field.values[flatIndex];
  }

  return result;
}

/**
 * Maps a simulation coordinate linearly onto the netCDF coordinate range from
 * minIndexOfCDFspaceInZDirection to maxIndexOfCDFspaceInZDirection.
 */
template <int FieldDimensions>
double
tarch::reader::NetCDFFieldSampler<FieldDimensions>::transformSimulationCoordinateToCDFRange(
  double coordinateOfDimensionZInSimulationSpace,
  double sizeOfSimulationSpaceInZDirection,
  double minIndexOfCDFspaceInZDirection,
  double maxIndexOfCDFspaceInZDirection,
  double offsetOfSimulationSpaceInZDirection
) {
  const double minZ = offsetOfSimulationSpaceInZDirection;
  const double maxZ = sizeOfSimulationSpaceInZDirection + minZ;

  const double slope     = ((maxIndexOfCDFspaceInZDirection - minIndexOfCDFspaceInZDirection) / (maxZ - minZ));
  const double intercept = minIndexOfCDFspaceInZDirection - slope * minZ;

  return slope * coordinateOfDimensionZInSimulationSpace + intercept;
}

/**
 * Returns the flat array index of the grid point at or below a netCDF coordinate tuple, with the
 * first coordinate varying fastest.
 */
template <int FieldDimensions>
std::size_t
tarch::reader::NetCDFFieldSampler<FieldDimensions>::transformIndexCDFRangeToArray(
  tarch::la::Vector<FieldDimensions, double>      coordinatesInCDFMesh,
  tarch::la::Vector<FieldDimensions, double>      firstCoordinatesInCDFMesh,
  tarch::la::Vector<FieldDimensions, double>      lastCoordinatesInCDFMesh,
  tarch::la::Vector<FieldDimensions, std::size_t> lengthOfDimensionsInCDFMesh
) {
  const auto computeIndex =
    [](double coordinate, double firstCoordinate, double lastCoordinate, std::size_t length) -> std::size_t {
    if (length <= 1 or tarch::la::equals(firstCoordinate, lastCoordinate)) {
      return 0;
    }

    const double index = ((coordinate - firstCoordinate) / (lastCoordinate - firstCoordinate)) * (length - 1);
    return static_cast<std::size_t>(
      std::clamp(static_cast<long long>(std::floor(index)), 0LL, static_cast<long long>(length - 1))
    );
  };

  // A 3D field can exceed 2^31 entries, so the flat index is a std::size_t.
  std::size_t index  = 0;
  std::size_t length = 1;
  for (int i = 0; i < FieldDimensions; i++) {
    index += length
      * computeIndex(
               coordinatesInCDFMesh[i],
               firstCoordinatesInCDFMesh[i],
               lastCoordinatesInCDFMesh[i],
               lengthOfDimensionsInCDFMesh[i]
      );
    length *= lengthOfDimensionsInCDFMesh[i];
  }

  return index;
}

template <int FieldDimensions>
bool
tarch::reader::NetCDFFieldSampler<FieldDimensions>::isCoordinateInRange(
  double coordinate,
  double firstCoordinate,
  double lastCoordinate
) {
  const auto [lower, upper] = std::minmax(firstCoordinate, lastCoordinate);
  return not tarch::la::smaller(coordinate, lower) and not tarch::la::greater(coordinate, upper);
}

template class tarch::reader::NetCDFFieldSampler<2>;
template class tarch::reader::NetCDFFieldSampler<3>;

#endif
