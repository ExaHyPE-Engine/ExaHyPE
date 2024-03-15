// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexMarker.h"
#include "swift2/kernels/ParticleUpdatePredicates.h"


namespace swift2 {
  namespace kernels {
    /**
     * @namespace swift2::kernels::legacy Legacy SPH implementation
     *
     * This implementation is very very close to the original Swift kernels.
     * This can immediately be seen from the signature of the routines: The
     * core routines all work solely with pointers.
     *
     * For Swift 2 however, we need signatures which work with references and
     * also evaluate predicates. So each routine actually comes along as pair:
     * The core science with a pointer and then a wrapper around it.
     *
     * The wrapper vs implementation distinction also is reflected in the
     * naming conventions: The actual physics remain written in C/Swift
     * style, whereas the wrappers follow the Peano's C++ convention.
     *
     * This file contains only the functions that we actually want to use from
     * the original swift codebase, and maintains their original name. The idea
     * is to eventually use the SWIFT implementation of these functions. But for
     * now, we keep a (modified) version of them here.
     *
     * A second point to keep these functions separate is the fact that they
     * do different things for each SPH flavour. So they are intended to be
     * replaceable anyway.
     *
     * There are some notable exceptions which are missing from this collection
     * of functions. Firstly, we keep the particle-particle interactions
     * separately. We are tinkering with them in Peano and optimizing them in a
     * different way. They are defined in ParticleParticleInteraction.h.
     * Secondly, anything related to the computation of the smoothing lengths is
     * defined in SmoothingLengthComputation.h. The smoothing length computation
     * is intimately tied to the neighbour search algorithms, and cannot be
     * separated trivially from the underlying codebase and infrastructure. As
     * such, we can't simply re-use what original SWIFT does, but need to
     * provide our own algorithms. In particular, we differentiate between the
     * search radius and the interaction radius/ compact support radius of
     * particles, which original swift doesn't. We provide smoothing length
     * computation algorithms for both a constant search radius as well as a
     * varying search radius.
     */
    namespace legacy {} // namespace legacy
  }                     // namespace kernels
} // namespace swift2


#include "Swift.cpph"
