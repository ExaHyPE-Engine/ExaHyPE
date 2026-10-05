// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PatchPlotter.h"

exahype2::plotter::PatchPlotter::PatchPlotter(std::string filename, std::vector<DataField> fields) :
  _filename(std::move(filename)),
  _fields(std::move(fields)) {
  assertion(not _filename.empty());
  for ([[maybe_unused]] const auto& field : _fields) {
    assertion(field.components > 0, field.name, field.components);
  }
}

std::vector<exahype2::plotter::PatchPlotter::PatchCorner>
exahype2::plotter::PatchPlotter::classifyCorners(const int cornersPerAxis) {
  assertion(cornersPerAxis > 1, cornersPerAxis);
  std::vector<PatchCorner> result(static_cast<std::size_t>(peano4::power(cornersPerAxis, DIMENSIONS)));
  int                      faceCorners     = 0;
  int                      interiorCorners = 0;
  for (std::size_t corner = 0; corner < result.size(); corner++) {
    const auto cornerIndex = peano4::dDelinearised(static_cast<int>(corner), cornersPerAxis);
    bool       onFace      = false;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      const int step = cornerIndex(axis);
      onFace         = onFace or step == 0 or step == cornersPerAxis - 1;
    }
    if (onFace) {
      result[corner] = {true, faceCorners};
      faceCorners++;
    } else {
      result[corner] = {false, interiorCorners};
      interiorCorners++;
    }
  }
  assertion(interiorCorners == peano4::power(cornersPerAxis - 2, DIMENSIONS), interiorCorners, cornersPerAxis);
  return result;
}

std::uint64_t
exahype2::plotter::PatchPlotter::hashLatticeKey(const std::array<std::int64_t, DIMENSIONS>& key) {
  constexpr std::uint64_t GoldenRatio = 0x9E3779B97F4A7C15ULL;
  std::uint64_t           result      = 0;
  for (const std::int64_t coordinate : key) {
    result ^= static_cast<std::uint64_t>(coordinate) + GoldenRatio + (result << 6) + (result >> 2);
  }
  result ^= result >> 30;
  result *= 0xBF58476D1CE4E5B9ULL;
  result ^= result >> 27;
  result *= 0x94D049BB133111EBULL;
  result ^= result >> 31;
  return result;
}

/**
 * Returns a mesh with the arrays of the patches and numberOfPoints points. The data fields become
 * point fields or cell fields, followed by the cell fields RefinementLevel, Subdomain, CurveIndex
 * and, with writesStatus, LimiterStatus. The arrays stay uninitialised. The caller writes every
 * entry. Every plot cell of a patch stores the curve index of its grid cell. A single-precision
 * plot file stores the curve indices of up to 2^24 grid cells exactly.
 */
tarch::plotter::Mesh
exahype2::plotter::PatchPlotter::createMesh(
  const int         numberOfPatches,
  const int         cellsPerPatch,
  const std::size_t numberOfPoints,
  const bool        writesPointData,
  const bool        writesStatus
) const {
  using Values = tarch::plotter::MeshArray<double>;
  assertion(numberOfPatches >= 0, numberOfPatches);
  assertion(cellsPerPatch > 0, cellsPerPatch);
  const auto numberOfCells = static_cast<std::size_t>(numberOfPatches) * cellsPerPatch;

  tarch::plotter::Mesh mesh;
  mesh.pointsXYZ.resize(numberOfPoints * 3);
  mesh.connectivity.resize(numberOfCells * peano4::VerticesPerCell);
  mesh.offsets.resize(numberOfCells);
  mesh.types.resize(numberOfCells);
  for (const auto& field : _fields) {
    auto&      fields   = writesPointData ? mesh.pointFields : mesh.cellFields;
    const auto entities = writesPointData ? numberOfPoints : numberOfCells;
    fields.push_back({field.name, field.components, Values(entities * field.components)});
  }
  mesh.cellFields.push_back({"RefinementLevel", 1, Values(numberOfCells)});
  mesh.cellFields.push_back({"Subdomain", 1, Values(numberOfCells)});
  mesh.cellFields.push_back({"CurveIndex", 1, Values(numberOfCells)});
  if (writesStatus) {
    mesh.cellFields.push_back({"LimiterStatus", 1, Values(numberOfCells)});
  }
  return mesh;
}

/**
 * Writes mesh as the snapshot at timeStamp. Every rank calls this at the same time with the same
 * format.
 */
void
exahype2::plotter::PatchPlotter::write(
  const tarch::plotter::Mesh&  mesh,
  const double                 timeStamp,
  const tarch::plotter::Format format,
  const std::string&           outputDirectory
) {
  if (_fileWriter == nullptr) {
    _fileWriter = tarch::plotter::createFileWriter(format);
  }
  assertion(_fileWriter->getFormat() == format);
  _fileWriter->write(mesh, outputDirectory + OutputSubdirectory, _filename, timeStamp);
}
