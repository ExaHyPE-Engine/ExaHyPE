// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FileWriter.h"

#include "tarch/mpi/Rank.h"

#include <algorithm> // for std::transform, std::max
#include <filesystem>
#include <iterator>  // for std::back_inserter
#include <stdexcept> // for std::runtime_error

namespace fs = std::filesystem;

tarch::plotter::FileWriter::FileWriter(const std::string& outputDirectory, int numberOfSubdomains):
  _outputDirectory(outputDirectory),
  _subdomains(static_cast<std::size_t>(numberOfSubdomains)) {}

bool tarch::plotter::FileWriter::isOpen() const { return not _written; }

std::string tarch::plotter::FileWriter::getOutputDirectory() const { return _outputDirectory; }

void tarch::plotter::FileWriter::createOutputDirectory() {
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    fs::path p(_outputDirectory);

    if (not _outputDirectory.empty()) {
      std::error_code ec;
      if (not fs::exists(p)) {
        if (not fs::create_directories(p, ec)) {
          if (not fs::exists(p)) {
            throw std::runtime_error("Failed to create output directory '" + p.string() + "': " + ec.message());
          }
        }
      } else if (not fs::is_directory(p)) {
        throw std::runtime_error("Output path exists but is not a directory: '" + p.string() + "'");
      }
    }
  }

  tarch::mpi::Rank::getInstance().barrier();
}

void tarch::plotter::FileWriter::validateIdentifier(const std::string& id) {
  if (id.find(' ') != std::string::npos) {
    throw std::runtime_error("VTU data identifier contains spaces: '" + id + "'");
  }
}

int tarch::plotter::FileWriter::findField(std::vector<DataField>& fields, const std::string& name) {
  for (int i = 0; i < static_cast<int>(fields.size()); ++i) {
    if (fields[i].name == name) {
      return i;
    }
  }
  return -1;
}

void tarch::plotter::FileWriter::checkCanOpenPlotter(int subdomainId) const {
  if (_written) {
    throw std::runtime_error("Cannot create plotter. File already written.");
  }

  if (subdomainId < 0 or subdomainId >= static_cast<int>(_subdomains.size())) {
    throw std::runtime_error(
      "Invalid subdomainId " + std::to_string(subdomainId) + " (have " + std::to_string(_subdomains.size())
      + " subdomains)"
    );
  }
}

std::unique_ptr<tarch::plotter::FileWriter::VertexPlotter> tarch::plotter::FileWriter::createVertexPlotter(
  int subdomainId
) {
  return std::make_unique<VertexPlotter>(*this, subdomainId);
}

std::unique_ptr<tarch::plotter::FileWriter::CellPlotter> tarch::plotter::FileWriter::createCellPlotter(
  int subdomainId
) {
  return std::make_unique<CellPlotter>(*this, subdomainId);
}

std::unique_ptr<tarch::plotter::FileWriter::VertexDataPlotter> tarch::plotter::FileWriter::
  createVertexDataPlotter(int subdomainId, const std::string& identifier, int recordsPerVertex) {
  validateIdentifier(identifier);
  return std::make_unique<VertexDataPlotter>(*this, subdomainId, identifier, recordsPerVertex);
}

std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> tarch::plotter::FileWriter::
  createCellDataPlotter(int subdomainId, const std::string& identifier, int recordsPerCell) {
  validateIdentifier(identifier);
  return std::make_unique<CellDataPlotter>(*this, subdomainId, identifier, recordsPerCell);
}

tarch::plotter::FileWriter::DataPlotter::DataPlotter(
  FileWriter&             plotter,
  int                     subdomainId,
  std::vector<DataField>& targetFields,
  const std::string&      identifier,
  int                     components
):
  _plotter(plotter),
  _targetFields(targetFields),
  _components(components) {

  _plotter.checkCanOpenPlotter(subdomainId);

  // Find or create the field
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

tarch::plotter::FileWriter::SubdomainBuffers tarch::plotter::FileWriter::mergeSubdomains() {
  auto mergedSubdomains = SubdomainBuffers();

  // We need a reference subdomain to determine which data fields exist.
  // We accept a subdomain as a reference if it has geometry or if it has
  // defined data fields. This ensures we capture the schema even on empty ranks
  // (e.g. particles that are not hosted on a particular rank).
  const SubdomainBuffers* ref = nullptr;
  for (const auto& s : _subdomains) {
    if (not s.pointsXYZ.empty() or not s.types.empty() or not s.pointFields.empty() or not s.cellFields.empty()) {
      ref = &s;
      break;
    }
  }

  // Leave merged subdomain empty and return.
  if (ref == nullptr) {
    return mergedSubdomains;
  }

  // Check that all other non-empty subdomains match the reference schema
  for (const auto& s : _subdomains) {
    // Skip checking completely empty subdomains (no geometry, no fields)
    if (s.pointsXYZ.empty() and s.types.empty() and s.pointFields.empty() and s.cellFields.empty()) {
      continue;
    }

    if (s.pointFields.size() != ref->pointFields.size() or s.cellFields.size() != ref->cellFields.size()) {
      // If a subdomain has geometry but no fields (or vice versa) compared to ref, this is a mismatch
      throw std::runtime_error("Inconsistent data field counts across subdomains.");
    }

    for (std::size_t i = 0; i < ref->pointFields.size(); ++i) {
      if (s.pointFields[i].name != ref->pointFields[i].name
          or s.pointFields[i].components != ref->pointFields[i].components) {
        throw std::runtime_error("Mismatch in point data field '" + ref->pointFields[i].name + "'.");
      }
    }

    for (std::size_t i = 0; i < ref->cellFields.size(); ++i) {
      if (s.cellFields[i].name != ref->cellFields[i].name
          or s.cellFields[i].components != ref->cellFields[i].components) {
        throw std::runtime_error("Mismatch in cell data field '" + ref->cellFields[i].name + "'.");
      }
    }
  }

  // Pre-calculate totals for memory reservation
  std::size_t totalPoints = 0;
  std::size_t totalCells  = 0;
  std::size_t totalConn   = 0;

  for (const auto& s : _subdomains) {
    totalPoints += s.pointsXYZ.size(); // size in doubles/floats
    totalCells += s.types.size();
    totalConn += s.connectivity.size();
  }

  // Allocate merged buffers
  mergedSubdomains.pointsXYZ.reserve(totalPoints);
  mergedSubdomains.connectivity.reserve(totalConn);
  mergedSubdomains.offsets.reserve(totalCells);
  mergedSubdomains.types.reserve(totalCells);

  // Initialise merged fields (based on reference)
  mergedSubdomains.pointFields = ref->pointFields;
  mergedSubdomains.cellFields  = ref->cellFields;

  for (auto& f : mergedSubdomains.pointFields) {
    f.values.clear();
    f.values.reserve(totalPoints / 3 * f.components);
  }
  for (auto& f : mergedSubdomains.cellFields) {
    f.values.clear();
    f.values.reserve(totalCells * f.components);
  }

  std::int32_t pointIndexOffset = 0;
  std::int32_t cellOffsetBase   = 0;

  for (const auto& s : _subdomains) {
    if (s.pointsXYZ.empty() and s.types.empty()) {
      continue;
    }

    const std::size_t nPoints = s.pointsXYZ.size() / 3;
    const std::size_t nCells  = s.types.size();

    // Check point data consistency
    for (const auto& f : s.pointFields) {
      if (f.values.size() != nPoints * f.components) {
        throw std::runtime_error(
          "Mismatch in point data field '" + f.name + "'. Expected " + std::to_string(nPoints) + " tuples but got "
          + std::to_string(f.values.size() / f.components) + "."
        );
      }
    }

    // Check cell data consistency
    for (const auto& f : s.cellFields) {
      if (f.values.size() != nCells * f.components) {
        throw std::runtime_error(
          "Mismatch in cell data field '" + f.name + "'. Expected " + std::to_string(nCells) + " tuples but got "
          + std::to_string(f.values.size() / f.components) + "."
        );
      }
    }

    // Geometry
    mergedSubdomains.pointsXYZ.insert(mergedSubdomains.pointsXYZ.end(), s.pointsXYZ.begin(), s.pointsXYZ.end());
    mergedSubdomains.types.insert(mergedSubdomains.types.end(), s.types.begin(), s.types.end());

    // Connectivity (indices must be shifted by previous point count)
    std::transform(
      s.connectivity.begin(),
      s.connectivity.end(),
      std::back_inserter(mergedSubdomains.connectivity),
      [&](std::int32_t idx) { return idx + pointIndexOffset; }
    );

    // Offsets (must be cumulative across the whole file)
    // The subdomain offsets start at 0 relative to that subdomain.
    // We add the `cellOffsetBase` (the end of the previous subdomain) to them.
    for (auto o : s.offsets) {
      mergedSubdomains.offsets.push_back(o + cellOffsetBase);
    }

    // Update bases for next iteration
    if (not s.offsets.empty()) {
      // The last offset of this block becomes the base for the next block
      cellOffsetBase = mergedSubdomains.offsets.back();
    }

    pointIndexOffset += static_cast<std::int32_t>(nPoints);

    // Data fields
    for (std::size_t i = 0; i < s.pointFields.size(); ++i) {
      std::size_t expectedSize = nPoints * s.pointFields[i].components;
      if (s.pointFields[i].values.size() != expectedSize) {
        throw std::runtime_error(
          "Point field '" + s.pointFields[i].name + "' has invalid size "
          + std::to_string(s.pointFields[i].values.size()) + ". Expected " + std::to_string(expectedSize) + " ("
          + std::to_string(nPoints) + " points x " + std::to_string(s.pointFields[i].components) + " components)."
        );
      }

      mergedSubdomains.pointFields[i].values.insert(
        mergedSubdomains.pointFields[i].values.end(),
        s.pointFields[i].values.begin(),
        s.pointFields[i].values.end()
      );
    }

    for (std::size_t i = 0; i < s.cellFields.size(); ++i) {
      std::size_t expectedSize = nCells * s.cellFields[i].components;
      if (s.cellFields[i].values.size() != expectedSize) {
        throw std::runtime_error(
          "Cell field '" + s.cellFields[i].name + "' has invalid size " + std::to_string(s.cellFields[i].values.size())
          + ". Expected " + std::to_string(expectedSize) + " (" + std::to_string(nCells) + " cells x "
          + std::to_string(s.cellFields[i].components) + " components)."
        );
      }

      mergedSubdomains.cellFields[i].values.insert(
        mergedSubdomains.cellFields[i].values.end(),
        s.cellFields[i].values.begin(),
        s.cellFields[i].values.end()
      );
    }
  }

  return mergedSubdomains;
}
