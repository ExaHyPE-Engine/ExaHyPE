// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NetCDFFieldSampler.h"

#ifdef PEANO_WITH_NETCDF

#include "tarch/Assertions.h"
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
    "The NetCDF reader parses the field " << fieldDescription.name << " from " << fieldDescription.filePath
                                          << " with the keys (" << fieldDescription.valueKey << ", "
                                          << coordinateKeysToString(fieldDescription) << ")."
  );

  if (fieldDescription.name.empty()) {
    tarch::terminateWithError("A NetCDF field expects a name.");
  }
  if (_fields.find(fieldDescription.name) != _fields.end()) {
    tarch::terminateWithError(
      "The configuration may name the NetCDF field \"" + fieldDescription.name + "\" once only."
    );
  }
  for (int i = 0; i < FieldDimensions; i++) {
    if (fieldDescription.key[i].empty()) {
      tarch::terminateWithError(
        "Coordinate key " + std::to_string(i) + " of the NetCDF field \"" + fieldDescription.name + "\" expects a name."
      );
    }
    // The default value key z is the conventional name of the third coordinate.
    if (fieldDescription.key[i] == fieldDescription.valueKey) {
      tarch::terminateWithError(
        "The value key \"" + fieldDescription.valueKey + "\" of the NetCDF field \"" + fieldDescription.name
        + "\" must differ from its coordinate keys."
      );
    }
  }

  const int ncid = _netCDFFileReader.open(fieldDescription.filePath);
  if (ncid == -1) {
    tarch::terminateWithError("The NetCDF reader cannot open the file '" + fieldDescription.filePath + "'.");
  }

  Field field;

  for (int i = 0; i < FieldDimensions; i++) {
    field.length[i] = _netCDFFileReader.getDimension(ncid, fieldDescription.key[i].c_str());
    if (field.length[i] == 0) {
      tarch::terminateWithError("The NetCDF reader cannot read the dimension \"" + fieldDescription.key[i] + "\".");
    }

    {
      std::vector<double> coordinates(field.length[i], 0.0);
      if (_netCDFFileReader.readVariable(ncid, fieldDescription.key[i].c_str(), coordinates.data()) == -1) {
        tarch::terminateWithError("The NetCDF reader cannot read the coordinates \"" + fieldDescription.key[i] + "\".");
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
    tarch::terminateWithError("The NetCDF reader cannot read the values of the field \"" + fieldDescription.name + "\".");
  }

  if (_netCDFFileReader.close(ncid) == -1) {
    tarch::terminateWithError("The NetCDF reader cannot close the file '" + fieldDescription.filePath + "'.");
  }

  _fields.emplace(fieldDescription.name, std::move(field));
}

/**
 * Returns the value at the lower grid point along every axis. Returns zero outside the stored
 * extent. getField() stops the run for an unknown field name.
 */
template <int FieldDimensions>
double
tarch::reader::NetCDFFieldSampler<FieldDimensions>::samplePiecewiseConstant(
  const std::string&                               fieldName,
  const tarch::la::Vector<FieldDimensions, double> position
) const {
  const Field& field = getField(fieldName);

  tarch::la::Vector<FieldDimensions, double> positionInCDFRange;
  for (int i = 0; i < FieldDimensions; i++) {
    assertion(std::isfinite(position[i]), fieldName, i, position[i]);
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
 * Interpolates linearly between the surrounding grid points along every axis. The grid points of
 * every axis have to be equidistant. A position outside the stored extent returns zero. getField()
 * stops the run for an unknown field name.
 */
template <int FieldDimensions>
double
tarch::reader::NetCDFFieldSampler<FieldDimensions>::sampleBilinear(
  const std::string&                               fieldName,
  const tarch::la::Vector<FieldDimensions, double> position
) const {
  const Field& field = getField(fieldName);

  tarch::la::Vector<FieldDimensions, std::size_t> lowerIndex(std::size_t(0));
  tarch::la::Vector<FieldDimensions, std::size_t> upperIndex(std::size_t(0));
  tarch::la::Vector<FieldDimensions, double> fraction(0.0);

  for (int i = 0; i < FieldDimensions; i++) {
    assertion(std::isfinite(position[i]), fieldName, i, position[i]);
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

    // The axes are equidistant between their first and last stored coordinate. The continuous
    // index follows from the coordinate alone.
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
  // the stored array. The flat index accumulates from the last dimension downwards.
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
 * Returns whether the stored extent of the field covers position. getField() stops the run for an
 * unknown field name.
 */
template <int FieldDimensions>
bool
tarch::reader::NetCDFFieldSampler<FieldDimensions>::covers(
  const std::string&                               fieldName,
  const tarch::la::Vector<FieldDimensions, double> position
) const {
  const Field& field = getField(fieldName);
  for (int i = 0; i < FieldDimensions; i++) {
    assertion(std::isfinite(position[i]), fieldName, i, position[i]);
    const double coordinate = transformSimulationCoordinateToCDFRange(
      position[i],
      field.lengthOfSimulationDomain[i],
      field.firstElement[i],
      field.lastElement[i],
      field.offsetOfSimulationDomain[i]
    );
    if (not isCoordinateInRange(coordinate, field.firstElement[i], field.lastElement[i])) {
      return false;
    }
  }
  return true;
}

/**
 * Returns the field of fieldName. Stops the run for an unknown field name.
 */
template <int FieldDimensions>
const typename tarch::reader::NetCDFFieldSampler<FieldDimensions>::Field&
tarch::reader::NetCDFFieldSampler<FieldDimensions>::getField(const std::string& fieldName) const {
  const auto fieldIterator = _fields.find(fieldName);
  if (fieldIterator == _fields.end()) {
    tarch::terminateWithError("The NetCDF reader cannot find the field \"" + fieldName + "\".");
  }
  return fieldIterator->second;
}

/**
 * Maps the simulation domain along one axis linearly onto the netCDF coordinate range from
 * firstElement to lastElement and returns the image of coordinate.
 */
template <int FieldDimensions>
double
tarch::reader::NetCDFFieldSampler<FieldDimensions>::transformSimulationCoordinateToCDFRange(
  const double coordinate,
  const double lengthOfSimulationDomain,
  const double firstElement,
  const double lastElement,
  const double offsetOfSimulationDomain
) {
  const double endOfSimulationDomain = lengthOfSimulationDomain + offsetOfSimulationDomain;
  assertion(endOfSimulationDomain != offsetOfSimulationDomain, lengthOfSimulationDomain, offsetOfSimulationDomain);

  const double slope    = ((lastElement - firstElement) / (endOfSimulationDomain - offsetOfSimulationDomain));
  const double intercept = firstElement - slope * offsetOfSimulationDomain;

  return slope * coordinate + intercept;
}

/**
 * Returns the flat array index of the grid point at or below a netCDF coordinate tuple. The first
 * coordinate varies fastest in the flat array.
 */
template <int FieldDimensions>
std::size_t
tarch::reader::NetCDFFieldSampler<FieldDimensions>::transformIndexCDFRangeToArray(
  const tarch::la::Vector<FieldDimensions, double>      coordinatesInCDFMesh,
  const tarch::la::Vector<FieldDimensions, double>      firstCoordinatesInCDFMesh,
  const tarch::la::Vector<FieldDimensions, double>      lastCoordinatesInCDFMesh,
  const tarch::la::Vector<FieldDimensions, std::size_t> lengthOfDimensionsInCDFMesh
) {
  const auto computeIndex = [](
                              const double      coordinate,
                              const double      firstCoordinate,
                              const double      lastCoordinate,
                              const std::size_t length
                            ) -> std::size_t {
    if (length <= 1 or tarch::la::equals(firstCoordinate, lastCoordinate)) {
      return 0;
    }

    const double index = ((coordinate - firstCoordinate) / (lastCoordinate - firstCoordinate)) * (length - 1);
    return static_cast<std::size_t>(
      std::clamp(static_cast<long long>(std::floor(index)), 0LL, static_cast<long long>(length - 1))
    );
  };

  // A 3D field can exceed 2^31 entries. The flat index is a std::size_t.
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
  const double coordinate,
  const double firstCoordinate,
  const double lastCoordinate
) {
  const auto [lower, upper] = std::minmax(firstCoordinate, lastCoordinate);
  return not tarch::la::smaller(coordinate, lower) and not tarch::la::greater(coordinate, upper);
}

template class tarch::reader::NetCDFFieldSampler<2>;
template class tarch::reader::NetCDFFieldSampler<3>;

#endif
