// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <vector>

#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/utils/Globals.h"


namespace exahype2 {
  namespace fv {
    /**
     * We need this routine within vectorised and GPUised code. Therefore,
     * it has to be in the header (as link-time optimisation is quite tricky
     * for many compilers). To avoid the implied multiple symbol errors, I
     * declare the function as static. Now the compilers plot warning, but
     * everything seems to work.
     */
    static tarch::la::Vector<2,double>  getVolumeSize(
      const tarch::la::Vector<2,double>&  h,
      int                                 numberOfVolumesPerAxisInPatch
    )
    //InlineMethod
    {
      tarch::la::Vector<2,double> volumeSize;

      for (int d=0; d<2; d++) {
        #if !defined(GPUOffloadingSYCL) and !defined(GPUOffloadingOMP)
        assertion2( numberOfVolumesPerAxisInPatch>=1, h, numberOfVolumesPerAxisInPatch );
        #endif
        volumeSize(d) = h(d)/numberOfVolumesPerAxisInPatch;
      }

      return volumeSize;
    }


    static tarch::la::Vector<3,double>  getVolumeSize(
      const tarch::la::Vector<3,double>&  h,
      int                                 numberOfVolumesPerAxisInPatch
    )
    //InlineMethod
    {
      tarch::la::Vector<3,double> volumeSize;

      for (int d=0; d<3; d++) {
        volumeSize(d) = h(d)/numberOfVolumesPerAxisInPatch;
      }

      return volumeSize;
    }


    static tarch::la::Vector<2,double>  getFaceSize(
      const tarch::la::Vector<2,double>&  h,
      int                                 numberOfVolumesPerAxisInPatch
    )
    //InlineMethod
    {
      return getVolumeSize(h,numberOfVolumesPerAxisInPatch);
    }


    static tarch::la::Vector<3,double>  getFaceSize(
      const tarch::la::Vector<3,double>&  h,
      int                                 numberOfVolumesPerAxisInPatch
    )
    //InlineMethod
    {
      return getVolumeSize(h,numberOfVolumesPerAxisInPatch);
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
    static tarch::la::Vector<2,double>  getVolumeCentre(
      const tarch::la::Vector<2,double>&  x,
      const tarch::la::Vector<2,double>&  h,
      int                                 numberOfVolumesPerAxisInPatch,
      const tarch::la::Vector<2,int>&     index
    )
    //InlineMethod
    {
      tarch::la::Vector<2,double> volumeSize = getVolumeSize(h,numberOfVolumesPerAxisInPatch);
      tarch::la::Vector<2,double> result     = x - 0.5 * h + 0.5 * volumeSize;
      for (int d=0; d<2; d++) {
        result(d) += index(d) * volumeSize(d);
      }
      return result;
    }


    static tarch::la::Vector<3,double>  getVolumeCentre(
      const tarch::la::Vector<3,double>&  x,
      const tarch::la::Vector<3,double>&  h,
      int                                 numberOfVolumesPerAxisInPatch,
      const tarch::la::Vector<3,int>&     index
    )
    //InlineMethod
    {
      tarch::la::Vector<3,double> volumeSize = getVolumeSize(h,numberOfVolumesPerAxisInPatch);
      tarch::la::Vector<3,double> result     = x - 0.5 * h + 0.5 * volumeSize;
      for (int d=0; d<3; d++) {
        result(d) += index(d) * volumeSize(d);
      }
      return result;
    }


    static tarch::la::Vector<2,double>  getFaceCentre(
      const tarch::la::Vector<2,double>&  x,
      const tarch::la::Vector<2,double>&  h,
      int                                 numberOfVolumesPerAxisInPatch,
      int                                 overlap,
      int                                 normal,
      const tarch::la::Vector<2,int>&     index
    )
    //InlineMethod
    {
      tarch::la::Vector<2,double> volumeSize = getFaceSize(h,numberOfVolumesPerAxisInPatch);
      tarch::la::Vector<2,double> result;

      for (int d=0; d<2; d++) {
        if (d==normal) {
          result(d) = x(d) - overlap * volumeSize(d) + 0.5 * volumeSize(d);
        }
        else {
          result(d) = x(d) - 0.5 * h(d) + 0.5 * volumeSize(d);
        }
        result(d) += index(d) * volumeSize(d);
      }

      return result;
    }


    static tarch::la::Vector<3,double>  getFaceCentre(
      const tarch::la::Vector<3,double>&  x,
      const tarch::la::Vector<3,double>&  h,
      int                                 numberOfVolumesPerAxisInPatch,
      int                                 overlap,
      int                                 normal,
      const tarch::la::Vector<3,int>&     index
    )
    //InlineMethod
    {
      tarch::la::Vector<3,double> volumeSize = getVolumeSize(h,numberOfVolumesPerAxisInPatch);
      tarch::la::Vector<3,double> result     = x - 0.5 * h + 0.5 * volumeSize;

      for (int d=0; d<3; d++) {
        if (d==normal) {
          result(d) = x(d) - overlap * volumeSize(d) + 0.5 * volumeSize(d);
        }
        else {
          result(d) = x(d) - 0.5 * h(d) + 0.5 * volumeSize(d);
        }
        result(d) += index(d) * volumeSize(d);
      }

      return result;
    }


    /**
     * With GCC 10, it was impossible to return/copy the vector class. We
     * almost never need it however, as we work with cubes. This specialisation
     * thus does the job.
     *
     * @see getVolumeSize()
     */
    double  getVolumeLength(
      const tarch::la::Vector<2,double>&  h,
      int                                 numberOfVolumesPerAxisInPatch
    );


    double  getVolumeLength(
      const tarch::la::Vector<3,double>&  h,
      int                                 numberOfVolumesPerAxisInPatch
    );


    /**
     * Helper routine that I need in the log statements. Applies to
     * AoS data only.
     *
     * @return Data of one volume as tuple.
     */
    std::string plotVolume(
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
      int    numberOfVolumesPerAxisInPatch,
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
      int    numberOfVolumesPerAxisInPatch,
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


    /**
     * A face always holds a left and a right overlap
     *
     * This routine copies over half of the overlap
     *
     * @param isRightLayer
     */
    void copyHalfOfHalo(
      int    unknownsPlusAuxiliaryVariables,
      int    numberOfGridCellsPerPatchPerAxis,
      int    haloSize,  // same as overlap
      int    normal,
      bool   isRightLayer,
      const double* __restrict__   srcQ,
      double* __restrict__         destQ
    );
  }
}

