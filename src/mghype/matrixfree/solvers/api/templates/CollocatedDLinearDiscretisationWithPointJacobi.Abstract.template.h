#pragma once

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

class {{NAMESPACE | join("::")}}::{{CLASSNAME}}: public ::mghype::matrixfree::solvers::Solver {
  public:
    static constexpr double     MinH = {{MIN_H}};
    static constexpr double     MaxH = {{MAX_H}};

    static constexpr double     Omega          = {{SMOOTHER_RELAXATION}};
    static constexpr int        VertexUnknowns = {{VERTEX_CARDINALITY}};

    enum class State {
      Solve,
      Suspend
    };

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
    ) {%if CUSTOM_INIT_VERTEX=="" %} = 0 {% endif %};

    virtual void setBoundaryConditions(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h,
      tarch::la::Vector< {{VERTEX_CARDINALITY}}, double >&  value
    ) {%if CUSTOM_BOUNDARY_CONDITIONS=="" %} = 0 {% endif %};

    virtual vertexdata::{{SOLVER_NAME}}::Type getVertexType(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h
    );

    virtual celldata::{{SOLVER_NAME}}::Type getCellType(
      const tarch::la::Vector<Dimensions, double>&  x,
      const tarch::la::Vector<Dimensions, double>&  h
    );

    virtual tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > getLocalAssemblyMatrix(
      const tarch::la::Vector<Dimensions, double>&  cellCentre,
      const tarch::la::Vector<Dimensions, double>&  cellSize
    ) {% if LOCAL_ASSEMBLY_MATRIX is not defined %} = 0 {% endif %};

    virtual tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > getRhsMatrix(
      const tarch::la::Vector<Dimensions, double>&  cellCentre,
      const tarch::la::Vector<Dimensions, double>&  cellSize
    ) {% if MASS_MATRIX is not defined  %} = 0 {% endif %};

    virtual void beginMeshSweep() override;
    virtual void endMeshSweep() override;

    /**
     * Suspend solver for one mesh sweep
     */
    void suspend();

    bool isSuspended() const;

    bool updateSolution() const;
    bool updateResidual() const;

    State getState() const;
    {{ABSTRACT_SOLVER_HEADER_EXTRAS}}
    
  protected:
    static tarch::logging::Log  _log;

    State  _state;

    /**
     * We need to allow the residual and diagonal values to
     * synchonise across parallel boundaries before updating
     * the solution. To prevent 0-divison error, we can't
     * update the solution on the first 
     */
    bool _updateSolution;
};

