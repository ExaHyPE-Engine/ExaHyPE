// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#ifdef PEANO_WITH_HDF5

namespace tarch::reader {
  class HDF5FileReader;
} // namespace tarch::reader

/**
 * Reads datasets of one HDF5 file through the serial API. Every rank opens
 * its own reader, so no call below is collective.
 *
 * The file handle is an hid_t stored as std::int64_t, so this header needs no
 * HDF5 include; the implementation asserts that the two types match.
 */
class tarch::reader::HDF5FileReader final {
public:
  explicit HDF5FileReader(const std::string& filename);
  HDF5FileReader(const HDF5FileReader&)            = delete;
  HDF5FileReader(HDF5FileReader&&)                 = delete;
  HDF5FileReader& operator=(const HDF5FileReader&) = delete;
  HDF5FileReader& operator=(HDF5FileReader&&)      = delete;
  ~HDF5FileReader();

  [[nodiscard]] bool datasetExists(const std::string& path) const;

  /** Returns {rows, columns} for a two-dimensional dataset and {rows, 1} for a one-dimensional one. */
  [[nodiscard]] std::pair<std::uint64_t, std::uint64_t> getDatasetDimensions(const std::string& path) const;

  /** Reads rows [firstRow, firstRow + rows) with every column, row-major. */
  [[nodiscard]] std::vector<double> readDoubleRows(
    const std::string& path,
    std::uint64_t      firstRow,
    std::uint64_t      rows
  ) const;
  [[nodiscard]] std::vector<std::int32_t> readInt32Rows(
    const std::string& path,
    std::uint64_t      firstRow,
    std::uint64_t      rows
  ) const;
  [[nodiscard]] std::vector<std::int64_t> readInt64Rows(
    const std::string& path,
    std::uint64_t      firstRow,
    std::uint64_t      rows
  ) const;

  [[nodiscard]] const std::string& getFilename() const;

private:
  void readRows(
    const std::string& path,
    std::uint64_t      firstRow,
    std::uint64_t      rows,
    void*              target,
    std::int64_t       memoryType
  ) const;

  std::string  _filename;
  std::int64_t _file;
};

#endif
