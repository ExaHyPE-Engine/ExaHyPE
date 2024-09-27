#pragma once

#include <string>
#include "tarch/la/Vector.h"
#include "tarch/la/Matrix.h"
#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/accelerator/accelerator.h"

#include "peano4/utils/Globals.h"

#include "Constants.h"
#include "vertexdata/{{SOLVER_NAME}}.h"
#include "celldata/{{SOLVER_NAME}}.h"
#include "facedata/{{SOLVER_NAME}}.h"

#include "mghype/matrixfree/solvers/Solver.h"

{{SOLVER_INCLUDES}}

{% for item in NAMESPACE -%}
  namespace {{ item }} {

{%- endfor %}
  class {{CLASSNAME}};

{% for item in NAMESPACE -%}
  }
{%- endfor %}

class {{NAMESPACE | join("::")}}::{{CLASSNAME}}: public mghype::matrixfree::solvers::Solver {
  public:
    static constexpr double     MinH = {{MIN_H}};
    static constexpr double     MaxH = {{MAX_H}};

    static constexpr double     Omega          = {{SMOOTHER_RELAXATION}};
    static constexpr int        VertexUnknowns = {{VERTEX_CARDINALITY}};
    
    static constexpr int        PreSmoothingSteps  = {{PRE_SMOOTHING_STEPS}};
    static constexpr int        PostSmoothingSteps = {{POST_SMOOTHING_STEPS}};

    enum class State {
      Solve,
      Suspend
    };

    enum class SmoothingState {
      // FirstFineGridSmoothing,
      PreSmoothing,
      Restrict,
      PostSmoothing
    };

    SmoothingState getSmoothingState();

    using mghype::matrixfree::solvers::Solver::toString;

    std::string toString(SmoothingState state) const;

    {{CLASSNAME}}();
    virtual ~{{CLASSNAME}}();

    /**
     * Initialise a vertex degree of freedom
     */
    virtual void initVertex(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h,
      tarch::la::Vector< {{VERTEX_CARDINALITY}}, double >&  value,
      tarch::la::Vector< {{VERTEX_CARDINALITY}}, double >&  rhs
    ) = 0;

    virtual void setBoundaryConditions(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h,
      tarch::la::Vector< {{VERTEX_CARDINALITY}}, double >&  value
    ) = 0;

    virtual vertexdata::{{SOLVER_NAME}}::Type getVertexDoFType(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h
    ) const final;

    virtual celldata::{{SOLVER_NAME}}::Type getCellDoFType(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h
    ) const;

    virtual tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > getLocalAssemblyMatrix(
      const tarch::la::Vector<Dimensions, double>&  cellCentre,
      const tarch::la::Vector<Dimensions, double>&  cellSize
    ) const {% if LOCAL_ASSEMBLY_MATRIX is not defined %} = 0 {% endif %};


    virtual tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > getMassMatrix(
      const tarch::la::Vector<Dimensions, double>&  cellCentre,
      const tarch::la::Vector<Dimensions, double>&  cellSize,
      const int level
    ) const {% if MASS_MATRIX is not defined  %} = 0 {% endif %};

    virtual tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*FourPowerD, double > getRestrictionMatrix(
      const tarch::la::Vector<Dimensions, double>&  cellCentre,
      const tarch::la::Vector<Dimensions, double>&  cellSize
    ) const {% if RESTRICTION_MATRIX is not defined  %} = 0 {% endif %};

    virtual tarch::la::Matrix< {{VERTEX_CARDINALITY}}*FourPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > getProlongationMatrix(
      const tarch::la::Vector<Dimensions, double>&  cellCentre,
      const tarch::la::Vector<Dimensions, double>&  cellSize
    ) const {% if PROLONGATION_MATRIX is not defined  %} = 0 {% endif %};
    
    virtual void beginMeshSweep() override;
    virtual void endMeshSweep() override;

    /**
     * Suspend solver for one mesh sweep
     */
    void suspend();

    bool update(int level) const;

    int  getActiveLevel() const;
    void setActiveLevel(int);
    int  getFinestLevel() const;
    void setFinestLevel(int);

    /**
     * For now, don't check the level in the function
     * since we expect to do something on the level above
     * the active level as well as the active level.
    */
    bool restrict() const;
    bool prolongate() const;

  protected:
    static tarch::logging::Log  _log;

    State           _state;
    SmoothingState  _smoothingState;

    int _activeLevel;
    int _finestLevel;

    int _preSmoothingStepsCounter;
    int _postSmoothingStepsCounter;

}; // {{CLASSNAME}}