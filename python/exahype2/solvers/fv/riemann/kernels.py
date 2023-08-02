import jinja2

def create_compute_Riemann_kernel_for_UserDefinedRiemann(flux_implementation, riemann_solver_implementation, compute_max_eigenvalue_of_next_time_step=True):
    template = "timeStepWithRiemann_patchwise_heap"
    template += """_functors(
  patchData,
  {{NUMBER_OF_VOLUMES_PER_AXIS}},
  {{HALO_SIZE}},
  {{NUMBER_OF_UNKNOWNS}},
  {{NUMBER_OF_AUXILIARY_VARIABLES}},
  {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
  {% if RIEMANN_SOLVER_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
  {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %},
  [&](
    const double * __restrict__ Q,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double * __restrict__ F
  )->void {
    {% if FLUX_IMPLEMENTATION!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.flux( Q, faceCentre, volumeH, t, dt, normal, F );
    {% endif %}
  },
  [&](
    const double * __restrict__                  QR,
    const double * __restrict__                  QL,
    const double * __restrict__                  FR,
    const double * __restrict__                  FL,
    const double * __restrict__                  maxLR,
    const double * __restrict__                  maxLL,
    const tarch::la::Vector<Dimensions,double>&  xR,
    const tarch::la::Vector<Dimensions,double>&  xL,
    const tarch::la::Vector<Dimensions,double>&  h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double * __restrict__                        QSR,
    double * __restrict__                        QSL
  )->void {
    {% if RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.solveRiemannProblem( QR, QL, FR, FL, maxLR, maxLL, xR, xL, h, t, dt, normal, QSR, QSL );
    {% endif %}
  },
  [&](
    const double * __restrict__ Q,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal
  )->double {
    return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, dt, normal);
  }
);
  """
    result = jinja2.Template( template, undefined=jinja2.DebugUndefined)
    d= {}
    d[ "FLUX_IMPLEMENTATION" ]         = flux_implementation
    d[ "NCP_IMPLEMENTATION" ]          = riemann_solver_implementation
    d[ "COMPUTE_MAX_EIGENVALUE" ]      = compute_max_eigenvalue_of_next_time_step
    d[ "TEMP_DATA_ENUMERATOR" ]        = "::exahype2::enumerator::AoSLexicographicEnumerator"
    return result.render(**d)

def create_abstract_solver_declarations(flux_implementation, riemann_solver_implementation, eigenvalues_implementation):
    Template = jinja2.Template( """
  public:
    {% if EIGENVALUES_IMPLEMENTATION=="<none>" %}
    #error eigenvalue implementation cannot be none
    {% endif %}

    /**
     * Determine max eigenvalue over Jacobian in a given point with solution values
     * (states) Q. All parameters are in.
     *
     * @return Max eigenvalue. Result has to be positive, so we are actually speaking
     *   about the maximum absolute eigenvalue.
     */
    virtual double maxEigenvalue(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal
    ) {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final{% endif %};
    
    {% if FLUX_IMPLEMENTATION!="<none>" %}
    virtual void flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if FLUX_IMPLEMENTATION=="<user-defined>" %}=0{% else %} final {% endif %};
    {% endif %}
                               
    {% if RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
    virtual void solveRiemannProblem(
        const double * __restrict__                  QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
        const double * __restrict__                  QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
        const double * __restrict__                  FR, // FR[{{NUMBER_OF_UNKNOWNS}}]
        const double * __restrict__                  FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
        const double * __restrict__                  maxLR, // maxLR[1]
        const double * __restrict__                  maxLL, // maxLL[1]
        const tarch::la::Vector<Dimensions,double>&  xR,
        const tarch::la::Vector<Dimensions,double>&  xL,
        const tarch::la::Vector<Dimensions,double>&  h,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__                        QSR, // QSR[{{NUMBER_OF_UNKNOWNS}}]
        double * __restrict__                        QSL  // QSR[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if RIEMANN_SOLVER_IMPLEMENTATION=="<user-defined>" %}=0{% else %} final {% endif %};
    {% endif %}
""", undefined=jinja2.DebugUndefined)

    d= {}
    d[ "FLUX_IMPLEMENTATION"]                 = flux_implementation
    d[ "EIGENVALUES_IMPLEMENTATION"]          = eigenvalues_implementation
    d[ "RIEMANN_SOLVER_IMPLEMENTATION"]       = riemann_solver_implementation
    return Template.render(**d)

def create_abstract_solver_definitions(flux_implementation, riemann_solver_implementation, eigenvalues_implementation):
    Template = jinja2.Template( """
{% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and EIGENVALUES_IMPLEMENTATION!="<none>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal
) {
  {{EIGENVALUES_IMPLEMENTATION}}
}
{% endif %}

{% if FLUX_IMPLEMENTATION!="<none>" and FLUX_IMPLEMENTATION!="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
 const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
 const tarch::la::Vector<Dimensions,double>&  faceCentre,
 const tarch::la::Vector<Dimensions,double>&  volumeH,
 double                                       t,
 double                                       dt,
 int                                          normal,
 double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{FLUX_IMPLEMENTATION}}
}
{% endif %}
                               
{% if RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
void solveRiemannProblem(
  const double * __restrict__                  QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const double * __restrict__                  QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const double * __restrict__                  FR, // FR[{{NUMBER_OF_UNKNOWNS}}]
  const double * __restrict__                  FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
  const double * __restrict__                  maxLR, // maxLR[1]
  const double * __restrict__                  maxLL, // maxLL[1]
  const tarch::la::Vector<Dimensions,double>&  xR,
  const tarch::la::Vector<Dimensions,double>&  xL,
  const tarch::la::Vector<Dimensions,double>&  h,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__                        QSR, // QSR[{{NUMBER_OF_UNKNOWNS}}]
  double * __restrict__                        QSL  // QSR[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{RIEMANN_SOLVER_IMPLEMENTATION}}          
}
{% endif %}
""", undefined=jinja2.DebugUndefined)

    d= {}
    d[ "FLUX_IMPLEMENTATION"]                 = flux_implementation
    d[ "RIEMANN_SOLVER_IMPLEMENTATION"]       = riemann_solver_implementation
    d[ "EIGENVALUES_IMPLEMENTATION"]          = eigenvalues_implementation
    return Template.render(**d)

def create_solver_declarations(flux_implementation, riemann_solver_implementation, eigenvalues_implementation):
    Template = jinja2.Template( """
  public:

    {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
    /**
     * Determine max eigenvalue over Jacobian in a given point with solution values
     * (states) Q. All parameters are in.
     *
     * @return Max eigenvalue. Result has to be positive, so we are actually speaking
     *   about the maximum absolute eigenvalue.
     */
    virtual double maxEigenvalue(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal
    ) override;
    {% endif %}


    {% if FLUX_IMPLEMENTATION=="<user-defined>" %}
    virtual void flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) override;
    {% endif %}
                               
    {% if RIEMANN_SOLVER_IMPLEMENTATION=="<user-defined>" %}
    virtual void solveRiemannProblem(
        const double * __restrict__                  QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
        const double * __restrict__                  QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
        const double * __restrict__                  FR, // FR[{{NUMBER_OF_UNKNOWNS}}]
        const double * __restrict__                  FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
        const double * __restrict__                  maxLR, // maxLR[1]
        const double * __restrict__                  maxLL, // maxLL[1]
        const tarch::la::Vector<Dimensions,double>&  xR,
        const tarch::la::Vector<Dimensions,double>&  xL,
        const tarch::la::Vector<Dimensions,double>&  h,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__                        QSR, // QSR[{{NUMBER_OF_UNKNOWNS}}]
        double * __restrict__                        QSL  // QSR[{{NUMBER_OF_UNKNOWNS}}]
    ) override;
    {% endif %}
""", undefined=jinja2.DebugUndefined)
    d= {}
    d[ "FLUX_IMPLEMENTATION"]                 = flux_implementation
    d[ "RIEMANN_SOLVER_IMPLEMENTATION"]       = riemann_solver_implementation
    d[ "EIGENVALUES_IMPLEMENTATION"]          = eigenvalues_implementation
    return Template.render(**d)

def create_solver_definitions(flux_implementation, riemann_solver_implementation, eigenvalues_implementation):
    Template = jinja2.Template( """
{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal
)  {
  logTraceInWith4Arguments( "maxEigenvalue(...)", faceCentre, volumeH, t, normal );
  // @todo implement
  logTraceOut( "maxEigenvalue(...)" );
}
{% endif %}


{% if FLUX_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
)  {
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );
  // @todo implement
  logTraceOut( "flux(...)" );
}
{% endif %}
                               

{% if RIEMANN_SOLVER_IMPLEMENTATION=="<user-defined>" %}
    virtual void solveRiemannProblem(
        const double * __restrict__                  QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
        const double * __restrict__                  QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
        const double * __restrict__                  FR, // FR[{{NUMBER_OF_UNKNOWNS}}]
        const double * __restrict__                  FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
        const double * __restrict__                  maxLR, // maxLR[1]
        const double * __restrict__                  maxLL, // maxLL[1]
        const tarch::la::Vector<Dimensions,double>&  xR,
        const tarch::la::Vector<Dimensions,double>&  xL,
        const tarch::la::Vector<Dimensions,double>&  h,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__                        QSR, // QSR[{{NUMBER_OF_UNKNOWNS}}]
        double * __restrict__                        QSL  // QSR[{{NUMBER_OF_UNKNOWNS}}]
    ) {
        logTraceInWith4Arguments( "solveRiemannProblem(...)", xR, xL, t, normal );
        // @todo implement
        logTraceOut( "solveRiemannProblem(...)" );                 
    }
    {% endif %}
""", undefined=jinja2.DebugUndefined)
    d= {}
    d[ "FLUX_IMPLEMENTATION"]                 = flux_implementation
    d[ "RIEMANN_SOLVER_IMPLEMENTATION"]       = riemann_solver_implementation
    d[ "EIGENVALUES_IMPLEMENTATION"]          = eigenvalues_implementation
    return Template.render(**d)