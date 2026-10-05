// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FileWriter.h"

#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Handles.h"
#include "tarch/mpi/Rank.h"

namespace {
  template <typename Body>
  void forEachRange(const std::size_t numberOfEntries, const Body& body) {
    constexpr std::size_t EntriesPerRange = tarch::plotter::FileWriter::EntriesPerRange;
    const std::size_t     numberOfRanges  = (numberOfEntries + EntriesPerRange - 1) / EntriesPerRange;
    assertion(numberOfRanges <= static_cast<std::size_t>(std::numeric_limits<int>::max()), numberOfRanges);
    tarch::accelerator::parallelForOnHost(
      tarch::accelerator::Range(static_cast<int>(numberOfRanges)),
      [&](const int range) {
        const std::size_t firstEntry = static_cast<std::size_t>(range) * EntriesPerRange;
        body(firstEntry, std::min(firstEntry + EntriesPerRange, numberOfEntries));
      }
    );
  }

  /**
   * Returns value as a float without raising a floating-point exception.
   */
  float narrow(const double value) {
    if (std::abs(value) < static_cast<double>(std::numeric_limits<float>::min())) {
      return std::signbit(value) ? -0.0F : 0.0F;
    }
    if (std::abs(value) > static_cast<double>(std::numeric_limits<float>::max())) {
      return std::signbit(value) ? -std::numeric_limits<float>::max() : std::numeric_limits<float>::max();
    }
    return static_cast<float>(value);
  }

  template <typename Target>
  void copyTuplesInParallel(
    const double* const source,
    const int           sourceStride,
    const int           copiedComponents,
    const int           targetStride,
    const std::size_t   numberOfTuples,
    Target* const       target
  ) {
    assertion(copiedComponents > 0 and copiedComponents <= sourceStride, copiedComponents, sourceStride);
    assertion(copiedComponents <= targetStride, copiedComponents, targetStride);
    assertion(numberOfTuples == 0 or (source != nullptr and target != nullptr), numberOfTuples);
    forEachRange(numberOfTuples, [=](const std::size_t firstTuple, const std::size_t endTuple) {
      for (std::size_t tuple = firstTuple; tuple < endTuple; tuple++) {
        const double* const tupleSource = source + tuple * static_cast<std::size_t>(sourceStride);
        Target* const       tupleTarget = target + tuple * static_cast<std::size_t>(targetStride);
        for (int component = 0; component < copiedComponents; component++) {
          if constexpr (std::is_same_v<Target, float>) {
            tupleTarget[component] = narrow(tupleSource[component]);
          } else {
            tupleTarget[component] = tupleSource[component];
          }
        }
        for (int component = copiedComponents; component < targetStride; component++) {
          tupleTarget[component] = Target{0};
        }
      }
    });
  }

  void validateFieldNames(const std::vector<tarch::plotter::Mesh::Field>& fields) {
    constexpr std::string_view IllegalCharacters = " \t\n\r\"'<>&/\\";
    for (std::size_t field = 0; field < fields.size(); field++) {
      const std::string& name = fields[field].name;
      if (name.empty()) {
        tarch::terminateWithError("A plot field needs a name.");
      }

      const std::size_t illegalPosition = name.find_first_of(IllegalCharacters);
      if (illegalPosition != std::string::npos) {
        tarch::terminateWithError(
          "A plot field name cannot contain whitespace, quotes, '<', '>', '&', '/' or '\\' (got '" + name + "')."
        );
      }

      for (std::size_t otherField = 0; otherField < field; otherField++) {
        if (fields[otherField].name == name) {
          tarch::terminateWithError("The plot field name '" + name + "' may appear once only.");
        }
      }
    }
  }
} // namespace

/**
 * Returns the shortest decimal representation of time that reads back as the same double.
 */
std::string
tarch::plotter::getExactTimeStamp(const double time) {
  std::array<char, 32> buffer{};
  const auto [end, error] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), time);
  assertion(error == std::errc(), time);
  return std::string(buffer.data(), end);
}

bool tarch::plotter::FileWriter::_doublePrecisionOutput{false};

void
tarch::plotter::FileWriter::setDoublePrecisionOutput(const bool value) {
  _doublePrecisionOutput = value;
}

bool
tarch::plotter::FileWriter::getDoublePrecisionOutput() {
  return _doublePrecisionOutput;
}

/**
 * Terminates for a field name that a file format cannot store.
 */
void
tarch::plotter::FileWriter::validateMesh(const Mesh& mesh) {
  validateFieldNames(mesh.pointFields);
  validateFieldNames(mesh.cellFields);

  [[maybe_unused]] const std::size_t numberOfPoints = mesh.pointsXYZ.size() / 3;
  const std::size_t                  numberOfCells  = mesh.types.size();
  assertion(mesh.pointsXYZ.size() % 3 == 0, mesh.pointsXYZ.size());
  assertion(mesh.offsets.size() == numberOfCells, mesh.offsets.size(), numberOfCells);
  assertion(
    numberOfCells == 0 ? mesh.connectivity.empty()
                       : static_cast<std::size_t>(mesh.offsets.back()) == mesh.connectivity.size(),
    numberOfCells,
    mesh.connectivity.size()
  );
  for (std::size_t cell = 1; cell < numberOfCells; cell++) {
    assertion(mesh.offsets[cell - 1] <= mesh.offsets[cell], cell, mesh.offsets[cell - 1], mesh.offsets[cell]);
  }
  for ([[maybe_unused]] const auto& field : mesh.pointFields) {
    assertion(
      field.values.size() == numberOfPoints * field.components,
      field.name,
      field.values.size(),
      numberOfPoints
    );
  }
  for ([[maybe_unused]] const auto& field : mesh.cellFields) {
    assertion(field.values.size() == numberOfCells * field.components, field.name, field.values.size(), numberOfCells);
  }
}

void
tarch::plotter::FileWriter::createOutputDirectory(const std::string& outputDirectory) {
  if (tarch::mpi::Rank::getInstance().isGlobalMaster() and not outputDirectory.empty()) {
    std::error_code filesystemError;
    std::filesystem::create_directories(outputDirectory, filesystemError);
    if (filesystemError) {
      tarch::terminateWithError(
        "The plotter cannot create the output directory '" + outputDirectory + "'. " + filesystemError.message() + "."
      );
    }
  }
  // The other ranks wait in the barrier until the global master creates the directory.
  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
}

/**
 * Returns filename followed by time with six decimals. Every dot of the result becomes an
 * underscore.
 */
std::string
tarch::plotter::FileWriter::getSnapshotName(const std::string& filename, const double time) {
  std::ostringstream stream;
  stream << filename << "-" << std::fixed << std::setprecision(6) << time;
  std::string name = stream.str();
  std::replace(name.begin(), name.end(), '.', '_');
  return name;
}

/**
 * Returns a new MPI info object with the MPI-IO hints that enable collective buffering. Only ROMIO
 * reads these keys. The caller frees the object.
 */
MPI_Info
tarch::plotter::FileWriter::createMPIFileHints() {
  MPI_Info hints = MPI_INFO_NULL;
  tarch::mpi::createInfo(&hints);
  tarch::mpi::setInfo(hints, "romio_cb_write", "enable");
  tarch::mpi::setInfo(hints, "romio_ds_write", "disable");
  return hints;
}

tarch::plotter::FileWriter::FieldLayout
tarch::plotter::FileWriter::classifyField(const int sourceComponents) {
  if (sourceComponents == 1) {
    return {FieldKind::Scalar, 1};
  }
  if (sourceComponents == 2 or sourceComponents == 3) {
    return {FieldKind::Vector, 3};
  }
  if (sourceComponents == 9) {
    return {FieldKind::Tensor, 9};
  }
  return {FieldKind::Generic, sourceComponents};
}

void
tarch::plotter::FileWriter::forEachRangeInParallel(
  const std::size_t                                                        numberOfEntries,
  const std::function<void(std::size_t firstEntry, std::size_t endEntry)>& body
) {
  forEachRange(numberOfEntries, body);
}

void
tarch::plotter::FileWriter::copyTuples(
  const double* const source,
  const int           sourceStride,
  const int           copiedComponents,
  const int           targetStride,
  const std::size_t   numberOfTuples,
  double* const       target
) {
  copyTuplesInParallel(source, sourceStride, copiedComponents, targetStride, numberOfTuples, target);
}

void
tarch::plotter::FileWriter::copyTuples(
  const double* const source,
  const int           sourceStride,
  const int           copiedComponents,
  const int           targetStride,
  const std::size_t   numberOfTuples,
  float* const        target
) {
  copyTuplesInParallel(source, sourceStride, copiedComponents, targetStride, numberOfTuples, target);
}
