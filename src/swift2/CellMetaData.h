// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <vector>


#include "peano4/datamanagement/CellMarker.h"


namespace swift2 {
  struct CellMetaData;
}


/**
 * Provide meta data around Swift 2 cell
 *
 * Lightweight wrapper around a couple of attributes which have to be passed
 * into particle (pair) update kernels. I started to add more and more
 * attributes. Over time, I got fed up with all the variants and therefore
 * introduce this lightweight wrapper around attributes. Due to this this one,
 * we can extend the attributes passed in at any point without having to touch
 * each individual piece of source code.
 */
struct swift2::CellMetaData {
  private:
    static std::vector<int> _dummyNumberOfCoalescedActiveParticlesPerVertex;
  public:
    const peano4::datamanagement::CellMarker&  marker;
    const std::vector<int>&                    numberOfCoalescedLocalParticlesPerVertex;
    const std::vector<int>&                    numberOfCoalescedActiveParticlesPerVertex;
    const std::vector<bool>&                   localParticleIsContainedInCell;

    CellMetaData() = delete;

    CellMetaData(
      const peano4::datamanagement::CellMarker&  marker,
      const std::vector<int>&                    numberOfCoalescedLocalParticlesPerVertex,
      const std::vector<int>&                    numberOfCoalescedActiveParticlesPerVertex,
      const std::vector<bool>&                   localParticleIsContainedInCell
    );

    CellMetaData(
      const peano4::datamanagement::CellMarker&  marker,
      const std::vector<bool>&                   localParticleIsContainedInCell
    );
};
