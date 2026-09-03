// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HDF5FileReader.h"

#ifdef PEANO_WITH_HDF5

#include "tarch/Termination.h"

#include <hdf5.h>

// The header stores the file handle as std::int64_t to keep <hdf5.h> out of it.
static_assert(std::is_same_v<hid_t, std::int64_t>);

tarch::reader::HDF5FileReader::HDF5FileReader(const std::string& filename) :
  _filename(filename),
  _file(H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT)) {
  if (_file < 0) {
    tarch::terminateWithError("Failed to open the HDF5 file '" + filename + "' for reading.");
  }
}

tarch::reader::HDF5FileReader::~HDF5FileReader() { H5Fclose(_file); }

bool tarch::reader::HDF5FileReader::datasetExists(const std::string& path) const {
  // Every group along the path has to exist before the dataset itself may be probed,
  // as H5Lexists on a path with a missing intermediate group reports an error.
  std::size_t position = path.find('/', 1);
  while (position != std::string::npos) {
    if (H5Lexists(_file, path.substr(0, position).c_str(), H5P_DEFAULT) <= 0) {
      return false;
    }
    position = path.find('/', position + 1);
  }
  return H5Lexists(_file, path.c_str(), H5P_DEFAULT) > 0;
}

std::pair<std::uint64_t, std::uint64_t> tarch::reader::HDF5FileReader::getDatasetDimensions(const std::string& path
) const {
  const hid_t dataset = H5Dopen(_file, path.c_str(), H5P_DEFAULT);
  if (dataset < 0) {
    tarch::terminateWithError("Failed to open the HDF5 dataset '" + path + "' in '" + _filename + "'.");
  }
  const hid_t filespace          = H5Dget_space(dataset);
  const int   numberOfDimensions = H5Sget_simple_extent_ndims(filespace);
  hsize_t     dims[2]            = {0, 0};
  if (numberOfDimensions < 1 or numberOfDimensions > 2) {
    H5Sclose(filespace);
    H5Dclose(dataset);
    tarch::terminateWithError("The HDF5 dataset '" + path + "' has an unsupported number of dimensions.");
  }
  H5Sget_simple_extent_dims(filespace, dims, nullptr);
  H5Sclose(filespace);
  H5Dclose(dataset);
  return {dims[0], numberOfDimensions == 2 ? dims[1] : 1};
}

void tarch::reader::HDF5FileReader::readRows(
  const std::string&  path,
  const std::uint64_t firstRow,
  const std::uint64_t rows,
  void*               target,
  const std::int64_t  memoryType
) const {
  if (rows == 0) {
    return;
  }
  const hid_t dataset = H5Dopen(_file, path.c_str(), H5P_DEFAULT);
  if (dataset < 0) {
    tarch::terminateWithError("Failed to open the HDF5 dataset '" + path + "' in '" + _filename + "'.");
  }
  const hid_t filespace          = H5Dget_space(dataset);
  const int   numberOfDimensions = H5Sget_simple_extent_ndims(filespace);
  hsize_t     dims[2]            = {0, 0};
  H5Sget_simple_extent_dims(filespace, dims, nullptr);

  const hsize_t offsets[2]   = {firstRow, 0};
  const hsize_t localDims[2] = {rows, dims[1]};
  const herr_t  selectStatus = H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offsets, nullptr, localDims, nullptr);
  const hid_t   memspace     = H5Screate_simple(numberOfDimensions, localDims, nullptr);
  const herr_t  readStatus   = selectStatus >= 0
      ? H5Dread(dataset, memoryType, memspace, filespace, H5P_DEFAULT, target)
      : -1;
  H5Sclose(memspace);
  H5Sclose(filespace);
  H5Dclose(dataset);
  if (selectStatus < 0 or readStatus < 0) {
    tarch::terminateWithError("Failed to read rows of the HDF5 dataset '" + path + "' in '" + _filename + "'.");
  }
}

std::vector<double> tarch::reader::HDF5FileReader::readDoubleRows(
  const std::string& path, const std::uint64_t firstRow, const std::uint64_t rows
) const {
  std::vector<double> result(rows * getDatasetDimensions(path).second, 0.0);
  readRows(path, firstRow, rows, result.data(), H5T_NATIVE_DOUBLE);
  return result;
}

std::vector<std::int32_t> tarch::reader::HDF5FileReader::readInt32Rows(
  const std::string& path, const std::uint64_t firstRow, const std::uint64_t rows
) const {
  std::vector<std::int32_t> result(rows * getDatasetDimensions(path).second, 0);
  readRows(path, firstRow, rows, result.data(), H5T_NATIVE_INT);
  return result;
}

std::vector<std::int64_t> tarch::reader::HDF5FileReader::readInt64Rows(
  const std::string& path, const std::uint64_t firstRow, const std::uint64_t rows
) const {
  std::vector<std::int64_t> result(rows * getDatasetDimensions(path).second, 0);
  readRows(path, firstRow, rows, result.data(), H5T_NATIVE_LLONG);
  return result;
}

const std::string& tarch::reader::HDF5FileReader::getFilename() const { return _filename; }

#endif
