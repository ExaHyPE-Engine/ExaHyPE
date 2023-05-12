// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/utils/Globals.h"


namespace toolbox {
  namespace blockstructured {
    /**
     * Project data from patch onto adjacent faces
     *
     * Used by exahype2.solvers.fv.ProjectPatchOntoFaces for example. The more
     * sophisticiated time-stepping schemes such as Runge-Kutta often have their
     * own projection realised via jinja2 in Python, as they not only project
     * some fixed data, but actually have to project linear combinations.
     * Furthermore, the solvers often set some helper variables (meta data) on
     * the faces, too.
     *
     * However, some manual projections (user preprocessing) can benefit
     * heavily from this routine, so I decided to deploy it to a C++ routine
     * of its own.
     *
     */
    void projectPatchSolutionOntoFaces(
      int    numberOfVolumesPerAxisInPatch,
      int    haloSize,
      int    unknowns,
      int    auxiliaryVariables,
      const double* __restrict__ Q,
      double* __restrict__ leftFace,
      double* __restrict__ bottomFace,
      double* __restrict__ rightFace,
      double* __restrict__ topFace,
      bool permuteInnerAndOuterSideOfFace = false
    );

    void projectPatchSolutionOntoFaces(
      int    numberOfVolumesPerAxisInPatch,
      int    haloSize,
      int    unknowns,
      int    auxiliaryVariables,
      const double* __restrict__ Q,
      double* __restrict__ leftFace,
      double* __restrict__ bottomFace,
      double* __restrict__ frontFace,
      double* __restrict__ rightFace,
      double* __restrict__ topFace,
      double* __restrict__ backFace,
      bool permuteInnerAndOuterSideOfFace = false
    );

    void projectPatchSolutionOntoFaces(
      int    numberOfVolumesPerAxisInPatch,
      int    haloSize,
      int    unknowns,
      int    auxiliaryVariables,
      const double* __restrict__ Q,
      double* faces[2*Dimensions],
      bool permuteInnerAndOuterSideOfFace = false
    );
  }
}


