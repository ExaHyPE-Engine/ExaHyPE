// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/plotter/Snapshot.h"
#include "tarch/plotter/FileWriter.h"

namespace exahype2::plotter::tests {
  class RecordingFileWriter;

  void openRecordingSnapshot(Snapshot& snapshot);

  RecordingFileWriter& getRecordingFileWriter(Snapshot& snapshot);

  int digitAlongAxis(int index, int axis, int base);
} // namespace exahype2::plotter::tests

/**
 * Keeps every piece in memory, so a test reads back what a plotter wrote without parsing a file.
 */
class exahype2::plotter::tests::RecordingFileWriter final : public tarch::plotter::FileWriter {
public:
  explicit RecordingFileWriter(const std::string& outputDirectory);
  RecordingFileWriter(const RecordingFileWriter&)            = delete;
  RecordingFileWriter(RecordingFileWriter&&)                 = delete;
  RecordingFileWriter& operator=(const RecordingFileWriter&) = delete;
  RecordingFileWriter& operator=(RecordingFileWriter&&)      = delete;
  ~RecordingFileWriter() override                            = default;

  bool                      writeToFile(const std::string& filename, double time) override;
  [[nodiscard]] std::string getFileExtension() const override;

  [[nodiscard]] int getNumberOfPoints(int pieceId) const;
  [[nodiscard]] int getNumberOfCells(int pieceId) const;

  [[nodiscard]] double getPointCoordinate(int pieceId, int pointIndex, int axis) const;

  [[nodiscard]] std::vector<std::int32_t> getConnectivity(int pieceId) const;

  [[nodiscard]] std::vector<std::string> getPointFieldNames(int pieceId) const;
  [[nodiscard]] std::vector<std::string> getCellFieldNames(int pieceId) const;

  [[nodiscard]] std::vector<double> getPointField(int pieceId, const std::string& name) const;
  [[nodiscard]] std::vector<double> getCellField(int pieceId, const std::string& name) const;
};
