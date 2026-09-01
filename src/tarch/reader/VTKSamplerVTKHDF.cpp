// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
//
// The VTKHDF back-end of tarch::reader::VTKSampler: one growing HDF5 file whose
// /VTKHDF group holds every step of a run.
#include "VTKSampler.h"

#ifdef PEANO_WITH_HDF5

#include <hdf5.h>

namespace {
  const std::string RootGroup = "/VTKHDF";

  [[noreturn]] void fail(const std::string& filePath, const std::string& message) {
    throw std::runtime_error("Could not read VTKHDF file \"" + filePath + "\": " + message + ".");
  }

  /**
   * Closes an HDF5 identifier when it goes out of scope, so a malformed file
   * can throw from anywhere in the read without leaking handles.
   */
  class Handle {
  private:
    hid_t _id;
    herr_t (*_close)(hid_t);

  public:
    Handle(hid_t id, herr_t (*close)(hid_t)):
      _id(id),
      _close(close) {}

    ~Handle() {
      if (_id >= 0) {
        _close(_id);
      }
    }

    Handle(const Handle&)            = delete;
    Handle& operator=(const Handle&) = delete;

               operator hid_t() const { return _id; }
    bool       isValid() const { return _id >= 0; }
  };

  /**
   * H5Lexists only accepts a path whose ancestors exist, so an optional group
   * or dataset deep in the hierarchy has to be probed component by component.
   */
  bool linkExists(hid_t file, const std::string& path) {
    std::string prefix;
    std::size_t begin = 0;

    while (begin <= path.size()) {
      const std::size_t end       = path.find('/', begin);
      const std::string component = path.substr(begin, end == std::string::npos ? std::string::npos : end - begin);

      if (not component.empty()) {
        prefix += "/" + component;
        if (H5Lexists(file, prefix.c_str(), H5P_DEFAULT) <= 0) {
          return false;
        }
      }

      if (end == std::string::npos) {
        break;
      }
      begin = end + 1;
    }

    return true;
  }

  /**
   * @brief Reads a range of rows of a one- or two-dimensional dataset.
   *
   * HDF5 converts into the requested native type, so the caller does not have
   * to care whether the plotter has dumped single or double precision.
   *
   * @param numberOfRows        0 reads all rows behind firstRow.
   * @param numberOfComponents  set to the dataset's second extent (1 for a
   *                            one-dimensional dataset) if not nullptr.
   */
  template <class T>
  std::vector<T> readRows(
    hid_t              file,
    const std::string& path,
    hid_t              nativeType,
    hsize_t            firstRow,
    hsize_t            numberOfRows,
    const std::string& filePath,
    int*               numberOfComponents = nullptr
  ) {
    Handle dataset(H5Dopen(file, path.c_str(), H5P_DEFAULT), H5Dclose);
    if (not dataset.isValid()) {
      fail(filePath, "dataset " + path + " is missing");
    }

    Handle    dataSpace(H5Dget_space(dataset), H5Sclose);
    const int rank    = H5Sget_simple_extent_ndims(dataSpace);
    hsize_t   dims[2] = {0, 1};
    if (rank < 1 or rank > 2) {
      fail(filePath, "dataset " + path + " is neither one- nor two-dimensional");
    }
    H5Sget_simple_extent_dims(dataSpace, dims, nullptr);

    const hsize_t components = (rank == 2) ? dims[1] : 1;
    if (numberOfComponents != nullptr) {
      *numberOfComponents = static_cast<int>(components);
    }

    if (numberOfRows == 0) {
      numberOfRows = (firstRow < dims[0]) ? dims[0] - firstRow : 0;
    }
    if (firstRow + numberOfRows > dims[0]) {
      fail(
        filePath,
        "dataset " + path + " holds " + std::to_string(dims[0]) + " rows, but rows [" + std::to_string(firstRow) + ","
          + std::to_string(firstRow + numberOfRows) + ") were requested"
      );
    }

    std::vector<T> result(numberOfRows * components);
    if (numberOfRows == 0) {
      return result;
    }

    const hsize_t start[2] = {firstRow, 0};
    const hsize_t count[2] = {numberOfRows, components};
    H5Sselect_hyperslab(dataSpace, H5S_SELECT_SET, start, nullptr, count, nullptr);

    Handle memorySpace(H5Screate_simple(rank, count, nullptr), H5Sclose);
    if (H5Dread(dataset, nativeType, memorySpace, dataSpace, H5P_DEFAULT, result.data()) < 0) {
      fail(filePath, "dataset " + path + " could not be read");
    }

    return result;
  }

  long long readEntry(hid_t file, const std::string& path, hsize_t row, const std::string& filePath) {
    return readRows<long long>(file, path, H5T_NATIVE_LLONG, row, 1, filePath).at(0);
  }
} // namespace

void tarch::reader::VTKSampler::readVTKHDF(const std::vector<std::string>& fieldNames, int step) {
  Handle file(H5Fopen(_filePath.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT), H5Fclose);
  if (not file.isValid()) {
    fail(_filePath, "file does not exist or is no HDF5 file");
  }
  if (not linkExists(file, RootGroup)) {
    fail(_filePath, "no " + RootGroup + " group, so this is no VTKHDF file");
  }

  // ------------------------------------------------------------------------
  // Pick the step
  // ------------------------------------------------------------------------
  const std::string stepsGroup   = RootGroup + "/Steps";
  const bool        isTimeSeries = linkExists(file, stepsGroup);
  _numberOfSteps                 = 1;

  if (isTimeSeries) {
    Handle steps(H5Gopen(file, stepsGroup.c_str(), H5P_DEFAULT), H5Gclose);
    Handle attribute(H5Aopen(steps, "NSteps", H5P_DEFAULT), H5Aclose);
    if (not attribute.isValid()) {
      fail(_filePath, "the Steps group has no NSteps attribute");
    }
    long long numberOfSteps = 0;
    H5Aread(attribute, H5T_NATIVE_LLONG, &numberOfSteps);
    _numberOfSteps = static_cast<int>(numberOfSteps);
  }

  if (_numberOfSteps <= 0) {
    fail(_filePath, "the file holds no time step");
  }

  _step = (step < 0) ? _numberOfSteps + step : step;
  if (_step < 0 or _step >= _numberOfSteps) {
    fail(
      _filePath,
      "step " + std::to_string(step) + " does not exist, the file holds " + std::to_string(_numberOfSteps) + " steps"
    );
  }

  if (isTimeSeries and linkExists(file, stepsGroup + "/Values")) {
    _timeStamp = readRows<double>(file, stepsGroup + "/Values", H5T_NATIVE_DOUBLE, _step, 1, _filePath).at(0);
  }

  // ------------------------------------------------------------------------
  // Geometry range of that step
  //
  // Every step is written as a sequence of parts; Peano's writer merges all
  // ranks into a single part, but the layout allows for more. Points and cells
  // of the parts of one step are contiguous, so summing the per-part counts
  // yields the step's range.
  // ------------------------------------------------------------------------
  const auto numberOfPointsPerPart = readRows<long long>(
    file, RootGroup + "/NumberOfPoints", H5T_NATIVE_LLONG, 0, 0, _filePath
  );
  const auto numberOfCellsPerPart = readRows<long long>(
    file, RootGroup + "/NumberOfCells", H5T_NATIVE_LLONG, 0, 0, _filePath
  );

  hsize_t firstPart     = 0;
  hsize_t numberOfParts = numberOfPointsPerPart.size();

  if (isTimeSeries) {
    if (linkExists(file, stepsGroup + "/PartOffsets")) {
      firstPart = static_cast<hsize_t>(readEntry(file, stepsGroup + "/PartOffsets", _step, _filePath));
    }
    if (linkExists(file, stepsGroup + "/NumberOfParts")) {
      numberOfParts = static_cast<hsize_t>(readEntry(file, stepsGroup + "/NumberOfParts", _step, _filePath));
    } else if (linkExists(file, stepsGroup + "/PartOffsets") and _step + 1 < _numberOfSteps) {
      numberOfParts = static_cast<hsize_t>(readEntry(file, stepsGroup + "/PartOffsets", _step + 1, _filePath))
                      - firstPart;
    } else {
      numberOfParts = numberOfPointsPerPart.size() - firstPart;
    }
  }

  if (firstPart + numberOfParts > numberOfPointsPerPart.size()) {
    fail(_filePath, "the part range of step " + std::to_string(_step) + " exceeds the file's parts");
  }

  hsize_t numberOfPoints = 0;
  hsize_t numberOfCells  = 0;
  for (hsize_t part = firstPart; part < firstPart + numberOfParts; part++) {
    numberOfPoints += static_cast<hsize_t>(numberOfPointsPerPart.at(part));
    numberOfCells += static_cast<hsize_t>(numberOfCellsPerPart.at(part));
  }

  if (numberOfPoints == 0) {
    fail(_filePath, "step " + std::to_string(_step) + " holds no points");
  }

  hsize_t pointOffset = 0;
  hsize_t cellOffset  = 0;
  if (isTimeSeries) {
    pointOffset = static_cast<hsize_t>(readEntry(file, stepsGroup + "/PointOffsets", _step, _filePath));
    if (linkExists(file, stepsGroup + "/CellOffsets")) {
      cellOffset = static_cast<hsize_t>(readEntry(file, stepsGroup + "/CellOffsets", _step, _filePath));
    }
  }

  // ------------------------------------------------------------------------
  // Locate the requested fields. They all have to share one association, as a
  // point index has to identify the same sample point for each of them.
  // ------------------------------------------------------------------------
  if (fieldNames.empty()) {
    fail(_filePath, "no field name was requested");
  }

  bool isFirstField = true;
  for (const std::string& fieldName : fieldNames) {
    const bool isPointData = linkExists(file, RootGroup + "/PointData/" + fieldName);
    const bool isCellData  = linkExists(file, RootGroup + "/CellData/" + fieldName);

    if (not isPointData and not isCellData) {
      fail(_filePath, "neither PointData nor CellData holds a field \"" + fieldName + "\"");
    }

    const bool isSampledAsCellData = not isPointData;
    if (isFirstField) {
      _samplesCellData = isSampledAsCellData;
      isFirstField     = false;
    } else if (isSampledAsCellData != _samplesCellData) {
      fail(
        _filePath,
        "field \"" + fieldName + "\" is " + (isSampledAsCellData ? "cell" : "point")
          + " data, while the preceding fields are " + (_samplesCellData ? "cell" : "point")
          + " data. One sampler can only serve one of the two"
      );
    }
  }

  // ------------------------------------------------------------------------
  // Sample point positions: the plotted vertices, or the cells' centroids
  // ------------------------------------------------------------------------
  int        pointComponents = 0;
  const auto points          = readRows<double>(
    file, RootGroup + "/Points", H5T_NATIVE_DOUBLE, pointOffset, numberOfPoints, _filePath, &pointComponents
  );
  if (pointComponents != 3) {
    fail(_filePath, "the Points dataset does not hold three coordinates per point");
  }

  if (not _samplesCellData) {
    _numberOfPoints = numberOfPoints;
    _positions      = points;
  } else {
    // A cell's sample point is its centroid, i.e. the average of its corners.
    // Peano writes one part per step, so the connectivity, which is part-local,
    // indexes into the step's points directly.
    if (numberOfParts != 1) {
      fail(_filePath, "cell data can only be sampled from a file with one part per step");
    }

    const hsize_t connectivityOffset = isTimeSeries ? static_cast<hsize_t>(
                                         readEntry(file, stepsGroup + "/ConnectivityIdOffsets", _step, _filePath)
                                       )
                                                    : 0;
    const hsize_t numberOfConnectivityIds = static_cast<hsize_t>(
      readRows<long long>(file, RootGroup + "/NumberOfConnectivityIds", H5T_NATIVE_LLONG, firstPart, 1, _filePath).at(0)
    );

    // The Offsets array carries a leading zero per part, hence the extra shift
    // by the part index.
    const auto offsets = readRows<long long>(
      file, RootGroup + "/Offsets", H5T_NATIVE_LLONG, cellOffset + firstPart, numberOfCells + 1, _filePath
    );
    const auto connectivity = readRows<long long>(
      file, RootGroup + "/Connectivity", H5T_NATIVE_LLONG, connectivityOffset, numberOfConnectivityIds, _filePath
    );

    _numberOfPoints = numberOfCells;
    _positions.resize(3 * numberOfCells, 0.0);

    for (hsize_t cell = 0; cell < numberOfCells; cell++) {
      const long long first = offsets.at(cell);
      const long long last  = offsets.at(cell + 1);
      if (last <= first) {
        fail(_filePath, "cell " + std::to_string(cell) + " has no corners");
      }

      for (long long entry = first; entry < last; entry++) {
        const long long point = connectivity.at(entry);
        for (int d = 0; d < 3; d++) {
          _positions[3 * cell + d] += points.at(3 * point + d);
        }
      }
      for (int d = 0; d < 3; d++) {
        _positions[3 * cell + d] /= static_cast<double>(last - first);
      }
    }
  }

  if (_numberOfPoints == 0) {
    fail(_filePath, "step " + std::to_string(_step) + " holds no data");
  }

  // ------------------------------------------------------------------------
  // The data arrays themselves
  // ------------------------------------------------------------------------
  const std::string dataGroup = _samplesCellData ? RootGroup + "/CellData/" : RootGroup + "/PointData/";
  const std::string dataOffsetGroup = _samplesCellData ? stepsGroup + "/CellDataOffsets/"
                                                       : stepsGroup + "/PointDataOffsets/";
  const hsize_t     defaultOffset   = _samplesCellData ? cellOffset : pointOffset;

  for (const std::string& fieldName : fieldNames) {
    hsize_t offset = defaultOffset;
    if (isTimeSeries and linkExists(file, dataOffsetGroup + fieldName)) {
      offset = static_cast<hsize_t>(readEntry(file, dataOffsetGroup + fieldName, _step, _filePath));
    }

    Field field;
    field.values = readRows<double>(
      file, dataGroup + fieldName, H5T_NATIVE_DOUBLE, offset, _numberOfPoints, _filePath, &field.numberOfComponents
    );

    _fields.emplace(fieldName, std::move(field));
  }
}

#else

void tarch::reader::VTKSampler::readVTKHDF(
  [[maybe_unused]] const std::vector<std::string>& fieldNames, [[maybe_unused]] int step
) {
  throw std::runtime_error(
    "Could not read \"" + _filePath
    + "\": Peano has been built without HDF5 support (PEANO_WITH_HDF5), so it cannot read a VTKHDF file. Either "
      "reconfigure with -DPEANO_WITH_HDF5=ON or restart from the VTU output (.pvd) of a run."
  );
}

#endif
