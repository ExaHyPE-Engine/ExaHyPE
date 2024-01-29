# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

import jinja2

def create_finish_time_step_implementation_for_fv_limiting():
    return """
  	if ( _solverState==SolverState::Correction ) {
        #ifdef Parallel
        int newNumberOfTroubledCells = _numberOfTroubledCellsThisTimeStep;

        tarch::mpi::Rank::getInstance().allReduce(
            &newNumberOfTroubledCells,
            &_numberOfTroubledCellsThisTimeStep,
            1, MPI_INT,
            MPI_SUM,
            [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
        );
        #endif
    }
"""

def create_abstract_solver_declarations_for_fv_limiting(number_of_dmp_observables, dmp_relaxation_parameter, dmp_differences_scaling):
    Template = jinja2.Template("""
    public:
        static constexpr int NumberOfDMPObservables    = {{NUMBER_OF_DMP_OBSERVABLES}};
        static constexpr double RelaxationParameter    = {{DMP_RELAXATION_PARAMETER}};
        static constexpr double DifferencesScaling     = {{DMP_DIFFERENCES_SCALING}};

        /**
        * Inform the solver about a cell being troubled, meaning its value
        * needs to be recomputed and the solver cannot move on to the next
        * timestep until this is done.
        */
        void addTroubledCell();
        int getNumberOfTroubledCells() const;

        /**
        * Default implementation. Please overwrite.
        *
        * See superclass for function's semantics.
        */
        void mapDiscreteMaximumPrincipleObservables(double* const observables, const double* const Q) const {
          if (NumberOfDMPObservables>0) {
            std::copy_n(Q,NumberOfDMPObservables,observables);
          }
        }

        /**
        * Default implementation. Please overwrite.
        *
        * See superclass for function's semantics.
        */
        double getDiscreteMaximumPrincipleRelaxationParameter(
            const double& specifiedRelaxationParameter,
            const int&    observable,
            const double& localMin,
            const double& localMax,
            const double& boundaryMinPerObservable,
            const double& previousMax) const {
          return specifiedRelaxationParameter;
        }
    """,
        undefined=jinja2.DebugUndefined,
    )

    d = {}
    d["NUMBER_OF_DMP_OBSERVABLES"] = number_of_dmp_observables
    d["DMP_RELAXATION_PARAMETER"]  = dmp_relaxation_parameter
    d["DMP_DIFFERENCES_SCALING"]   = dmp_differences_scaling
    return Template.render(**d)

def create_abstract_solver_definitions_for_fv_limiting():
    return """
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::addTroubledCell(){
  _numberOfTroubledCellsThisTimeStep += 1;
}
int {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::getNumberOfTroubledCells() const {
  return _numberOfTroubledCellsThisTimeStep;  
}
"""

def create_abstract_solver_user_declarations_for_fv_limiting(physical_admissibility_implementation):
    Template = jinja2.Template("""
    public:

{% if ADMISSIBILITY_IMPLEMENTATION!="<none>" %}
    virtual bool isPhysicallyAdmissible(
        const double* const                         solution,
        const tarch::la::Vector<Dimensions,double>& x,
        const double timeStamp) {% if ADMISSIBILITY_IMPLEMENTATION=="<user-defined>" %} = 0 {% else %} final {% endif %};
{% endif %}
    """,
        undefined=jinja2.DebugUndefined,
    )

    d = {}
    d["ADMISSIBILITY_IMPLEMENTATION"] = physical_admissibility_implementation
    return Template.render(**d)



def create_abstract_solver_user_definitions_for_fv_limiting(physical_admissibility_implementation):
    Template = jinja2.Template("""
{% if ADMISSIBILITY_IMPLEMENTATION!="<empty>" and ADMISSIBILITY_IMPLEMENTATION!="<user-defined>" and ADMISSIBILITY_IMPLEMENTATION!="<none>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
bool isPhysicallyAdmissible(
  const double* const                         solution,
  const tarch::la::Vector<Dimensions,double>& x,
  const double timeStamp
){
  {{ADMISSIBILITY_IMPLEMENTATION}}
}
{% endif %}
    """,
        undefined=jinja2.DebugUndefined,
    )

    d = {}
    d["ADMISSIBILITY_IMPLEMENTATION"] = physical_admissibility_implementation
    return Template.render(**d)

def create_solver_user_declarations_for_fv_limiting(physical_admissibility_implementation):
    Template = jinja2.Template("""
{% if ADMISSIBILITY_IMPLEMENTATION=="<user-defined>" %}
  bool isPhysicallyAdmissible(
    const double* const                         solution,
    const tarch::la::Vector<Dimensions,double>& x,
    const double timeStamp) override;
{% endif %}
    """,
        undefined=jinja2.DebugUndefined,
    )

    d = {}
    d["ADMISSIBILITY_IMPLEMENTATION"] = physical_admissibility_implementation
    return Template.render(**d)

def create_solver_user_definitions_for_fv_limiting(physical_admissibility_implementation):
    Template = jinja2.Template("""
{% if ADMISSIBILITY_IMPLEMENTATION=="<user-defined>" %}
bool isPhysicallyAdmissible(
  const double* const                         solution,
  const tarch::la::Vector<Dimensions,double>& x,
  const double timeStamp
){
  return true;
}
{% endif %}
    """,
        undefined=jinja2.DebugUndefined,
    )

    d = {}
    d["ADMISSIBILITY_IMPLEMENTATION"] = physical_admissibility_implementation
    return Template.render(**d)


def create_start_time_step_implementation_for_fv_limiting():
    predicate = """
    tarch::mpi::Rank::getInstance().isGlobalMaster() 
    and
    _maxCellH>0.0
  """

    statistics = (
        """
  if ("""
        + predicate
        + """) {
    logInfo( "startTimeStep()", "number of troubled cells = " << _numberOfTroubledCellsThisTimeStep );
  }
"""
    )

    return statistics
