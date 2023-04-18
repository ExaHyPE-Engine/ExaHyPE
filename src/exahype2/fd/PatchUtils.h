// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <vector>

#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/utils/Globals.h"

#include "exahype2/fv/PatchUtils.h"


namespace exahype2 {
  namespace fd {
    /**
     * @see exahype2::fv::getVolumeSize()
     */
    static tarch::la::Vector<2,double>  getGridCellSize(
      const tarch::la::Vector<2,double>&  h,
      int                                 numberOfGridCellsPerPatchPerAxis
    )
    //InlineMethod
    {
      return ::exahype2::fv::getVolumeSize(h,numberOfGridCellsPerPatchPerAxis);
    }


    static tarch::la::Vector<3,double>  getGridCellSize(
      const tarch::la::Vector<3,double>&  h,
      int                                 numberOfGridCellsPerPatchPerAxis
    )
    //InlineMethod
    {
      return ::exahype2::fv::getVolumeSize(h,numberOfGridCellsPerPatchPerAxis);
    }


    static tarch::la::Vector<2,double>  getGridFaceSize(
      const tarch::la::Vector<2,double>&  h,
      int                                 numberOfGridCellsPerPatchPerAxis
    )
    //InlineMethod
    {
      return ::exahype2::fv::getFaceSize(h,numberOfGridCellsPerPatchPerAxis);
    }


    static tarch::la::Vector<3,double>  getGridFaceSize(
      const tarch::la::Vector<3,double>&  h,
      int                                 numberOfGridCellsPerPatchPerAxis
    )
    //InlineMethod
    {
      return ::exahype2::fv::getFaceSize(h,numberOfGridCellsPerPatchPerAxis);
    }


    /**
     * In ExaHyPE's Finite Volume setup, a cell hosts a patch of Finite Volumes.
     * When we iterate over these volumes, we typically have to know the centre
     * of the volume.
     *
     * I use this routine in a lot of the loops that have to be vectorised.
     * Therefore, it is absolutely essential that the compiler can inline them.
     * Inlining works properly with most compilers if and only if the definition
     * is available in the header. This is the reason why this implementation
     * ended up here and not in the cpp file.
     *
     * @param x      Centre of the cell
     * @param h      Size of the cell
     * @param index  Index of Finite Volume (in lexicographic ordering)
     */
    static tarch::la::Vector<2,double>  getGridCellCentre(
      const tarch::la::Vector<2,double>&  x,
      const tarch::la::Vector<2,double>&  h,
      int                                 numberOfGridCellsPerPatchPerAxis,
      const tarch::la::Vector<2,int>&     index
    )
    //InlineMethod
    {
      return ::exahype2::fv::getVolumeCentre(x,h,numberOfGridCellsPerPatchPerAxis,index);
    }


    static tarch::la::Vector<3,double>  getGridCellCentre(
      const tarch::la::Vector<3,double>&  x,
      const tarch::la::Vector<3,double>&  h,
      int                                 numberOfGridCellsPerPatchPerAxis,
      const tarch::la::Vector<3,int>&     index
    )
    //InlineMethod
    {
      return ::exahype2::fv::getVolumeCentre(x,h,numberOfGridCellsPerPatchPerAxis,index);
    }


    static tarch::la::Vector<2,double>  getGridFaceCentre(
      const tarch::la::Vector<2,double>&  x,
      const tarch::la::Vector<2,double>&  h,
      int                                 numberOfGridCellsPerPatchPerAxis,
      int                                 overlap,
      int                                 normal,
      const tarch::la::Vector<2,int>&     index
    )
    //InlineMethod
    {
      return ::exahype2::fv::getFaceCentre(x,h,numberOfGridCellsPerPatchPerAxis,overlap,normal,index);
    }


    static tarch::la::Vector<3,double>  getGridFaceCentre(
      const tarch::la::Vector<3,double>&  x,
      const tarch::la::Vector<3,double>&  h,
      int                                 numberOfGridCellsPerPatchPerAxis,
      int                                 overlap,
      int                                 normal,
      const tarch::la::Vector<3,int>&     index
    )
    //InlineMethod
    {
      return ::exahype2::fv::getFaceCentre(x,h,numberOfGridCellsPerPatchPerAxis,overlap,normal,index);
    }


    /**
     * Helper routine that I need in the log statements. Applies to
     * AoS data only.
     *
     * @return Data of one volume as tuple.
     */
    std::string plotGridCell(
      const double* __restrict__ Q,
      int    unknowns
    );

    /**
     * Just runs over the patch and ensures that no entry is non or infinite. In
     * ExaHyPE, we primarily work with split approaches. That is, diagonal halo
     * entries are never initialised properly: We can copy over the face-connected
     * data, but we lack information through the diagonals. This routine takes
     * this into account when it validates the entries.
     *
     * Assumes all data are held as AoS.
     *
     * @param location String that tells system from where this routine got called
     * @param minValues Is either a nullptr or it points to a double array with
     *   exactly unknowns+auxiliaryVariables entries
     */
    void validatePatch(
      const double* __restrict__ Q,
      int    unknowns,
      int    auxiliaryVariables,
      int    numberOfGridCellsPerPatchPerAxis,
      int    haloSize,
      const std::string& location = "",
      bool   triggerNonCriticalAssertion = true,
      double* minValues = nullptr,
      double* maxValues = nullptr
    );

    /**
     * Plot patch.
     *
     * Usually used for debugging.
     *
     * Assumes all data are held as AoS.
     */
    std::string plotPatch(
      const double* __restrict__ Q,
      int    unknowns,
      int    auxiliaryVariables,
      int    numberOfGridCellsPerPatchPerAxis,
      int    haloSize,
      bool   prettyPrint = false
    );


    std::string plotPatchOverlap(
      const double* __restrict__ Q,
      int    unknowns,
      int    auxiliaryVariables,
      int    numberOfGridCellsPerPatchPerAxis,
      int    haloSize,
      int    normal,
      bool   prettyPrint = false
    );
  }
}

