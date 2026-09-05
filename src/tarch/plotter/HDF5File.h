// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#ifdef PEANO_WITH_HDF5

namespace tarch::plotter {
  class HDF5File;
} // namespace tarch::plotter

/**
 * One HDF5 file created for a collective parallel write.
 *
 * openForDataHoldingRanks() runs on every rank and returns the file on the
 * ranks that write it. Dataset construction, writeRows() and writeTable() are
 * collective over those ranks: Each of them makes the same calls in the same
 * order with the same arguments, and only the row range of writeRows()
 * differs per rank.
 *
 * The handle members store hid_t values as std::int64_t, so this header needs
 * no HDF5 include; the implementation asserts that the two types match.
 */
class tarch::plotter::HDF5File final {
public:
  enum class ElementType { Float32, Float64, Int32, Int64 };

  /**
   * One chunked, shuffle-and-deflate-compressed dataset of the file. A global
   * row count of zero creates no dataset, and writeRows() on such a dataset
   * returns without an HDF5 call on every rank.
   */
  class Dataset final {
  public:
    /** Zero columns create a one-dimensional dataset, a positive count a two-dimensional one. */
    Dataset(HDF5File& file, const std::string& name, unsigned long long rows, int columns, ElementType storageType);

    Dataset()                          = delete;
    Dataset(const Dataset&)            = delete;
    Dataset(Dataset&&)                 = delete;
    Dataset& operator=(const Dataset&) = delete;
    Dataset& operator=(Dataset&&)      = delete;
    ~Dataset();

    /**
     * Writes rows [firstRow, firstRow + rows) from data, which holds the rows
     * contiguously in memoryType elements. A rank with zero rows still calls
     * this, so the collective transfer stays balanced.
     */
    void writeRows(unsigned long long firstRow, unsigned long long rows, const void* data, ElementType memoryType);

    /**
     * Writes numberOfRuns disjoint row runs, run i covering rows
     * [firstRows[i], firstRows[i] + rowCounts[i]), from data, which holds all
     * runs contiguously in run order. The runs join one collective transfer,
     * so ranks with different run counts stay balanced.
     */
    void writeRuns(
      const unsigned long long* firstRows,
      const unsigned long long* rowCounts,
      int                       numberOfRuns,
      const void*               data,
      ElementType               memoryType
    );

  private:
    std::int64_t _dataset;
    std::int64_t _transferProperties;
    int          _columns;
  };

  HDF5File()                           = delete;
  HDF5File(const HDF5File&)            = delete;
  HDF5File(HDF5File&&)                 = delete;
  HDF5File& operator=(const HDF5File&) = delete;
  HDF5File& operator=(HDF5File&&)      = delete;
  ~HDF5File();

  /**
   * Creates path on the ranks that hold data and returns nullptr on every
   * other rank; a caller receiving nullptr takes no further part in the
   * write. When no rank holds data, the global master alone creates the file.
   * The exclusion exists because a rank that joins a collective write to a
   * deflate-filtered dataset with an empty selection deadlocks the
   * filtered-chunk redistribution on some MPI-IO stacks.
   *
   * Collective over every rank of the global communicator.
   */
  static std::unique_ptr<HDF5File> openForDataHoldingRanks(const std::string& path, bool holdsData);

  /** True on exactly one rank of the file, which writes the tables and any metadata file. */
  [[nodiscard]] bool writesTables() const;

  /**
   * Turns count per-rank row counts into each rank's first global row and the
   * global totals, through one exclusive prefix sum and one reduction over
   * the file's ranks.
   */
  void deriveRowRanges(
    const unsigned long long* localRows,
    unsigned long long*       firstRows,
    unsigned long long*       totalRows,
    int                       count
  ) const;

  /**
   * Creates a small contiguous, uncompressed dataset and fills it from the
   * rank writesTables() selects. Every other rank joins the collective write
   * with an empty selection, so data may be nullptr there.
   */
  void writeTable(const std::string& name, unsigned long long rows, int columns, const double* data);
  void writeTable(const std::string& name, unsigned long long rows, int columns, const std::int32_t* data);
  void writeTable(const std::string& name, unsigned long long rows, int columns, const std::int64_t* data);

private:
  HDF5File(std::int64_t file, std::int64_t linkCreationProperties, std::int64_t transferProperties, bool writesTables);

  void writeTable(
    const std::string& name,
    unsigned long long rows,
    int                columns,
    const void*        data,
    ElementType        storageType
  );

  std::int64_t _file;
  std::int64_t _linkCreationProperties;
  std::int64_t _transferProperties;
  bool         _writesTables;
};

#endif
