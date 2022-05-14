// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAMPLES_INTEGER_DIFFUSION_THROUGH_FACES_MYMAPPING_H_
#define _EXAMPLES_INTEGER_DIFFUSION_THROUGH_FACES_MYMAPPING_H_


#include "Mapping.h"


#include "peano4/utils/Globals.h"
#include "tarch/la/Vector.h"
#include "tarch/logging/Log.h"


namespace examples {
  namespace integerdiffusionthroughfaces {
    class MyMapping;
  }
}


/**
 * Integer diffusion mapping
 *
 * \section Marker semantics
 *
 * - Marker k=K:   Cell holds valid data.
 * - Marker k=K-1: Remove cell if appropriate.
 * - Marker k<K-1: Type of boundary data.
 * - Marker k=K+1: Cell where all successors host or shall host K cells. Holds data.
 * - Marker k=K+2: Cell where all successors host or shall host K cells. Holds no data.
 *
 * K is typically set to something around 5. The bigger K the bigger the
 * diffusive area around the marker.
 *
 * The faces always hold the sum of the two adjacent cells. Therefore, we can
 * reconstruct neighbour values in every cell.
 *
 * The cell information is analysed
 * on-the-fly: We copy the marker over into a backup variable when we touch a
 * face for the first time, clear the face attribute, and then accumulate the
 * value again.
 *
 * \section Marker flow
 *
 * We basically solve an integer-valued equation similar to
 *
 * @f$
 * -\Delta k = -1
 * @f$
 *
 * on the grid, while we expect valid cells to overwrite their value to K in
 * each and every sweep. As these values are fixed, the k marker diffuses out
 * into the domain. Boundary faces need no special treatment as the right-hand
 * side brings down the k values automatically.
 *
 * The multiscale behaviour is slightly more delicate:
 *
 * - All markers propagate bottom-up.
 * - All markers decrease monotonically (yet not strict) top-down.
 *
 * @author Tobias Weinzierl
 */
class examples::integerdiffusionthroughfaces::MyMapping: public Mapping {
  private:
	static tarch::logging::Log   _log;
  public:

	void beginTraversal() final;
	void endTraversal() final;

	void createPersistentFace(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    ) override;

	void createHangingFace(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    ) override;

	void destroyPersistentFace(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    ) override;

	void destroyHangingFace(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    ) override;

	void touchFaceFirstTime(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    ) override;

	void touchFaceLastTime(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
      const tarch::la::Vector<Dimensions,double>&  normal,
	  FaceData&                                    data
    ) override;

	void createCell(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
	  CellData&                                    data,
	  Faces&                                       faces,
	  CellData&                                    coarseData,
	  Faces&                                       coarseFaces
    ) override;

	void destroyCell(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
	  CellData&                                    data,
	  Faces&                                       faces,
	  CellData&                                    coarseData,
	  Faces&                                       coarseFaces
    ) override;

	/**
	 * This routine is invoked as the pushback automaton running through the
	 * spacetree hits a cell for the first time. If a cell is brand new, this
	 * routine is invoked directly after the create call.
	 */
	void touchCellFirstTime(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
	  CellData&                                    data,
	  Faces&                                       faces,
	  CellData&                                    coarseData,
	  Faces&                                       coarseFaces
    ) override;

	void touchCellLastTime(
      const tarch::la::Vector<Dimensions,double>&  center,
      const tarch::la::Vector<Dimensions,double>&  h,
	  CellData&                                    data,
	  Faces&                                       faces,
	  CellData&                                    coarseData,
	  Faces&                                       coarseFaces
    ) override;
};

#endif
