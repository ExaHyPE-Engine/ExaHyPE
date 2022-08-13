// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_POSTPROCESSING_KERNELS_H_
#define _EXAHYPE2_FV_POSTPROCESSING_KERNELS_H_


#include <vector>

#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"

#include "peano4/utils/Globals.h"


namespace exahype2 {
  namespace fv {
    /**
     * Simple extrapolation within patch
     *
     * The routine accepts a NxNxN patch. It runs over the layer
     *
     * ## Usage
     *
     * A typical usage first adds the header to the solver, so the present
     * postprocessing routines are known to the compiler:
     *
     *       thesolver.add_user_action_set_includes( """
     *         #include "exahype2/fv/PostprocessingKernels.h"
     *       """)
     *
     * After that, it adds the actual postprocessing call. In the example
     * below, we take the first unknown (index 0) and map it onto the first
     * auxiliary variable (index {{NUMBER_OF_UNKNOWNS}}).
     *
     *       thesolver.postprocess_updated_patch = """
     *         ::exahype2::fv::mapInnerNeighbourVoxelAlongBoundayOntoAuxiliaryVariable(
     *           targetPatch,
     *           {{NUMBER_OF_VOLUMES_PER_AXIS}},
     *           {{NUMBER_OF_UNKNOWNS}},
     *           {{NUMBER_OF_AUXILIARY_VARIABLES}},
     *           0,
     *           {{NUMBER_OF_UNKNOWNS}}
     *         );
     *       """
     *
     *
     */
    void mapInnerNeighbourVoxelAlongBoundayOntoAuxiliaryVariable(
        double* __restrict__ Q,
        int    unknowns,
        int    auxiliaryVariables,
        int    numberOfVolumesPerAxisInPatch,
        int    fromIndex,
        int    toIndex
    );
  }
}

#endif
