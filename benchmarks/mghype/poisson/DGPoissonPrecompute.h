#pragma once


#include "AbstractDGPoissonPrecompute.h"
#include <mutex>

namespace benchmarks {namespace mghype {namespace poisson {
  class DGPoissonPrecompute;

}}}


class benchmarks::mghype::poisson::DGPoissonPrecompute: public benchmarks::mghype::poisson::AbstractDGPoissonPrecompute {
  public:

    /**
     * Default constructor
     *
     * @todo Please add your documentation here.
     */
    DGPoissonPrecompute();

    /**
     * Destructor
     *
     * Has to be virtual, as there is a superclass with virtual functions.
     *
     * @todo Please add your documentation here.
     */
    virtual ~DGPoissonPrecompute();

    
    virtual void initCellDoF(
        const tarch::la::Vector<Dimensions, double>&  x,
        const tarch::la::Vector<Dimensions, double>&  h,
        tarch::la::Vector< UnknownsPerCellNode, double >&    value,
        tarch::la::Vector< UnknownsPerCellNode, double >&    rhs,
        tarch::la::Vector< Dimensions, int >&         node
    ) override;

    virtual tarch::la::Matrix< benchmarks::mghype::poisson::AbstractDGPoissonPrecompute::CellUnknowns, benchmarks::mghype::poisson::AbstractDGPoissonPrecompute::CellUnknowns, double > getInvertedApproxSystemMatrix(
      const tarch::la::Vector<Dimensions, double>&  cellCentre,
      const tarch::la::Vector<Dimensions, double>&  cellSize
    ) override;
};
