// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/plotter/Format.h"
#include "tarch/plotter/Mesh.h"

#include <mpi.h>

namespace tarch::plotter {
  class FileWriter;

  [[nodiscard]] std::string getExactTimeStamp(double time);
} // namespace tarch::plotter

/**
 * Writes the snapshots of one plot series in one format.
 */
class tarch::plotter::FileWriter {
public:
  /**
   * Counts the entries of one range of forEachRangeInParallel(). The range with index r starts at
   * the entry r * EntriesPerRange.
   */
  static constexpr std::size_t EntriesPerRange{4096};

  FileWriter()                                   = default;
  FileWriter(const FileWriter& other)            = delete;
  FileWriter(FileWriter&& other)                 = delete;
  FileWriter& operator=(const FileWriter& other) = delete;
  FileWriter& operator=(FileWriter&& other)      = delete;
  virtual ~FileWriter()                          = default;

  virtual void write(
    const Mesh&        mesh,
    const std::string& outputDirectory,
    const std::string& filename,
    double             time
  ) = 0;

  [[nodiscard]] virtual Format getFormat() const = 0;

  static void setDoublePrecisionOutput(bool value);
  static bool getDoublePrecisionOutput();

protected:
  enum class FieldKind { Scalar, Vector, Tensor, Generic };

  struct FieldLayout final {
    FieldKind kind{FieldKind::Scalar};

    /**
     * A writer pads the components beyond the source components with zero.
     */
    int writtenComponents{1};
  };

  static void validateMesh(const Mesh& mesh);
  static void createOutputDirectory(const std::string& outputDirectory);

  [[nodiscard]] static std::string getSnapshotName(const std::string& filename, double time);
  [[nodiscard]] static MPI_Info createMPIFileHints();
  [[nodiscard]] static FieldLayout classifyField(int sourceComponents);

  static void forEachRangeInParallel(
    std::size_t                                                              numberOfEntries,
    const std::function<void(std::size_t firstEntry, std::size_t endEntry)>& body
  );

  static void copyTuples(
    const double* source,
    int           sourceStride,
    int           copiedComponents,
    int           targetStride,
    std::size_t   numberOfTuples,
    double*       target
  );
  static void copyTuples(
    const double* source,
    int           sourceStride,
    int           copiedComponents,
    int           targetStride,
    std::size_t   numberOfTuples,
    float*        target
  );

private:
  static bool _doublePrecisionOutput;
};
