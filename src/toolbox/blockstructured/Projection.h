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
     * @param Q Patch data of size @f$ numberOfVolumesPerAxisInPatch^d \cdot (unknowns + auxiliaryVariables)@f$.
     *   That is, the pointer points to a patch without any halo data.
     * @param leftFace Pointer to face data, i.e. a field of size
     *   @f$ numberOfVolumesPerAxisInPatch^{d-1} \cdot 2 \cdot haloSize \cdot (unknowns + auxiliaryVariables)@f$
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
      double* __restrict__ topFace
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
      double* __restrict__ backFace
    );

    void projectPatchSolutionOntoFaces(
      int    numberOfVolumesPerAxisInPatch,
      int    haloSize,
      int    unknowns,
      int    auxiliaryVariables,
      const double* __restrict__ Q,
      double* faces[2*Dimensions]
    );

    /**
     * Take elements from the halo and project them onto the face
     *
     * This routine is the cousin of projectPatchSolutionOntoFaces() but
     * assumes that Q does not point to the patch only but to the patch
     * including a halo of size haloSize. It now takes this halo and
     * projects it onto the face.
     *
     * When projectPatchSolutionOntoFaces() maps its data onto the face,
     * it writes these data copies onto the interior parts of the face.
     * In ASCII art, this resembles
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *      | a | b | c | d |
     *  | x | y |
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * The patch with four entries (a,b,c,d) takes its leftmost entry a and
     * writes it into y on the left face. This example is for a halo size of
     * 1. This is projectPatchSolutionOntoFaces().
     *
     * The present routine assumes that we have the patch plus its halo and
     * therefore does
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *  | h | a | b | c | d | i |
     *  | x | y |
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * project the halo entry (h) onto the face at point x.
     *
     *
     * @param Q Patch data with halo, i.e.
     */
    void projectPatchHaloOntoFaces(
      int    numberOfVolumesPerAxisInPatch,
      int    haloSize,
      int    unknowns,
      int    auxiliaryVariables,
      const double* __restrict__ Q,
      double* __restrict__ leftFace,
      double* __restrict__ bottomFace,
      double* __restrict__ rightFace,
      double* __restrict__ topFace
    );

    void projectPatchHaloOntoFaces(
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
      double* __restrict__ backFace
    );

    void projectPatchHaloOntoFaces(
      int    numberOfVolumesPerAxisInPatch,
      int    haloSize,
      int    unknowns,
      int    auxiliaryVariables,
      const double* __restrict__ Q,
      double* faces[2*Dimensions]
    );

    void extrapolatePatchSolutionAndProjectExtrapolatedHaloOntoFaces(
      int    numberOfVolumesPerAxisInPatch,
      int    haloSize,
      int    unknowns,
      int    auxiliaryVariables,
      const double* __restrict__ Q,
      double* __restrict__ leftFace,
      double* __restrict__ bottomFace,
      double* __restrict__ rightFace,
      double* __restrict__ topFace
    );

    void extrapolatePatchSolutionAndProjectExtrapolatedHaloOntoFaces(
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
      double* __restrict__ backFace
    );

    void extrapolatePatchSolutionAndProjectExtrapolatedHaloOntoFaces(
      int    numberOfVolumesPerAxisInPatch,
      int    haloSize,
      int    unknowns,
      int    auxiliaryVariables,
      const double* __restrict__ Q,
      double* faces[2*Dimensions]
    );
  }
}


