# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from enum import Enum

from exahype2.solvers.LagrangeBasisWithDiagonalMassMatrix import GaussLegendreBasis


class SolverVariant(Enum):
    WithVirtualFunctions = 0
    Stateless = 1
    Accelerator = 2


def create_abstract_solver_declarations(
    flux_implementation,
    ncp_implementation,
    eigenvalues_implementation,
    source_term_implementation,
    point_source_implementation,
    pde_terms_without_state,
):
    Template = jinja2.Template(
        """
  public:
    {% if EIGENVALUES_IMPLEMENTATION=="<none>" or EIGENVALUES_IMPLEMENTATION=="<empty>" %}
    #error The eigenvalue implementation must not be <none> or <empty>.
    {% endif %}

    {% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    static double maxEigenvalue(
      const double* NOALIAS                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      double                                       t,
      double                                       dt,
      int                                          normal,
      [[maybe_unused]] Offloadable                 offloadable
    );
    {% endif %}

    /**
     * Returns the maximum absolute eigenvalue of the Jacobian in the point with
     * the solution values Q. All parameters are in.
     */
    virtual double maxEigenvalue(
      const double* NOALIAS                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      double                                       t,
      double                                       dt,
      int                                          normal
    ) {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final{% endif %};

    {% if FLUX_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    static void flux(
      const double* NOALIAS                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* NOALIAS                              F, // F[{{NUMBER_OF_UNKNOWNS}}]
      [[maybe_unused]] Offloadable                 offloadable
    );
    {% endif %}

    {% if FLUX_IMPLEMENTATION!="<none>" %}
    virtual void flux(
      const double* NOALIAS                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* NOALIAS                              F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if FLUX_IMPLEMENTATION=="<user-defined>" %}=0{% else %} final {% endif %};
    {% endif %}

    {% if NCP_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    static void nonconservativeProduct(
      const double* NOALIAS                        Q,    // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double* NOALIAS                        deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* NOALIAS                              BTimesDeltaQ, // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
      [[maybe_unused]] Offloadable                 offloadable
    );
    {% endif %}

    {% if NCP_IMPLEMENTATION!="<none>" %}
    virtual void nonconservativeProduct(
      const double* NOALIAS                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double* NOALIAS                        deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* NOALIAS                              BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if NCP_IMPLEMENTATION=="<user-defined>" %}=0{% endif %};
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    static void sourceTerm(
      const double* NOALIAS                        Q,
      const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      double                                       t,
      double                                       dt,
      double* NOALIAS                              S,
      [[maybe_unused]] Offloadable                 offloadable
    );
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    virtual void sourceTerm(
      const double* NOALIAS                        Q,
      const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      double                                       t,
      double                                       dt,
      double* NOALIAS                              S
    ) {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final {% endif %};
    {% endif %}

    {% if POINT_SOURCE_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    void pointSource(
      const double* NOALIAS                        Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      double                                       t,
      double                                       dt,
      double* NOALIAS                              S,
      [[maybe_unused]] Offloadable                 offloadable
    );
    {% endif %}

    {% if POINT_SOURCE_IMPLEMENTATION!="<none>" %}
    virtual void pointSource(
      const double* NOALIAS                        Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      double                                       t,
      double                                       dt,
      double* NOALIAS                              S
    ) {% if POINT_SOURCE_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final {% endif %};
    {% endif %}
""",
        undefined=jinja2.DebugUndefined,
    )

    d = {}
    d["FLUX_IMPLEMENTATION"] = flux_implementation
    d["NCP_IMPLEMENTATION"] = ncp_implementation
    d["EIGENVALUES_IMPLEMENTATION"] = eigenvalues_implementation
    d["SOURCE_TERM_IMPLEMENTATION"] = source_term_implementation
    d["POINT_SOURCE_IMPLEMENTATION"] = point_source_implementation
    d["STATELESS_PDE_TERMS"] = pde_terms_without_state
    return Template.render(**d)


def create_abstract_solver_definitions(
    flux_implementation,
    ncp_implementation,
    eigenvalues_implementation,
    source_term_implementation,
    point_source_implementation,
    pde_terms_without_state,
):
    Template = jinja2.Template(
        """
{% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and EIGENVALUES_IMPLEMENTATION!="<none>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const double* NOALIAS                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  double                                       t,
  double                                       dt,
  int                                          normal
) {
  {{EIGENVALUES_IMPLEMENTATION}}
}
{% endif %}

{% if FLUX_IMPLEMENTATION!="<none>" and FLUX_IMPLEMENTATION!="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  const double* NOALIAS                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* NOALIAS                              F // F[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{FLUX_IMPLEMENTATION}}
}
{% endif %}

{% if NCP_IMPLEMENTATION!="<none>" and NCP_IMPLEMENTATION!="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  const double* NOALIAS                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const double* NOALIAS                        deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* NOALIAS                              BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{NCP_IMPLEMENTATION}}
}
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  const double* NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
  double                                       t,
  double                                       dt,
  double* NOALIAS                              S
) {
  {% if SOURCE_TERM_IMPLEMENTATION!="<empty>" %}
  {{SOURCE_TERM_IMPLEMENTATION}}
  {% else %}
  std::fill_n(S,{{NUMBER_OF_UNKNOWNS}},0.0);
  {% endif %}
}
{% endif %}

{% if POINT_SOURCE_IMPLEMENTATION!="<user-defined>" and POINT_SOURCE_IMPLEMENTATION!="<none>" %}
#error A Runge-Kutta discontinuous Galerkin solver does not support point sources.
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::pointSource(
  const double* NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  double                                       t,
  double                                       dt,
  double* NOALIAS                              S
) {
  {% if POINT_SOURCE_IMPLEMENTATION!="<empty>" %}
  {{POINT_SOURCE_IMPLEMENTATION}}
  {% else %}
  std::fill_n(S,{{NUMBER_OF_UNKNOWNS}},0.0);
  {% endif %}
}
{% endif %}

{% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const double* NOALIAS                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  double                                       t,
  double                                       dt,
  int                                          normal,
  [[maybe_unused]] const Offloadable           offloadable
) {
  {{EIGENVALUES_IMPLEMENTATION}};
}
{% endif %}

{% if FLUX_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  const double* NOALIAS                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* NOALIAS                              F, // F[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] const Offloadable           offloadable
) {
  {% if FLUX_IMPLEMENTATION=="<none>" %}
  abort();
  {% else %}
  {{FLUX_IMPLEMENTATION}}
  {% endif %}
}
{% endif %}

{% if NCP_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  const double* NOALIAS                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const double* NOALIAS                        deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* NOALIAS                              BTimesDeltaQ, // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] const Offloadable           offloadable
) {
  {% if NCP_IMPLEMENTATION=="<none>" %}
  abort();
  {% else %}
  {{NCP_IMPLEMENTATION}}
  {% endif %}
}
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  const double* NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
  double                                       t,
  double                                       dt,
  double* NOALIAS                              S,
  [[maybe_unused]] const Offloadable           offloadable
) {
  {% if SOURCE_TERM_IMPLEMENTATION=="<none>" %}
  abort();
  {% else %}
  {{SOURCE_TERM_IMPLEMENTATION}}
  {% endif %}
}
{% endif %}

{% if POINT_SOURCE_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::pointSource(
  const double* NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  double                                       t,
  double                                       dt,
  double* NOALIAS                              S,
  [[maybe_unused]] const Offloadable           offloadable
) {
  assertion(false, "Point sources are not implemented yet");
  {% if POINT_SOURCE_IMPLEMENTATION=="<none>" %}
  abort();
  {% else %}
  {{POINT_SOURCE_IMPLEMENTATION}}
  {% endif %}
}
{% endif %}
""",
        undefined=jinja2.DebugUndefined,
    )

    d = {}
    d["FLUX_IMPLEMENTATION"] = flux_implementation
    d["NCP_IMPLEMENTATION"] = ncp_implementation
    d["EIGENVALUES_IMPLEMENTATION"] = eigenvalues_implementation
    d["SOURCE_TERM_IMPLEMENTATION"] = source_term_implementation
    d["POINT_SOURCE_IMPLEMENTATION"] = point_source_implementation
    d["STATELESS_PDE_TERMS"] = pde_terms_without_state
    return Template.render(**d)


def create_solver_declarations(
    flux_implementation,
    ncp_implementation,
    eigenvalues_implementation,
    source_term_implementation,
    point_source_implementation,
    pde_terms_without_state,
):
    Template = jinja2.Template(
        """
  public:
    {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
    virtual void sourceTerm(
      const double* NOALIAS                        Q,
      const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      double                                       t,
      double                                       dt,
      double* NOALIAS                              S
    ) override;
    {% endif %}

    {% if POINT_SOURCE_IMPLEMENTATION=="<user-defined>" %}
    #error A Runge-Kutta discontinuous Galerkin solver does not support point sources.
    virtual void pointSource(
      const double* NOALIAS                        Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      double                                       t,
      double                                       dt,
      double* NOALIAS                              S
    ) override;
    {% endif %}

    {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
    /**
     * Determine max eigenvalue over Jacobian in a given point with solution values
     * (states) Q. All parameters are in.
     *
     * @return Max eigenvalue. Result has to be positive, so we are actually speaking
     *   about the maximum absolute eigenvalue.
     */
    virtual double maxEigenvalue(
      const double* NOALIAS                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      double                                       t,
      double                                       dt,
      int                                          normal
    ) override;
    {% endif %}

    {% if FLUX_IMPLEMENTATION=="<user-defined>" %}
    virtual void flux(
      const double* NOALIAS                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* NOALIAS                              F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) override;
    {% endif %}

    {% if NCP_IMPLEMENTATION=="<user-defined>" %}
    virtual void nonconservativeProduct(
      const double* NOALIAS                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double* NOALIAS                        deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* NOALIAS                              BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
    ) override;
    {% endif %}

    {% if STATELESS_PDE_TERMS  and SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
    /**
     * The GPU offloading requires a static function. As we cannot overload the
     * virtual function of the same name with a static one, the additional enum
     * separates the two signatures.
     */
     static void sourceTerm(
       const double* NOALIAS                        Q,
       const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
       double                                       t,
       double                                       dt,
       double* NOALIAS                              S,
       [[maybe_unused]] Offloadable                 offloadable
     );
    {% endif %}

    {% if STATELESS_PDE_TERMS  and POINT_SOURCE_IMPLEMENTATION=="<user-defined>" %}
    /**
     * The GPU offloading requires a static function. As we cannot overload the
     * virtual function of the same name with a static one, the additional enum
     * separates the two signatures.
     */
    #error A Runge-Kutta discontinuous Galerkin solver does not support point sources.
     static void pointSource(
       const double* NOALIAS                        Q,
       const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
       const tarch::la::Vector<DIMENSIONS, double>& volumeH,
       double                                       t,
       double                                       dt,
       double* NOALIAS                              S,
       [[maybe_unused]] Offloadable                 offloadable
     );
    {% endif %}

    {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    /**
     * The GPU offloading requires a static function. As we cannot overload the
     * virtual function of the same name with a static one, the additional enum
     * separates the two signatures.
     */
    static double maxEigenvalue(
      const double* NOALIAS                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      double                                       t,
      double                                       dt,
      int                                          normal,
      [[maybe_unused]] Offloadable                 offloadable
    );
    {% endif %}

    {% if STATELESS_PDE_TERMS  and FLUX_IMPLEMENTATION=="<user-defined>" %}
    /**
     * The GPU offloading requires a static function. As we cannot overload the
     * virtual function of the same name with a static one, the additional enum
     * separates the two signatures.
     */
    static void flux(
      const double* NOALIAS                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* NOALIAS                              F, // F[{{NUMBER_OF_UNKNOWNS}}]
      [[maybe_unused]] Offloadable                 offloadable
    );
    {% endif %}

    {% if STATELESS_PDE_TERMS  and NCP_IMPLEMENTATION=="<user-defined>" %}
    /**
     * The GPU offloading requires a static function. As we cannot overload the
     * virtual function of the same name with a static one, the additional enum
     * separates the two signatures.
     */
    static void nonconservativeProduct(
      const double* NOALIAS                        Q,       // Q[5+0],
      const double* NOALIAS                        deltaQ,    // deltaQ[5+0]
      const tarch::la::Vector<DIMENSIONS, double>& x,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* NOALIAS                              BTimesDeltaQ,     // BTimesDeltaQ[5]
      [[maybe_unused]] Offloadable                 offloadable
    );
    {% endif %}
""",
        undefined=jinja2.DebugUndefined,
    )
    d = {}
    d["FLUX_IMPLEMENTATION"] = flux_implementation
    d["NCP_IMPLEMENTATION"] = ncp_implementation
    d["EIGENVALUES_IMPLEMENTATION"] = eigenvalues_implementation
    d["SOURCE_TERM_IMPLEMENTATION"] = source_term_implementation
    d["POINT_SOURCE_IMPLEMENTATION"] = point_source_implementation
    d["STATELESS_PDE_TERMS"] = pde_terms_without_state
    return Template.render(**d)


def create_solver_definitions(
    flux_implementation,
    ncp_implementation,
    eigenvalues_implementation,
    source_term_implementation,
    point_source_implementation,
    pde_terms_without_state,
):
    Template = jinja2.Template(
        """
{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const double* NOALIAS                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  double                                       t,
  double                                       dt,
  int                                          normal
)  {
  // @todo implement
}
{% endif %}

{% if FLUX_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  const double* NOALIAS                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* NOALIAS                              F // F[{{NUMBER_OF_UNKNOWNS}}]
)  {
  // @todo implement
}
{% endif %}

{% if NCP_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  const double* NOALIAS                        Q,    // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const double* NOALIAS                        deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* NOALIAS                              BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
)  {
  // @todo implement
}
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  const double* NOALIAS                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
  double                                       t,
  double                                       dt,
  double* NOALIAS                              S  // S[{{NUMBER_OF_UNKNOWNS}}]
) {
  // @todo implement and set every entry of S
  for (int i=0; i<NumberOfUnknowns; i++) {
    S[i] = 0.0;
  }
}
{% endif %}

{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const double* NOALIAS                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  double                                       t,
  double                                       dt,
  int                                          normal,
  [[maybe_unused]] const Offloadable           offloadable
)  {
  // @todo implement
}
{% endif %}

{% if FLUX_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  const double* NOALIAS                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* NOALIAS                              F,
  [[maybe_unused]] const Offloadable           offloadable
)  {
  // @todo implement
}
{% endif %}

{% if NCP_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  const double* NOALIAS                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const double* NOALIAS                        deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* NOALIAS                              BTimesDeltaQ,
  [[maybe_unused]] const Offloadable           offloadable
)  {
  // @todo implement
}
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  const double* NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
  double                                       t,
  double                                       dt,
  double* NOALIAS                              S,
  [[maybe_unused]] const Offloadable           offloadable
) {
  // @todo implement
}
{% endif %}

{% if POINT_SOURCE_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
#error A Runge-Kutta discontinuous Galerkin solver does not support point sources.
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  const double* NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
  double                                       t,
  double                                       dt,
  double* NOALIAS                              S,
  [[maybe_unused]] const Offloadable           offloadable
) {
  // @todo implement
}
{% endif %}
""",
        undefined=jinja2.DebugUndefined,
    )
    d = {}
    d["FLUX_IMPLEMENTATION"] = flux_implementation
    d["NCP_IMPLEMENTATION"] = ncp_implementation
    d["EIGENVALUES_IMPLEMENTATION"] = eigenvalues_implementation
    d["SOURCE_TERM_IMPLEMENTATION"] = source_term_implementation
    d["POINT_SOURCE_IMPLEMENTATION"] = point_source_implementation
    d["STATELESS_PDE_TERMS"] = pde_terms_without_state
    return Template.render(**d)


def create_start_time_step_implementation_for_fixed_time_stepping(
    normalised_time_step_size,
):
    return (
        """
  if (
    _maxCellH>0.0
    and
    isFirstGridSweepOfTimeStep()
  ) {
    logInfo("t" << tarch::align << _minTimeStampThisTimeStep);
    logInfo("dt" << tarch::align << getTimeStepSize());
    logInfo("h_{min}" << tarch::align << _minCellHThisTimeStep);
    logInfo("h_{max}" << tarch::align << _maxCellHThisTimeStep);
  }
  _timeStepSize  = """
        + str(normalised_time_step_size)
        + """;
"""
    )


def create_volumetric_solver_call(polynomial_basis, solver_variant):
    """
    Returns the volumetric kernel call that matches polynomial_basis and
    solver_variant.
    """
    if (
        isinstance(polynomial_basis, GaussLegendreBasis)
        and solver_variant == SolverVariant.WithVirtualFunctions
    ):
        return """cellIntegralPatchwiseInSituGaussLegendreFunctors(
      cellData,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      [&](
        const double* NOALIAS                        Q,
        const tarch::la::Vector<DIMENSIONS, double>& x,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double* NOALIAS                              F
      )->void {
        {% if FLUX_IMPLEMENTATION!="<none>" %}
        repositories::{{SOLVER_INSTANCE}}.flux(Q,x,t,dt,normal,F);
        {% endif %}
      }, //flux
      [&](
        const double* NOALIAS                        Q,
        const double* NOALIAS                        deltaQ,
        const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double* NOALIAS                              F
      ) -> void {
          {% if NCP_IMPLEMENTATION!="<none>" %}
          repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct(Q, deltaQ, faceCentre, t, dt, normal, F);
          {% endif %}
        }, //ncp
      [&](
        const double* NOALIAS                        Q,
        const tarch::la::Vector<DIMENSIONS, double>& x,
        double                                       t,
        double                                       dt,
        double* NOALIAS                              S
      ) -> void {
        {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
        repositories::{{SOLVER_INSTANCE}}.sourceTerm(Q,x,t,dt,S);
        {% endif %}
      }, //source
      [&](
        const double* NOALIAS                        Q,
        const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
        const tarch::la::Vector<DIMENSIONS, double>& cellH,
        double                                       t,
        double                                       dt
      ) -> std::vector<::exahype2::dg::PointSource> {
        {% if POINT_SOURCES_IMPLEMENTATION!="<none>" %}
        return repositories::{{SOLVER_INSTANCE}}.pointSources(Q,cellCentre,cellH,t,dt);
        {% else %}
        return std::vector<::exahype2::dg::PointSource>();
        {% endif %}
      }, //source
      repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1D,
      repositories::{{SOLVER_INSTANCE}}.MassMatrixDiagonal1D,
      repositories::{{SOLVER_INSTANCE}}.StiffnessOperator1D,
      repositories::{{SOLVER_INSTANCE}}.DerivativeOperator1D,
      {{ "true" if FLUX_IMPLEMENTATION!="<none>" else "false" }}, //useFlux
      {{ "true" if NCP_IMPLEMENTATION!="<none>" else "false" }}, //useNCP
      {{ "true" if SOURCE_TERM_IMPLEMENTATION!="<none>" else "false" }}, //useSource
      {{ "true" if POINT_SOURCES_IMPLEMENTATION!="<none>" else "false" }} //usePointSource
    );
"""
    elif (
        isinstance(polynomial_basis, GaussLegendreBasis)
        and solver_variant == SolverVariant.Stateless
    ):
        return """cellIntegralPatchwiseInSituGaussLegendre<{{SOLVER_NAME}},{{ORDER}},{{NUMBER_OF_UNKNOWNS}},{{NUMBER_OF_AUXILIARY_VARIABLES}}>(
      cellData,
      {{ "true" if FLUX_IMPLEMENTATION!="<none>" else "false" }},          //useFlux
      {{ "true" if NCP_IMPLEMENTATION!="<none>" else "false" }},           //useNCP
      {{ "true" if SOURCE_TERM_IMPLEMENTATION!="<none>" else "false" }},   //useSource
      {{ "true" if POINT_SOURCES_IMPLEMENTATION!="<none>" else "false" }}  //usePointSource
    );
"""
    elif (
        isinstance(polynomial_basis, GaussLegendreBasis)
        and solver_variant == SolverVariant.Accelerator
    ):
        return """cellIntegralPatchwiseInSituGaussLegendre<{{SOLVER_NAME}},{{ORDER}},{{NUMBER_OF_UNKNOWNS}},{{NUMBER_OF_AUXILIARY_VARIABLES}}>(
      targetDevice,
      cellData,
      {{ "true" if FLUX_IMPLEMENTATION!="<none>" else "false" }},          //useFlux
      {{ "true" if NCP_IMPLEMENTATION!="<none>" else "false" }},           //useNCP
      {{ "true" if SOURCE_TERM_IMPLEMENTATION!="<none>" else "false" }},   //useSource
      {{ "true" if POINT_SOURCES_IMPLEMENTATION!="<none>" else "false" }}  //usePointSource
    );
"""
    else:
        return "#solver variant not implemented yet"


def create_add_solver_contributions_call(polynomial_basis):
    if isinstance(polynomial_basis, GaussLegendreBasis):
        return """integrateOverRiemannSolutionsAndAddToVolumeGaussLegendre(
    #if DIMENSIONS==2
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(0).data(), //left
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(2).data(), //right
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(1).data(), //down
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(3).data(), //up
    #else
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(0).data(), //left
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(3).data(), //right
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(1).data(), //down
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(4).data(), //up
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(2).data(), //front
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(5).data(), //back
    #endif
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      marker.getH(),
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft1D,
      repositories::{{SOLVER_INSTANCE}}.MassMatrixDiagonal1D,
      dQdt
    );
"""
    else:
        return "#error The kernel call supports a Gauss-Legendre basis only."


def create_multiply_with_inverted_mass_matrix_call(polynomial_basis):
    if isinstance(polynomial_basis, GaussLegendreBasis):
        return """multiplyWithInvertedMassMatrixGaussLegendre(
      cellData,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      repositories::{{SOLVER_INSTANCE}}.MassMatrixDiagonal1D
    );
"""
    else:
        return "#error The kernel call supports a Gauss-Legendre basis only."
