// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FileWriter.h"

#include "tarch/Termination.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

bool tarch::plotter::FileWriter::_doublePrecisionOutput = false;

tarch::plotter::FileWriter::FileWriter(const std::string& outputDirectory) :
  _outputDirectory(outputDirectory) {}

/**
 * Claims one piece of the output file and returns its id. The file orders the pieces by orderKey,
 * so the output does not depend on the order of concurrent claims.
 */
int
tarch::plotter::FileWriter::acquirePiece(int orderKey) {
  const std::lock_guard<std::mutex> acquisitionGuard(_pieceAcquisitionMutex);
  if (_written) {
    tarch::terminateWithError("Cannot acquire a plot piece, as the file has already been written.");
  }

  // The order key sets the position of a piece in the file, so two pieces with one key would
  // overwrite each other.
  if (not _claimedPieceOrderKeys.insert(orderKey).second) {
    tarch::terminateWithError("Plot piece order key " + std::to_string(orderKey) + " is already claimed.");
  }

  _pieceOrderKeys.push_back(orderKey);
  _subdomains.emplace_back();
  return static_cast<int>(_subdomains.size()) - 1;
}

bool
tarch::plotter::FileWriter::isOpen() const {
  return not _written;
}

/**
 * Sets whether the writers store the fields in double or in single precision. The plotters keep
 * their values in double and narrow them as they write, so the setting changes the file size only.
 */
void
tarch::plotter::FileWriter::setDoublePrecisionOutput(const bool value) {
  _doublePrecisionOutput = value;
}

bool
tarch::plotter::FileWriter::getDoublePrecisionOutput() {
  return _doublePrecisionOutput;
}

std::vector<std::pair<std::string, std::string>>
tarch::plotter::FileWriter::getMPIFileHints() {
  // ROMIO reads these keys for collective buffering, and other MPI-IO stacks ignore them.
  return {
    {"romio_cb_write", "enable"},
    {"romio_ds_write", "disable"},
  };
}

void
tarch::plotter::FileWriter::createOutputDirectory() const {
  int         success = 1;
  std::string failureReason;

  // Only the global master touches the filesystem. It records a failure in a flag, because a stop
  // here would leave the other ranks blocked in the collective call below.
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    const std::filesystem::path outputPath(_outputDirectory);

    if (not _outputDirectory.empty()) {
      std::error_code filesystemError;
      const bool      outputPathExists = std::filesystem::exists(outputPath, filesystemError);
      if (filesystemError) {
        success = 0;
        failureReason =
          "Failed to inspect output directory '" + outputPath.string() + "': " + filesystemError.message();
      } else if (not outputPathExists) {
        std::filesystem::create_directories(outputPath, filesystemError);
        if (filesystemError) {
          success = 0;
          failureReason =
            "Failed to create output directory '" + outputPath.string() + "': " + filesystemError.message();
        }
      }

      filesystemError.clear();
      if (success != 0 and not std::filesystem::is_directory(outputPath, filesystemError)) {
        success       = 0;
        failureReason = filesystemError
          ? "Failed to inspect output directory '" + outputPath.string() + "': " + filesystemError.message()
          : "Output path exists but is not a directory: '" + outputPath.string() + "'";
      }
    }
  }

  // Every rank proceeds or stops together, and the allreduce synchronises the ranks before they
  // write.
  int globalSuccess = success;
  tarch::mpi::allreduce(
    &success,
    &globalSuccess,
    1,
    MPI_INT,
    MPI_MIN,
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
  success = globalSuccess;

  if (success == 0) {
    if (failureReason.empty()) {
      failureReason = "The global master failed to create output directory '" + _outputDirectory + "'.";
    }
    tarch::terminateWithError(failureReason);
  }
}

void
tarch::plotter::FileWriter::validateIdentifier(const std::string& id) {
  if (id.empty()) {
    throw std::runtime_error("Plotter data identifier is empty.");
  }

  // The identifier becomes an XML attribute, an HDF5 dataset path component and a NetCDF variable
  // name, so these characters are illegal.
  const std::string illegalCharacters = " \t\n\r\"'<>&/\\";
  const std::size_t illegalPosition   = id.find_first_of(illegalCharacters);
  if (illegalPosition != std::string::npos) {
    throw std::runtime_error(
      std::string("Invalid plotter data identifier '") + id + "': The character '" + id[illegalPosition]
      + "' is not allowed. Avoid whitespace, quotes, '<', '>', '&', '/' and '\\'."
    );
  }
}

/**
 * Returns the buffers of a piece. The lock guards the lookup against concurrent claims, and the
 * returned reference stays valid without it.
 */
tarch::plotter::FileWriter::SubdomainBuffers&
tarch::plotter::FileWriter::getPieceBuffers(int subdomainId) {
  const std::lock_guard<std::mutex> acquisitionGuard(_pieceAcquisitionMutex);
  if (_written) {
    throw std::runtime_error("Cannot create a data plotter after the file has been written.");
  }

  if (subdomainId < 0 or subdomainId >= static_cast<int>(_subdomains.size())) {
    throw std::runtime_error(
      "Subdomain identifier " + std::to_string(subdomainId) + " is outside the valid range for "
      + std::to_string(_subdomains.size()) + " subdomains."
    );
  }
  return _subdomains[static_cast<std::size_t>(subdomainId)];
}

/**
 * Creates a vertex plotter. Merged vertices shrink the geometry of a continuous field, and a
 * distinct vertex per cell keeps the separate values of a discontinuous field.
 */
std::unique_ptr<tarch::plotter::FileWriter::VertexPlotter>
tarch::plotter::FileWriter::createVertexPlotter(const int subdomainId, const VertexMerging vertexMerging) {
  return std::make_unique<VertexPlotter>(*this, subdomainId, vertexMerging);
}

std::unique_ptr<tarch::plotter::FileWriter::CellPlotter>
tarch::plotter::FileWriter::createCellPlotter(const int subdomainId) {
  return std::make_unique<CellPlotter>(*this, subdomainId);
}

std::unique_ptr<tarch::plotter::FileWriter::VertexDataPlotter>
tarch::plotter::FileWriter::createVertexDataPlotter(
  const int          subdomainId,
  const std::string& identifier,
  const int          recordsPerVertex
) {
  validateIdentifier(identifier);
  return std::make_unique<VertexDataPlotter>(*this, subdomainId, identifier, recordsPerVertex);
}

std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter>
tarch::plotter::FileWriter::createCellDataPlotter(
  const int          subdomainId,
  const std::string& identifier,
  const int          recordsPerCell
) {
  validateIdentifier(identifier);
  return std::make_unique<CellDataPlotter>(*this, subdomainId, identifier, recordsPerCell);
}

tarch::plotter::FileWriter::DataPlotter::DataPlotter(
  std::vector<DataField>& targetFields,
  const std::string&      identifier,
  const int               components
) :
  _targetFields(targetFields),
  _components(components) {
  assertion(components > 0, components);

  for (std::size_t field = 0; field < _targetFields.size(); field++) {
    if (_targetFields[field].name == identifier) {
      if (_targetFields[field].components != components) {
        throw std::runtime_error("Field '" + identifier + "' already exists with different component count.");
      }
      _fieldIndex = static_cast<int>(field);
      break;
    }
  }

  if (_fieldIndex == -1) {
    DataField newField;
    newField.name       = identifier;
    newField.components = components;
    _targetFields.push_back(newField);
    _fieldIndex = static_cast<int>(_targetFields.size()) - 1;
  }
}

bool
tarch::plotter::FileWriter::hasSchemaOrGeometry(const SubdomainBuffers& buffers) {
  return not buffers.pointsXYZ.empty() or not buffers.connectivity.empty() or not buffers.offsets.empty()
    or not buffers.types.empty() or not buffers.pointFields.empty() or not buffers.cellFields.empty();
}

bool
tarch::plotter::FileWriter::hasPayload(const SubdomainBuffers& buffers) {
  if (
    not buffers.pointsXYZ.empty() or not buffers.connectivity.empty() or not buffers.offsets.empty()
    or not buffers.types.empty()
  ) {
    return true;
  }

  for (const auto& field : buffers.pointFields) {
    if (not field.values.empty()) {
      return true;
    }
  }
  for (const auto& field : buffers.cellFields) {
    if (not field.values.empty()) {
      return true;
    }
  }
  return false;
}

const tarch::plotter::FileWriter::SubdomainBuffers*
tarch::plotter::FileWriter::findReferenceSubdomain() const {
  for (const auto& subdomain : _subdomains) {
    if (hasSchemaOrGeometry(subdomain)) {
      return &subdomain;
    }
  }
  return nullptr;
}

void
tarch::plotter::FileWriter::validateSubdomainsForWriting(const SubdomainBuffers& reference) const {
  for (const auto& subdomain : _subdomains) {
    if (not hasSchemaOrGeometry(subdomain)) {
      continue;
    }

    if (subdomain.pointsXYZ.size() % 3 != 0) {
      throw std::runtime_error("Point coordinate buffer size is not a multiple of 3.");
    }

    if (subdomain.offsets.size() != subdomain.types.size()) {
      throw std::runtime_error("Cell offset count does not match cell type count.");
    }

    if (subdomain.offsets.empty() and not subdomain.connectivity.empty()) {
      throw std::runtime_error("Connectivity buffer is non-empty but no cell offsets are defined.");
    }

    std::int32_t previousOffset = 0;
    for (const std::int32_t offset : subdomain.offsets) {
      if (offset < previousOffset) {
        throw std::runtime_error("Cell offsets are not monotonically increasing.");
      }
      previousOffset = offset;
    }

    if (
      not subdomain.offsets.empty()
      and static_cast<std::size_t>(subdomain.offsets.back()) != subdomain.connectivity.size()
    ) {
      throw std::runtime_error("Last cell offset does not match connectivity buffer size.");
    }

    if (
      subdomain.pointFields.size() != reference.pointFields.size()
      or subdomain.cellFields.size() != reference.cellFields.size()
    ) {
      throw std::runtime_error("Inconsistent data field counts across subdomains.");
    }

    for (std::size_t field = 0; field < reference.pointFields.size(); field++) {
      if (
        subdomain.pointFields[field].name != reference.pointFields[field].name
        or subdomain.pointFields[field].components != reference.pointFields[field].components
      ) {
        throw std::runtime_error("Mismatch in point data field '" + reference.pointFields[field].name + "'.");
      }
    }

    for (std::size_t field = 0; field < reference.cellFields.size(); field++) {
      if (
        subdomain.cellFields[field].name != reference.cellFields[field].name
        or subdomain.cellFields[field].components != reference.cellFields[field].components
      ) {
        throw std::runtime_error("Mismatch in cell data field '" + reference.cellFields[field].name + "'.");
      }
    }

    const std::size_t numberOfPoints = subdomain.pointsXYZ.size() / 3;
    const std::size_t numberOfCells  = subdomain.types.size();

    for (const auto& field : subdomain.pointFields) {
      if (field.values.size() != numberOfPoints * field.components) {
        throw std::runtime_error(
          "Mismatch in point data field '" + field.name + "'. Expected " + std::to_string(numberOfPoints)
          + " tuples but got " + std::to_string(field.values.size() / field.components) + "."
        );
      }
    }

    for (const auto& field : subdomain.cellFields) {
      if (field.values.size() != numberOfCells * field.components) {
        throw std::runtime_error(
          "Mismatch in cell data field '" + field.name + "'. Expected " + std::to_string(numberOfCells)
          + " tuples but got " + std::to_string(field.values.size() / field.components) + "."
        );
      }
    }
  }
}

tarch::plotter::FileWriter::SubdomainBuffersSequence
tarch::plotter::FileWriter::getSubdomainBuffersSequenceForWriting() const {
  SubdomainBuffersSequence result;

  result.reference = findReferenceSubdomain();
  if (result.reference == nullptr) {
    return result;
  }

  validateSubdomainsForWriting(*result.reference);

  // The pieces enter the file ordered by their keys, so the output does not
  // depend on the order in which concurrent writers claimed them.
  std::vector<std::size_t> pieceOrder(_subdomains.size());
  std::iota(pieceOrder.begin(), pieceOrder.end(), std::size_t{0});
  std::sort(pieceOrder.begin(), pieceOrder.end(), [this](std::size_t leftPiece, std::size_t rightPiece) {
    return _pieceOrderKeys[leftPiece] < _pieceOrderKeys[rightPiece];
  });

  result.payloads.reserve(_subdomains.size());
  for (const std::size_t piece : pieceOrder) {
    if (hasPayload(_subdomains[piece])) {
      result.payloads.push_back(&_subdomains[piece]);
    }
  }

  return result;
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

/**
 * Returns the tuples of values widened from sourceComponents to writtenComponents, with zero in
 * every added component.
 */
std::vector<double>
tarch::plotter::FileWriter::expandFieldComponents(
  const std::vector<double>& values,
  const int                  sourceComponents,
  const int                  writtenComponents
) {
  if (writtenComponents == sourceComponents or sourceComponents <= 0) {
    return values;
  }
  const std::size_t   tuples = values.size() / static_cast<std::size_t>(sourceComponents);
  std::vector<double> result(tuples * static_cast<std::size_t>(writtenComponents), 0.0);
  for (std::size_t tuple = 0; tuple < tuples; tuple++) {
    for (int component = 0; component < sourceComponents; component++) {
      result[tuple * writtenComponents + component] = values[tuple * sourceComponents + component];
    }
  }
  return result;
}

std::vector<float>
tarch::plotter::FileWriter::narrowToFloat(const std::vector<double>& values) {
  std::vector<float> result(values.size());
  std::transform(values.begin(), values.end(), result.begin(), [](const double value) {
    // A value below the normal float range flushes to signed zero and a value beyond the finite
    // range saturates, so the narrowing raises no floating-point exception.
    if (std::abs(value) < static_cast<double>(std::numeric_limits<float>::min())) {
      return std::signbit(value) ? -0.0F : 0.0F;
    }
    if (std::abs(value) > static_cast<double>(std::numeric_limits<float>::max())) {
      return std::signbit(value) ? -std::numeric_limits<float>::max() : std::numeric_limits<float>::max();
    }
    return static_cast<float>(value);
  });
  return result;
}

/**
 * Returns the field names and component counts of localReference without communication. The
 * collective writes run on the ranks with data only, so every participant has the same fields.
 */
tarch::plotter::FileWriter::FieldSchema
tarch::plotter::FileWriter::getFieldSchema(const SubdomainBuffers* const localReference) {
  FieldSchema schema;
  if (localReference != nullptr) {
    const auto seed = [](std::vector<DataField>& target, const std::vector<DataField>& source) {
      target.reserve(source.size());
      for (const auto& field : source) {
        DataField entry;
        entry.name       = field.name;
        entry.components = field.components;
        target.push_back(std::move(entry));
      }
    };
    seed(schema.pointFields, localReference->pointFields);
    seed(schema.cellFields, localReference->cellFields);
  }
  return schema;
}
