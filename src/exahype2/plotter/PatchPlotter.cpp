// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PatchPlotter.h"

#include "tarch/mpi/Rank.h"
#include "tarch/plotter/CellTypes.h"

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

/**
 * Opens the snapshot file, sizes the piece of this rank and returns its id. The cell fields
 * RefinementLevel, Subdomain, CurveIndex and, with writesStatus, LimiterStatus follow the data
 * fields. All plot cells of one patch carry the curve index of its grid cell, which a single
 * precision plot file holds exactly up to 2^24 grid cells.
 */
int
exahype2::plotter::PatchPlotter::openPiece(
  const int                    numberOfPatches,
  const int                    cellsPerPatch,
  const std::size_t            numberOfPoints,
  const bool                   writesPointData,
  const bool                   writesStatus,
  const double                 timeStamp,
  const tarch::plotter::Format format,
  const std::string&           outputDirectory
) {
  assertion(numberOfPatches >= 0, numberOfPatches);
  assertion(cellsPerPatch > 0, cellsPerPatch);
  constexpr auto CellType =
    DIMENSIONS == 2 ? tarch::plotter::CellType::Quadrangle : tarch::plotter::CellType::Hexahedron;
  const auto numberOfCells = static_cast<std::size_t>(numberOfPatches) * cellsPerPatch;
  const int  rank          = tarch::mpi::Rank::getInstance().getRank();

  _snapshot.openFile(format, outputDirectory + OutputSubdirectory, timeStamp);
  auto&     writer  = _snapshot.getFileWriter();
  const int pieceId = writer.acquirePiece(rank);
  auto&     piece   = writer.getPieceBuffers(pieceId);
  piece.pointsXYZ.assign(numberOfPoints * 3, 0.0);
  piece.connectivity.resize(numberOfCells * peano4::VerticesPerCell);
  piece.offsets.resize(numberOfCells);
  piece.types.assign(numberOfCells, static_cast<std::uint8_t>(CellType));
  piece.pointFields.clear();
  piece.cellFields.clear();
  for (const auto& field : _fields) {
    auto&      fields   = writesPointData ? piece.pointFields : piece.cellFields;
    const auto entities = writesPointData ? numberOfPoints : numberOfCells;
    fields.push_back({field.name, field.components, std::vector<double>(entities * field.components)});
  }
  piece.cellFields.push_back({"RefinementLevel", 1, std::vector<double>(numberOfCells)});
  piece.cellFields.push_back({"Subdomain", 1, std::vector<double>(numberOfCells, static_cast<double>(rank))});
  piece.cellFields.push_back({"CurveIndex", 1, std::vector<double>(numberOfCells)});
  if (writesStatus) {
    piece.cellFields.push_back({"LimiterStatus", 1, std::vector<double>(numberOfCells)});
  }
  return pieceId;
}
