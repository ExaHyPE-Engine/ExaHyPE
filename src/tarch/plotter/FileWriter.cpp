// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FileWriter.h"

#include "tarch/Termination.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

bool tarch::plotter::FileWriter::_doublePrecisionOutput = false;

tarch::plotter::FileWriter::FileWriter(const std::string& outputDirectory, const int numberOfSubdomains) :
  _outputDirectory(outputDirectory),
  _subdomains(static_cast<std::size_t>(numberOfSubdomains)) {}

bool tarch::plotter::FileWriter::isOpen() const { return not _written; }

/**
 * Select the width the writers store their fields in.
 *
 * The setting reaches the file only. Every plotter accumulates its points and fields in
 * DataField, whose values are double, and narrows them as it writes, so single precision
 * halves the file and leaves the memory a plot holds unchanged. Keeping the buffers in
 * double is what lets a plotter serve both settings without a second code path, and what
 * keeps the accumulated geometry exact while subdomains are merged.
 */
void tarch::plotter::FileWriter::setDoublePrecisionOutput(const bool value) { _doublePrecisionOutput = value; }

bool tarch::plotter::FileWriter::getDoublePrecisionOutput() { return _doublePrecisionOutput; }

std::vector<std::pair<std::string, std::string>> tarch::plotter::FileWriter::mpiIoHints() {
  // Collective buffering for these large collective writes. ROMIO keys (MPICH); other
  // MPI-IO stacks ignore unknown keys.
  return {
    {"romio_cb_write", "enable"},
    {"romio_ds_write", "disable"},
  };
}

void tarch::plotter::FileWriter::createOutputDirectory() const {
  int         success = 1;
  std::string failureReason;

  // The global master records the filesystem outcome before all ranks agree on it.
  // Entering the fatal path before this collective would leave the peers blocked.
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    const std::filesystem::path outputPath(_outputDirectory);

    if (not _outputDirectory.empty()) {
      std::error_code ec;
      if (not std::filesystem::exists(outputPath)) {
        if (not std::filesystem::create_directories(outputPath, ec) and not std::filesystem::exists(outputPath)) {
          success       = 0;
          failureReason = "Failed to create output directory '" + outputPath.string() + "': " + ec.message() + ".";
        }
      } else if (not std::filesystem::is_directory(outputPath)) {
        success       = 0;
        failureReason = "Output path '" + outputPath.string() + "' is not a directory.";
      }
    }
  }

  // Agree on the outcome so every rank proceeds or terminates together. This allreduce
  // also synchronises the ranks, so no separate barrier is needed before they write.
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

void tarch::plotter::FileWriter::validateIdentifier(const std::string& id) {
  if (id.empty()) {
    throw std::runtime_error("Plotter data identifier is empty.");
  }

  // The identifier is written into VTU/XDMF XML, as an HDF5 dataset path
  // component and as a NetCDF variable name. Reject the characters that would
  // produce malformed XML, an invalid HDF5 path component, or a rejected NetCDF
  // variable name.
  const std::string illegal = " \t\n\r\"'<>&/\\";
  const std::size_t pos     = id.find_first_of(illegal);
  if (pos != std::string::npos) {
    throw std::runtime_error(
      std::string("Invalid plotter data identifier '") + id + "': the character '" + id[pos]
      + "' is not allowed. Avoid whitespace, quotes, '<', '>', '&', '/' and '\\'."
    );
  }
}

void tarch::plotter::FileWriter::checkCanOpenPlotter(const int subdomainId) const {
  if (_written) {
    throw std::runtime_error("Cannot create a data plotter after the file has been written.");
  }

  if (subdomainId < 0 or subdomainId >= static_cast<int>(_subdomains.size())) {
    throw std::runtime_error(
      "Subdomain identifier " + std::to_string(subdomainId) + " is outside the valid range for "
      + std::to_string(_subdomains.size()) + " subdomains."
    );
  }
}

/**
 * Create a vertex plotter with the requested treatment of coincident points.
 *
 * Shared vertices reduce the geometry size for continuous fields. Distinct vertices
 * preserve separate point-data values at the same position, as required by
 * discontinuous fields.
 */
std::unique_ptr<tarch::plotter::FileWriter::VertexPlotter> tarch::plotter::FileWriter::createVertexPlotter(
  const int           subdomainId,
  const VertexSharing vertexSharing
) {
  return std::make_unique<VertexPlotter>(*this, subdomainId, vertexSharing);
}

std::unique_ptr<tarch::plotter::FileWriter::CellPlotter> tarch::plotter::FileWriter::createCellPlotter(
  const int subdomainId
) {
  return std::make_unique<CellPlotter>(*this, subdomainId);
}

std::unique_ptr<tarch::plotter::FileWriter::VertexDataPlotter> tarch::plotter::FileWriter::createVertexDataPlotter(
  const int          subdomainId,
  const std::string& identifier,
  const int          recordsPerVertex
) {
  validateIdentifier(identifier);
  return std::make_unique<VertexDataPlotter>(*this, subdomainId, identifier, recordsPerVertex);
}

std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> tarch::plotter::FileWriter::createCellDataPlotter(
  const int          subdomainId,
  const std::string& identifier,
  const int          recordsPerCell
) {
  validateIdentifier(identifier);
  return std::make_unique<CellDataPlotter>(*this, subdomainId, identifier, recordsPerCell);
}

tarch::plotter::FileWriter::DataPlotter::DataPlotter(
  FileWriter&             plotter,
  const int               subdomainId,
  std::vector<DataField>& targetFields,
  const std::string&      identifier,
  const int               components
) :
  _plotter(plotter),
  _targetFields(targetFields),
  _components(components) {
  _plotter.checkCanOpenPlotter(subdomainId);

  // Reuse the field if it already exists, otherwise append it.
  _fieldIndex = -1;
  for (size_t i = 0; i < _targetFields.size(); ++i) {
    if (_targetFields[i].name == identifier) {
      if (_targetFields[i].components != components) {
        throw std::runtime_error("Field '" + identifier + "' already exists with different component count.");
      }
      _fieldIndex = static_cast<int>(i);
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

bool tarch::plotter::FileWriter::hasSchemaOrGeometry(const SubdomainBuffers& buffers) {
  return not buffers.pointsXYZ.empty() or not buffers.connectivity.empty() or not buffers.offsets.empty()
    or not buffers.types.empty() or not buffers.pointFields.empty() or not buffers.cellFields.empty();
}

bool tarch::plotter::FileWriter::hasPayload(const SubdomainBuffers& buffers) {
  if (not buffers.pointsXYZ.empty() or not buffers.connectivity.empty() or not buffers.offsets.empty()
      or not buffers.types.empty()) {
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

const tarch::plotter::FileWriter::SubdomainBuffers* tarch::plotter::FileWriter::findReferenceSubdomain() const {
  for (const auto& s : _subdomains) {
    if (hasSchemaOrGeometry(s)) {
      return &s;
    }
  }
  return nullptr;
}

void tarch::plotter::FileWriter::validateSubdomainsForWriting(const SubdomainBuffers& reference) const {
  for (const auto& s : _subdomains) {
    if (not hasSchemaOrGeometry(s)) {
      continue;
    }

    if (s.pointsXYZ.size() % 3 != 0) {
      throw std::runtime_error("Point coordinate buffer size is not a multiple of 3.");
    }

    if (s.offsets.size() != s.types.size()) {
      throw std::runtime_error("Cell offset count does not match cell type count.");
    }

    if (s.offsets.empty() and not s.connectivity.empty()) {
      throw std::runtime_error("Connectivity buffer is non-empty but no cell offsets are defined.");
    }

    std::int32_t previousOffset = 0;
    for (const std::int32_t offset : s.offsets) {
      if (offset < previousOffset) {
        throw std::runtime_error("Cell offsets are not monotonically increasing.");
      }
      previousOffset = offset;
    }

    if (not s.offsets.empty() and static_cast<std::size_t>(s.offsets.back()) != s.connectivity.size()) {
      throw std::runtime_error("Last cell offset does not match connectivity buffer size.");
    }

    if (s.pointFields.size() != reference.pointFields.size() or s.cellFields.size() != reference.cellFields.size()) {
      throw std::runtime_error("Inconsistent data field counts across subdomains.");
    }

    for (std::size_t i = 0; i < reference.pointFields.size(); ++i) {
      if (s.pointFields[i].name != reference.pointFields[i].name
          or s.pointFields[i].components != reference.pointFields[i].components) {
        throw std::runtime_error("Mismatch in point data field '" + reference.pointFields[i].name + "'.");
      }
    }

    for (std::size_t i = 0; i < reference.cellFields.size(); ++i) {
      if (s.cellFields[i].name != reference.cellFields[i].name
          or s.cellFields[i].components != reference.cellFields[i].components) {
        throw std::runtime_error("Mismatch in cell data field '" + reference.cellFields[i].name + "'.");
      }
    }

    const std::size_t nPoints = s.pointsXYZ.size() / 3;
    const std::size_t nCells  = s.types.size();

    for (const auto& f : s.pointFields) {
      if (f.values.size() != nPoints * f.components) {
        throw std::runtime_error(
          "Mismatch in point data field '" + f.name + "'. Expected " + std::to_string(nPoints) + " tuples but got "
          + std::to_string(f.values.size() / f.components) + "."
        );
      }
    }

    for (const auto& f : s.cellFields) {
      if (f.values.size() != nCells * f.components) {
        throw std::runtime_error(
          "Mismatch in cell data field '" + f.name + "'. Expected " + std::to_string(nCells) + " tuples but got "
          + std::to_string(f.values.size() / f.components) + "."
        );
      }
    }
  }
}

tarch::plotter::FileWriter::SubdomainBuffersSequence tarch::plotter::FileWriter::
  getSubdomainBuffersSequenceForWriting() const {
  SubdomainBuffersSequence result;

  result.reference = findReferenceSubdomain();
  if (result.reference == nullptr) {
    return result;
  }

  validateSubdomainsForWriting(*result.reference);

  result.payloads.reserve(_subdomains.size());
  for (const auto& s : _subdomains) {
    if (hasPayload(s)) {
      result.payloads.push_back(&s);
    }
  }

  return result;
}

tarch::plotter::FileWriter::FieldLayout tarch::plotter::FileWriter::classifyField(const int sourceComponents) {
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

std::vector<double> tarch::plotter::FileWriter::expandFieldComponents(
  const std::vector<double>& values,
  const int                  sourceComponents,
  const int                  writtenComponents
) {
  if (writtenComponents == sourceComponents or sourceComponents <= 0) {
    return values;
  }
  const std::size_t   tuples = values.size() / static_cast<std::size_t>(sourceComponents);
  std::vector<double> result(tuples * static_cast<std::size_t>(writtenComponents), 0.0);
  for (std::size_t t = 0; t < tuples; ++t) {
    for (int c = 0; c < sourceComponents; ++c) {
      result[t * writtenComponents + c] = values[t * sourceComponents + c];
    }
  }
  return result;
}

std::vector<float> tarch::plotter::FileWriter::narrowToFloat(const std::vector<double>& values) {
  std::vector<float> result(values.size());
  std::transform(values.begin(), values.end(), result.begin(), [](double value) {
    // The narrowing must not trap when floating-point exceptions are enabled:
    // a value below float's normal range flushes to signed zero and a value
    // beyond its finite range saturates, so every solution state remains
    // plottable.
    if (std::abs(value) < static_cast<double>(std::numeric_limits<float>::min())) {
      return std::signbit(value) ? -0.0f : 0.0f;
    }
    if (std::abs(value) > static_cast<double>(std::numeric_limits<float>::max())) {
      return std::signbit(value) ? -std::numeric_limits<float>::max() : std::numeric_limits<float>::max();
    }
    return static_cast<float>(value);
  });
  return result;
}

tarch::plotter::FileWriter::FieldSchema tarch::plotter::FileWriter::fieldSchemaFromReference(
  const SubdomainBuffers* const localReference
) {
  // Names and component counts only; the field values are streamed from the
  // subdomain buffers at write time.
  FieldSchema schema;
  if (localReference != nullptr) {
    const auto seed = [](std::vector<DataField>& target, const std::vector<DataField>& source) {
      target.reserve(source.size());
      for (const auto& f : source) {
        DataField entry;
        entry.name       = f.name;
        entry.components = f.components;
        target.push_back(std::move(entry));
      }
    };
    seed(schema.pointFields, localReference->pointFields);
    seed(schema.cellFields, localReference->cellFields);
  }
  return schema;
}
