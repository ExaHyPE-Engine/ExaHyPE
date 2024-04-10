#ifndef EXASEIS_CARTESIAN_MESH_HEADER
#define EXASEIS_CARTESIAN_MESH_HEADER

#include "Mesh.h"
#include "../../../ExaHyPE/kernels/GaussLegendreBasis.h"
#include "../../../../ExaHyPE/kernels/KernelUtils.h"

template <class Shortcuts, int basisSize>
class CurvilinearMeshAnalytic: public Mesh<Shortcuts, basisSize> {

public:
  CurvilinearMeshAnalytic(Topography* topography) { tranformation = new CurvilinearTransformation(topography); }

private:
  CurvilinearTransormation transformation;
};

#endif
