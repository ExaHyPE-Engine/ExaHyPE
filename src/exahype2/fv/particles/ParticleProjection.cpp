#include "ParticleProjection.h"

void exahype2::fv::particles::projection::depositMass_PiecewiseConstant(
  const peano4::datamanagement::CellMarker& marker,
  int                                       voxelsPerAxis,
  int                                       unknownsPerVoxel,
  double* __restrict__ Q,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  double                                       mass,
  int                                          unknownIndex
) {
  std::cout << "[depositMass_PiecewiseConstant] Called with:\n";
  std::cout << "  particleX = " << particleX << "\n";
  std::cout << "  mass = " << mass << ", unknownIndex = " << unknownIndex << "\n";
  std::cout << "  marker offset = " << marker.getOffset() << ", h = " << marker.h() << "\n";

  // Map particle to its voxel
  tarch::la::Vector<DIMENSIONS, int>
    voxel = exahype2::fv::particles::interpolation::internal::mapParticleOntoVoxel(marker, voxelsPerAxis, particleX);
  std::cout << "  mapped voxel = " << voxel << "\n";

  int voxelIndex = peano4::utils::dLinearised(voxel, voxelsPerAxis);
  std::cout << "  voxelIndex = " << voxelIndex << "\n";

  int index = voxelIndex * unknownsPerVoxel + unknownIndex;
  std::cout
    << "  full Q index = " << index << " (total size hint: " << (voxelsPerAxis * voxelsPerAxis * unknownsPerVoxel)
    << ")\n";

  // Compute voxel volume
  double h = marker.h()(0) / voxelsPerAxis;
  std::cout << "  h (voxel length) = " << h << "\n";

  double volume = 1.0;
  for (int d = 0; d < DIMENSIONS; ++d) {
    volume *= h;
  }
  std::cout << "  voxel volume = " << volume << "\n";

  // Deposit density (mass/volume)
  assertion1(index < voxelsPerAxis * voxelsPerAxis * unknownsPerVoxel, index);
  double deltaDensity = mass / volume;
  std::cout << "  deltaDensity = " << deltaDensity << "\n";

  Q[index] += deltaDensity;
  std::cout << "  Q[" << index << "] after deposit = " << Q[index] << "\n";
}