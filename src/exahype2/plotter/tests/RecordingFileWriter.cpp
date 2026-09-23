// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RecordingFileWriter.h"

#include "tarch/Assertions.h"
#include "tarch/plotter/PVDTimeSeriesWriter.h"
#include "tarch/tests/TestCase.h"

namespace {
  template <typename Field>
  std::vector<double> valuesOfField(const std::vector<Field>& fields, const std::string& name) {
    for (const auto& field : fields) {
      if (field.name == name) {
        return field.values;
      }
    }
    return {};
  }

  template <typename Field>
  std::vector<std::string> namesOfFields(const std::vector<Field>& fields) {
    std::vector<std::string> names;
    names.reserve(fields.size());
    for (const auto& field : fields) {
      names.push_back(field.name);
    }
    return names;
  }
} // namespace

exahype2::plotter::tests::RecordingFileWriter::RecordingFileWriter(const std::string& outputDirectory) :
  tarch::plotter::FileWriter(outputDirectory) {}

/**
 * Opens a snapshot whose file writer keeps its pieces in memory, so a test writes no file.
 */
void exahype2::plotter::tests::openRecordingSnapshot(Snapshot& snapshot) {
  snapshot.openFile<RecordingFileWriter, tarch::plotter::PVDTimeSeriesWriter>(
    tarch::tests::getWritableTestDirectory().string(),
    0.0
  );
}

/**
 * Returns the file writer of a snapshot that openRecordingSnapshot() opened.
 */
exahype2::plotter::tests::RecordingFileWriter& exahype2::plotter::tests::getRecordingFileWriter(Snapshot& snapshot) {
  assertion(dynamic_cast<RecordingFileWriter*>(&snapshot.getFileWriter()) != nullptr);
  return static_cast<RecordingFileWriter&>(snapshot.getFileWriter());
}

/**
 * Returns the digit of index at the position of axis, where index counts along the axes with
 * base steps per axis and the first axis runs fastest.
 */
int exahype2::plotter::tests::digitAlongAxis(const int index, const int axis, const int base) {
  assertion(base > 0, base);
  int digit = index;
  for (int step = 0; step < axis; step++) {
    digit /= base;
  }
  return digit % base;
}

/**
 * Closes the writer without touching the file system, and keeps the buffers for the test.
 */
bool exahype2::plotter::tests::RecordingFileWriter::writeToFile(
  [[maybe_unused]] const std::string& filename,
  [[maybe_unused]] const double       time
) {
  _written = true;
  return true;
}

std::string exahype2::plotter::tests::RecordingFileWriter::getFileExtension() const { return ".recording"; }

int exahype2::plotter::tests::RecordingFileWriter::getNumberOfPoints(const int pieceId) const {
  return static_cast<int>(_subdomains.at(static_cast<std::size_t>(pieceId)).pointsXYZ.size() / 3);
}

int exahype2::plotter::tests::RecordingFileWriter::getNumberOfCells(const int pieceId) const {
  return static_cast<int>(_subdomains.at(static_cast<std::size_t>(pieceId)).types.size());
}

double exahype2::plotter::tests::RecordingFileWriter::getPointCoordinate(
  const int pieceId,
  const int pointIndex,
  const int axis
) const {
  const auto& points = _subdomains.at(static_cast<std::size_t>(pieceId)).pointsXYZ;
  return points.at(static_cast<std::size_t>(pointIndex * 3 + axis));
}

std::vector<std::int32_t> exahype2::plotter::tests::RecordingFileWriter::getConnectivity(const int pieceId) const {
  return _subdomains.at(static_cast<std::size_t>(pieceId)).connectivity;
}

std::vector<std::string> exahype2::plotter::tests::RecordingFileWriter::getPointFieldNames(const int pieceId) const {
  return namesOfFields(_subdomains.at(static_cast<std::size_t>(pieceId)).pointFields);
}

std::vector<std::string> exahype2::plotter::tests::RecordingFileWriter::getCellFieldNames(const int pieceId) const {
  return namesOfFields(_subdomains.at(static_cast<std::size_t>(pieceId)).cellFields);
}

std::vector<double> exahype2::plotter::tests::RecordingFileWriter::getPointField(
  const int          pieceId,
  const std::string& name
) const {
  return valuesOfField(_subdomains.at(static_cast<std::size_t>(pieceId)).pointFields, name);
}

std::vector<double> exahype2::plotter::tests::RecordingFileWriter::getCellField(
  const int          pieceId,
  const std::string& name
) const {
  return valuesOfField(_subdomains.at(static_cast<std::size_t>(pieceId)).cellFields, name);
}
