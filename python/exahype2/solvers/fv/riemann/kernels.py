# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from enum import Enum


class SolverVariant(Enum):
    WithVirtualFunctions = 0
    Stateless = 1
    Multicore = 2
    Accelerator = 3


class KernelVariant(Enum):
    PatchWiseAoS = 10
    PatchWiseAoSoA = 11
    PatchWiseSoA = 12
    BatchedAoS = 20
    BatchedAoSoA = 21
    BatchedSoA = 22
    TaskGraphAoS = 30
    TaskGraphAoSoA = 31
    TaskGraphSoA = 32
    VolumeWiseAoS = 40
    VolumeWiseAoSoA = 41
    VolumeWiseSoA = 42


def create_compute_Riemann_kernel(
    flux_implementation,
    ncp_implementation,
    riemann_solver_implementation,
    source_term_implementation,
    compute_max_eigenvalue_of_next_time_step,
    solver_variant: SolverVariant,
    kernel_variant: KernelVariant,
):
    """
    Return only the unqualified function call, i.e., without any namespaces.
    So by setting the right namespace as prefix, you can direct it to particular
    implementations.
    """
    KernelCalls = {
        KernelVariant.PatchWiseAoS: "timeStepWithRiemannPatchwiseHeap",
        KernelVariant.PatchWiseAoSoA: "timeStepWithRiemannPatchwiseHeap",
        KernelVariant.PatchWiseSoA: "timeStepWithRiemannPatchwiseHeap",
        KernelVariant.BatchedAoS: "timeStepWithRiemannBatchedHeap",
        KernelVariant.BatchedAoSoA: "timeStepWithRiemannBatchedHeap",
        KernelVariant.BatchedSoA: "timeStepWithRiemannBatchedHeap",
        KernelVariant.TaskGraphAoS: "timeStepWithRiemannTaskgraphHeap",
        KernelVariant.TaskGraphAoSoA: "timeStepWithRiemannTaskgraphHeap",
        KernelVariant.TaskGraphSoA: "timeStepWithRiemannTaskgraphHeap",
        KernelVariant.VolumeWiseAoS: "timeStepWithRiemannVolumewise",
        KernelVariant.VolumeWiseAoSoA: "timeStepWithRiemannVolumewise",
        KernelVariant.VolumeWiseSoA: "timeStepWithRiemannVolumewise",
    }

    EnumeratorTemplateTypes = {
        KernelVariant.PatchWiseAoS: "::exahype2::enumerator::AoSLexicographicEnumerator",
        KernelVariant.PatchWiseAoSoA: "::exahype2::enumerator::AoSoALexicographicEnumerator",
        KernelVariant.PatchWiseSoA: "::exahype2::enumerator::SoALexicographicEnumerator",
        KernelVariant.BatchedAoS: "::exahype2::enumerator::AoSLexicographicEnumerator",
        KernelVariant.BatchedAoSoA: "::exahype2::enumerator::AoSoALexicographicEnumerator",
        KernelVariant.BatchedSoA: "::exahype2::enumerator::SoALexicographicEnumerator",
        KernelVariant.TaskGraphAoS: "::exahype2::enumerator::AoSLexicographicEnumerator",
        KernelVariant.TaskGraphAoSoA: "::exahype2::enumerator::AoSoALexicographicEnumerator",
        KernelVariant.TaskGraphSoA: "::exahype2::enumerator::SoALexicographicEnumerator",
        KernelVariant.VolumeWiseAoS: "::exahype2::enumerator::AoSLexicographicEnumerator",
        KernelVariant.VolumeWiseAoSoA: "::exahype2::enumerator::AoSoALexicographicEnumerator",
        KernelVariant.VolumeWiseSoA: "::exahype2::enumerator::SoALexicographicEnumerator",
    }

    template = KernelCalls[kernel_variant]

    if solver_variant == SolverVariant.WithVirtualFunctions:
        template += """Functors<
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      {{HALO_SIZE}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_TERM_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if RIEMANN_SOLVER_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %},
      {{TEMP_DATA_ENUMERATOR}}
    >(patchData,
  [&](
    [[maybe_unused]] const double* __restrict__                    Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&  x,
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&  h,
    [[maybe_unused]] double                                        t,
    [[maybe_unused]] double                                        dt,
    [[maybe_unused]] int                                           normal,
    [[maybe_unused]] double* __restrict__                          F // F[{{NUMBER_OF_UNKNOWNS}}]
  )->void {
    {% if FLUX_IMPLEMENTATION!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.flux(Q, x, h, t, dt, normal, F);
    {% endif %}
  },
  [&](
    [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
    [[maybe_unused]] double                                       t,
    [[maybe_unused]] double                                       dt,
    [[maybe_unused]] int                                          normal,
    [[maybe_unused]] double* __restrict__                         BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
  )->void {
    {% if NCP_IMPLEMENTATION!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct(Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ);
    {% endif %}
  },
  [&](
    [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
    [[maybe_unused]] double                                       t,
    [[maybe_unused]] double                                       dt,
    [[maybe_unused]] double* __restrict__                         S // S[{{NUMBER_OF_UNKNOWNS}}]
  )->void {
    {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.sourceTerm(Q, x, h, t, dt, S);
    {% endif %}
  },
  [&](
    const double* __restrict__                    Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    const tarch::la::Vector<Dimensions, double>&  x,
    const tarch::la::Vector<Dimensions, double>&  h,
    double                                        t,
    double                                        dt,
    int                                           normal,
    double* __restrict__                          L
  )->void {
    repositories::{{SOLVER_INSTANCE}}.eigenvalues(Q, x, h, t, dt, normal, L);
  },
  [&](
    [[maybe_unused]] const double* __restrict__                    QR,
    [[maybe_unused]] const double* __restrict__                    QL,
    [[maybe_unused]] const double* __restrict__                    FR,
    [[maybe_unused]] const double* __restrict__                    FL,
    [[maybe_unused]] const double* __restrict__                    LR,
    [[maybe_unused]] const double* __restrict__                    LL,
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&  xR,
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&  xL,
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&  h,
    [[maybe_unused]] double                                        t,
    [[maybe_unused]] double                                        dt,
    [[maybe_unused]] int                                           normal,
    [[maybe_unused]] double* __restrict__                          APDQ,
    [[maybe_unused]] double* __restrict__                          AMDQ
  )->double {
    {% if RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
    return repositories::{{SOLVER_INSTANCE}}.solveRiemannProblem(QR, QL, FR, FL, LR, LL, xR, xL, h, t, dt, normal, APDQ, AMDQ);
    {% endif %}
  }
);
  """
    elif solver_variant == SolverVariant.Stateless:
        template += """Stateless<
      {{SOLVER_NAME}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      {{HALO_SIZE}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_TERM_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if RIEMANN_SOLVER_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %},
      {{TEMP_DATA_ENUMERATOR}}
      >(patchData);
  """
    elif solver_variant == SolverVariant.Multicore:
        template += """Stateless<
      {{SOLVER_NAME}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      {{HALO_SIZE}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_TERM_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if RIEMANN_SOLVER_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %},
      {{TEMP_DATA_ENUMERATOR}}
      >(patchData, peano4::utils::LoopPlacement::SpreadOut);
  """
    elif solver_variant == SolverVariant.Accelerator:
        template += """Stateless<
      {{SOLVER_NAME}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      {{HALO_SIZE}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_TERM_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if RIEMANN_SOLVER_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %},
      {{TEMP_DATA_ENUMERATOR}}
      >(targetDevice, patchData);
  """
    else:
        assert False, "Not supported combination: {} x {}".format(
            solver_variant, kernel_variant
        )

    result = jinja2.Template(template, undefined=jinja2.DebugUndefined)
    d = {}
    d["FLUX_IMPLEMENTATION"] = flux_implementation
    d["NCP_IMPLEMENTATION"] = ncp_implementation
    d["RIEMANN_SOLVER_IMPLEMENTATION"] = riemann_solver_implementation
    d["SOURCE_TERM_IMPLEMENTATION"] = source_term_implementation
    d["COMPUTE_MAX_EIGENVALUE"] = compute_max_eigenvalue_of_next_time_step
    d["TEMP_DATA_ENUMERATOR"] = EnumeratorTemplateTypes[kernel_variant]
    return result.render(**d)


def create_abstract_solver_declarations(
    flux_implementation,
    ncp_implementation,
    eigenvalues_implementation,
    riemann_solver_implementation,
    source_term_implementation,
    pde_terms_without_state,
):
    Template = jinja2.Template(
        """
  public:
    {% if EIGENVALUES_IMPLEMENTATION=="<none>" %}
    #error Eigenvalue implementation cannot be none
    {% endif %}

    {% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void eigenvalues(
      const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* __restrict__                         L,
      Offloadable
    )
    //#if defined(GPUOffloadingSYCL)
    //{}
    //#else
    ;
    //#endif
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    virtual void eigenvalues(
      const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* __restrict__                         L
    ) {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %} = 0{% else %} final{% endif %};

    {% if FLUX_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void flux(
      const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* __restrict__                         F, // F[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if FLUX_IMPLEMENTATION!="<none>" %}
    virtual void flux(
      const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* __restrict__                         F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if FLUX_IMPLEMENTATION=="<user-defined>" %} = 0{% else %} final {% endif %};
    {% endif %}

    {% if NCP_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void nonconservativeProduct(
      const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* __restrict__                         BTimesDeltaQ, // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if NCP_IMPLEMENTATION!="<none>" %}
    virtual void nonconservativeProduct(
      const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* __restrict__                         BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if NCP_IMPLEMENTATION=="<user-defined>" %} = 0{% else %} final {% endif %};
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void sourceTerm(
      const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      double                                       t,
      double                                       dt,
      double* __restrict__                         S, // S[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    virtual void sourceTerm(
      const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      double                                       t,
      double                                       dt,
      double* __restrict__                         S // S[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %} = 0{% else %} final {% endif %};
    {% endif %}

    {% if RIEMANN_SOLVER_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    /**
     * @param QR the right state vector
     * @param QL the left state vector
     * @param FR the right flux vector
     * @param FL the left flux vector
     * @param LR the eigenvalues of the right state vector
     * @param LL the eigenvalues of the left state vector
     * @param xR coordinates of the right state vector
     * @param xL coordinates of the left state vector
     * @param h volume dimensions
     * @param t current time step
     * @param dt previous time step width
     * @param normal dimension currently being solved for
     * @param APDQ right going update, has a positive contribution on right state
     * @param AMDQ left going update, has a negative contribution on left state
     */
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod double solveRiemannProblem(
      const double* __restrict__                    QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const double* __restrict__                    QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const double* __restrict__                    FR, // FR[{{NUMBER_OF_UNKNOWNS}}]
      const double* __restrict__                    FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
      const double* __restrict__                    LR, // LR[{{NUMBER_OF_UNKNOWNS}}]
      const double* __restrict__                    LL, // LL[{{NUMBER_OF_UNKNOWNS}}]
      const tarch::la::Vector<Dimensions, double>&  xR,
      const tarch::la::Vector<Dimensions, double>&  xL,
      const tarch::la::Vector<Dimensions, double>&  h,
      double                                        t,
      double                                        dt,
      int                                           normal,
      double* __restrict__                          APDQ, // APDQ[{{NUMBER_OF_UNKNOWNS}}]
      double* __restrict__                          AMDQ, // AMDQ[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
    /**
     * @param QR the right state vector
     * @param QL the left state vector
     * @param FR the right flux vector
     * @param FL the left flux vector
     * @param LR the eigenvalues of the right state vector
     * @param LL the eigenvalues of the left state vector
     * @param xR coordinates of the right state vector
     * @param xL coordinates of the left state vector
     * @param h volume dimensions
     * @param t current time step
     * @param dt previous time step width
     * @param normal dimension currently being solved for
     * @param APDQ right going update, has a positive contribution on right state
     * @param AMDQ left going update, has a negative contribution on left state
     */
    virtual double solveRiemannProblem(
      const double* __restrict__                    QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const double* __restrict__                    QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const double* __restrict__                    FR, // FR[{{NUMBER_OF_UNKNOWNS}}]
      const double* __restrict__                    FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
      const double* __restrict__                    LR, // LR[{{NUMBER_OF_UNKNOWNS}}]
      const double* __restrict__                    LL, // LL[{{NUMBER_OF_UNKNOWNS}}]
      const tarch::la::Vector<Dimensions, double>&  xR,
      const tarch::la::Vector<Dimensions, double>&  xL,
      const tarch::la::Vector<Dimensions, double>&  h,
      double                                        t,
      double                                        dt,
      int                                           normal,
      double* __restrict__                          APDQ, // APDQ[{{NUMBER_OF_UNKNOWNS}}]
      double* __restrict__                          AMDQ // AMDQ[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if RIEMANN_SOLVER_IMPLEMENTATION=="<user-defined>" %} = 0{% else %} final {% endif %};
    {% endif %}
""",
        undefined=jinja2.DebugUndefined,
    )

    d = {}
    d["FLUX_IMPLEMENTATION"] = flux_implementation
    d["NCP_IMPLEMENTATION"] = ncp_implementation
    d["EIGENVALUES_IMPLEMENTATION"] = eigenvalues_implementation
    d["RIEMANN_SOLVER_IMPLEMENTATION"] = riemann_solver_implementation
    d["SOURCE_TERM_IMPLEMENTATION"] = source_term_implementation
    d["STATELESS_PDE_TERMS"] = pde_terms_without_state
    return Template.render(**d)


def create_abstract_solver_definitions(
    flux_implementation,
    ncp_implementation,
    eigenvalues_implementation,
    riemann_solver_implementation,
    source_term_implementation,
    pde_terms_without_state,
):
    Template = jinja2.Template(
        """
{% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and EIGENVALUES_IMPLEMENTATION!="<none>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
//#if !defined(GPUOffloadingSYCL)
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::eigenvalues(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         L,
  Offloadable
) {
  {{EIGENVALUES_IMPLEMENTATION}}
}
//#endif
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}

{% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and EIGENVALUES_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::eigenvalues(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         L
) {
  {{EIGENVALUES_IMPLEMENTATION}}
}
{% endif %}

{% if FLUX_IMPLEMENTATION!="<user-defined>" and FLUX_IMPLEMENTATION!="<none>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
//#if !defined(GPUOffloadingSYCL)
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         F, // F[{{NUMBER_OF_UNKNOWNS}}]
  Offloadable
) {
  {{FLUX_IMPLEMENTATION}}
}
//#endif
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}

{% if FLUX_IMPLEMENTATION!="<user-defined>" and FLUX_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         F // F[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{FLUX_IMPLEMENTATION}}
}
{% endif %}

{% if NCP_IMPLEMENTATION!="<user-defined>" and NCP_IMPLEMENTATION!="<none>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         BTimesDeltaQ, // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] Offloadable
) {
  {{NCP_IMPLEMENTATION}}
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}

{% if NCP_IMPLEMENTATION!="<user-defined>" and NCP_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{NCP_IMPLEMENTATION}}
}
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] double* __restrict__                         S, // S[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] Offloadable
) {
  {{SOURCE_TERM_IMPLEMENTATION}}
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] double* __restrict__                         S // S[{{NUMBER_OF_UNKNOWNS}}]
) {
  {% if SOURCE_TERM_IMPLEMENTATION!="<empty>" %}
  {{SOURCE_TERM_IMPLEMENTATION}}
  {% else %}
  std::fill_n(S, {{NUMBER_OF_UNKNOWNS}}, 0.0);
  {% endif %}
}
{% endif %}

{% if RIEMANN_SOLVER_IMPLEMENTATION!="<user-defined>" and RIEMANN_SOLVER_IMPLEMENTATION!="<none>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
GPUCallableMethod double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::solveRiemannProblem(
  [[maybe_unused]] const double* __restrict__                    QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* __restrict__                    QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* __restrict__                    FR, // FR[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] const double* __restrict__                    FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] const double* __restrict__                    LR, // LR[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] const double* __restrict__                    LL, // LL[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&  xR,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&  xL,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&  h,
  [[maybe_unused]] double                                        t,
  [[maybe_unused]] double                                        dt,
  [[maybe_unused]] int                                           normal,
  [[maybe_unused]] double* __restrict__                          APDQ, // APDQ[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] double* __restrict__                          AMDQ, // AMDQ[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] Offloadable
) {
  {{RIEMANN_SOLVER_IMPLEMENTATION}}
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}

{% if RIEMANN_SOLVER_IMPLEMENTATION!="<user-defined>" and RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::solveRiemannProblem(
  [[maybe_unused]] const double* __restrict__                    QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* __restrict__                    QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* __restrict__                    FR, // FR[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] const double* __restrict__                    FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] const double* __restrict__                    LR, // LR[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] const double* __restrict__                    LL, // LL[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&  xR,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&  xL,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&  h,
  [[maybe_unused]] double                                        t,
  [[maybe_unused]] double                                        dt,
  [[maybe_unused]] int                                           normal,
  [[maybe_unused]] double* __restrict__                          APDQ, // APDQ[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] double* __restrict__                          AMDQ // AMDQ[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{RIEMANN_SOLVER_IMPLEMENTATION}}
}
{% endif %}
""",
        undefined=jinja2.DebugUndefined,
    )

    d = {}
    d["FLUX_IMPLEMENTATION"] = flux_implementation
    d["NCP_IMPLEMENTATION"] = ncp_implementation
    d["EIGENVALUES_IMPLEMENTATION"] = eigenvalues_implementation
    d["RIEMANN_SOLVER_IMPLEMENTATION"] = riemann_solver_implementation
    d["SOURCE_TERM_IMPLEMENTATION"] = source_term_implementation
    d["STATELESS_PDE_TERMS"] = pde_terms_without_state
    return Template.render(**d)


def create_solver_declarations(
    flux_implementation,
    ncp_implementation,
    eigenvalues_implementation,
    riemann_solver_implementation,
    source_term_implementation,
    pde_terms_without_state,
):
    Template = jinja2.Template(
        """
  public:
    {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void eigenvalues(
      const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* __restrict__                         L,
      Offloadable
    )
    //#if defined(GPUOffloadingSYCL)
    //{}
    //#else
    ;
    //#endif
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
    virtual void eigenvalues(
      const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* __restrict__                         L
    ) override;
    {% endif %}

    {% if FLUX_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void flux(
      const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* __restrict__                         F, // F[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if FLUX_IMPLEMENTATION=="<user-defined>" %}
    virtual void flux(
      const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* __restrict__                         F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) override;
    {% endif %}

    {% if NCP_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void nonconservativeProduct(
      const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* __restrict__                         BTimesDeltaQ, // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if NCP_IMPLEMENTATION=="<user-defined>" %}
    virtual void nonconservativeProduct(
      const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* __restrict__                         BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
    ) override;
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void sourceTerm(
      const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      double                                       t,
      double                                       dt,
      double* __restrict__                         S, // S[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
    virtual void sourceTerm(
      const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      double                                       t,
      double                                       dt,
      double* __restrict__                         S // S[{{NUMBER_OF_UNKNOWNS}}]
    ) override;
    {% endif %}

    {% if RIEMANN_SOLVER_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    /**
     * @param QR the right state vector
     * @param QL the left state vector
     * @param FR the right flux vector
     * @param FL the left flux vector
     * @param LR the eigenvalues of the right state vector
     * @param LL the eigenvalues of the left state vector
     * @param xR coordinates of the right state vector
     * @param xL coordinates of the left state vector
     * @param h volume dimensions
     * @param t current time step
     * @param dt previous time step width
     * @param normal dimension currently being solved for
     * @param APDQ right going update, has a positive contribution on right state
     * @param AMDQ left going update, has a negative contribution on left state
     */
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod double solveRiemannProblem(
      const double* __restrict__                    QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const double* __restrict__                    QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const double* __restrict__                    FR, // FR[{{NUMBER_OF_UNKNOWNS}}]
      const double* __restrict__                    FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
      const double* __restrict__                    LR, // LR[{{NUMBER_OF_UNKNOWNS}}]
      const double* __restrict__                    LL, // LL[{{NUMBER_OF_UNKNOWNS}}]
      const tarch::la::Vector<Dimensions, double>&  xR,
      const tarch::la::Vector<Dimensions, double>&  xL,
      const tarch::la::Vector<Dimensions, double>&  h,
      double                                        t,
      double                                        dt,
      int                                           normal,
      double* __restrict__                          APDQ, // APDQ[{{NUMBER_OF_UNKNOWNS}}]
      double* __restrict__                          AMDQ, // AMDQ[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if RIEMANN_SOLVER_IMPLEMENTATION=="<user-defined>" %}
    /**
     * @param QR the right state vector
     * @param QL the left state vector
     * @param FR the right flux vector
     * @param FL the left flux vector
     * @param LR the eigenvalues of the right state vector
     * @param LL the eigenvalues of the left state vector
     * @param xR coordinates of the right state vector
     * @param xL coordinates of the left state vector
     * @param h volume dimensions
     * @param t current time step
     * @param dt previous time step width
     * @param normal dimension currently being solved for
     * @param APDQ right going update, has a positive contribution on right state
     * @param AMDQ left going update, has a negative contribution on left state
     */
    virtual double solveRiemannProblem(
      const double* __restrict__                    QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const double* __restrict__                    QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const double* __restrict__                    FR, // FR[{{NUMBER_OF_UNKNOWNS}}]
      const double* __restrict__                    FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
      const double* __restrict__                    LR, // LR[{{NUMBER_OF_UNKNOWNS}}]
      const double* __restrict__                    LL, // LL[{{NUMBER_OF_UNKNOWNS}}]
      const tarch::la::Vector<Dimensions, double>&  xR,
      const tarch::la::Vector<Dimensions, double>&  xL,
      const tarch::la::Vector<Dimensions, double>&  h,
      double                                        t,
      double                                        dt,
      int                                           normal,
      double* __restrict__                          APDQ, // APDQ[{{NUMBER_OF_UNKNOWNS}}]
      double* __restrict__                          AMDQ // AMDQ[{{NUMBER_OF_UNKNOWNS}}]
    ) override;
    {% endif %}
""",
        undefined=jinja2.DebugUndefined,
    )

    d = {}
    d["FLUX_IMPLEMENTATION"] = flux_implementation
    d["NCP_IMPLEMENTATION"] = ncp_implementation
    d["EIGENVALUES_IMPLEMENTATION"] = eigenvalues_implementation
    d["RIEMANN_SOLVER_IMPLEMENTATION"] = riemann_solver_implementation
    d["SOURCE_TERM_IMPLEMENTATION"] = source_term_implementation
    d["STATELESS_PDE_TERMS"] = pde_terms_without_state
    return Template.render(**d)


def create_solver_definitions(
    flux_implementation,
    ncp_implementation,
    eigenvalues_implementation,
    riemann_solver_implementation,
    source_term_implementation,
    pde_terms_without_state,
):
    Template = jinja2.Template(
        """
{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
//#if !defined(GPUOffloadingSYCL)
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::eigenvalues(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         L,
  Offloadable
) {
  logTraceInWith4Arguments("eigenvalues(...)", x, h, t, normal);
  // @todo implement
  logTraceOut("eigenvalues(...)");
}
//#endif
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}

{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::eigenvalues(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         L
) {
  logTraceInWith4Arguments("eigenvalues(...)", x, h, t, normal);
  // @todo implement
  logTraceOut("eigenvalues(...)");
}
{% endif %}

{% if FLUX_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
//#if !defined(GPUOffloadingSYCL)
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         F, // F[{{NUMBER_OF_UNKNOWNS}}]
  Offloadable
) {
  logTraceInWith4Arguments("flux(...)", x, h, t, normal);
  // @todo implement
  logTraceOut("flux(...)");
}
//#endif
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}

{% if FLUX_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         F // F[{{NUMBER_OF_UNKNOWNS}}]
) {
  logTraceInWith4Arguments("flux(...)", x, h, t, normal);
  // @todo implement
  logTraceOut("flux(...)");
}
{% endif %}

{% if NCP_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         BTimesDeltaQ, // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] Offloadable
) {
  logTraceInWith4Arguments("nonconservativeProduct(...)", x, h, t, normal);
  // @todo implement
  logTraceOut("nonconservativeProduct(...)");
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}

{% if NCP_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         BTimesDeltaQ // BTimesDeltaQQ[{{NUMBER_OF_UNKNOWNS}}]
) {
  logTraceInWith4Arguments("nonconservativeProduct(...)", x, h, t, normal);
  // @todo implement
  logTraceOut("nonconservativeProduct(...)");
}
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] double* __restrict__                         S, // S[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] Offloadable
) {
  logTraceInWith4Arguments("sourceTerm(...)", x, h, t, dt);

  // @todo implement and ensure that all entries of S are properly set
  for (int i = 0; i < NumberOfUnknowns; i++) {
    S[i] = 0.0;
  }

  logTraceOut("sourceTerm(...)");
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] double* __restrict__                         S // S[{{NUMBER_OF_UNKNOWNS}}]
) {
  logTraceInWith4Arguments("sourceTerm(...)", x, h, t, dt);

  // @todo implement and ensure that all entries of S are properly set
  for (int i = 0; i < NumberOfUnknowns; i++) {
    S[i] = 0.0;
  }

  logTraceOut("sourceTerm(...)");
}
{% endif %}

{% if RIEMANN_SOLVER_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
GPUCallableMethod double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::solveRiemannProblem(
  [[maybe_unused]] const double* __restrict__                    QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* __restrict__                    QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* __restrict__                    FR, // FR[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] const double* __restrict__                    FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] const double* __restrict__                    LR, // LR[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] const double* __restrict__                    LL, // LL[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&  xR,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&  xL,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&  h,
  [[maybe_unused]] double                                        t,
  [[maybe_unused]] double                                        dt,
  [[maybe_unused]] int                                           normal,
  [[maybe_unused]] double* __restrict__                          APDQ, // APDQ[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] double* __restrict__                          AMDQ, // AMDQ[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] Offloadable
) {
  logTraceInWith4Arguments("solveRiemannProblem(...)", xR, xL, t, normal);
  // @todo implement
  logTraceOut("solveRiemannProblem(...)");
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}

{% if RIEMANN_SOLVER_IMPLEMENTATION=="<user-defined>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::solveRiemannProblem(
  [[maybe_unused]] const double* __restrict__                    QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* __restrict__                    QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* __restrict__                    FR, // FR[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] const double* __restrict__                    FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] const double* __restrict__                    LR, // LR[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] const double* __restrict__                    LL, // LL[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&  xR,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&  xL,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&  h,
  [[maybe_unused]] double                                        t,
  [[maybe_unused]] double                                        dt,
  [[maybe_unused]] int                                           normal,
  [[maybe_unused]] double* __restrict__                          APDQ, // APDQ[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] double* __restrict__                          AMDQ // AMDQ[{{NUMBER_OF_UNKNOWNS}}]
) {
  logTraceInWith4Arguments("solveRiemannProblem(...)", xR, xL, t, normal);
  // @todo implement
  logTraceOut("solveRiemannProblem(...)");
}
{% endif %}
""",
        undefined=jinja2.DebugUndefined,
    )

    d = {}
    d["FLUX_IMPLEMENTATION"] = flux_implementation
    d["NCP_IMPLEMENTATION"] = ncp_implementation
    d["EIGENVALUES_IMPLEMENTATION"] = eigenvalues_implementation
    d["RIEMANN_SOLVER_IMPLEMENTATION"] = riemann_solver_implementation
    d["SOURCE_TERM_IMPLEMENTATION"] = source_term_implementation
    d["STATELESS_PDE_TERMS"] = pde_terms_without_state
    return Template.render(**d)
