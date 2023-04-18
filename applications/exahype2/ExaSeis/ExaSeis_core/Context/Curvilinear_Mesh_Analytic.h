#ifndef EXASEIS_CARTESIAN_MESH_HEADER
#define EXASEIS_CARTESIAN_MESH_HEADER


#include "Mesh.h"
#include "exahype2/aderdg/kernels/Basis/GaussLegendreBasis.h"
#include "exahype2/aderdg/kernels/Utils/KernelUtils.h"

template<class Shortcuts, int basisSize>
class CurvilinearMeshAnalytic : public Mesh<Shortcuts,basisSize>{

public:

  CurvilinearMeshAnalytic(Topography* topography){
    tranformation = new CurvilinearTransformation(topography);
  }


 private:
  CurvilinearTransormation transformation;
};

#endif
