// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% macro ComputeFluxesAndPerformTimeStep() %}
  const auto volumeSize = ::exahype2::fv::getVolumeSize(patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}});
  const auto volumeIndex = ::exahype2::volumeIndex(x, y {% if DIMENSIONS==3 %}, z {% endif %});
  const auto volumeCentre = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}}, volumeIndex);

{% if DIMENSIONS==2 %}
  const {{COMPUTE_PRECISION}}* const QInCenter = &QIn[(volumeIndex(1) + {{HALO_SIZE}}) * {{PATCH_CARDINALITY_WITH_HALO_1D}} + (volumeIndex(0) + {{HALO_SIZE}}) * {{NUMBER_OF_DATA}}];
  {{COMPUTE_PRECISION}}* const QOutCenter = &QOut[(volumeIndex(1)) * {{PATCH_CARDINALITY_1D}} + (volumeIndex(0)) * {{NUMBER_OF_DATA}}];
{% else %}
  const {{COMPUTE_PRECISION}}* const QInCenter = &QIn[(volumeIndex(2) + {{HALO_SIZE}}) * {{PATCH_CARDINALITY_WITH_HALO_2D}} + (volumeIndex(1) + {{HALO_SIZE}}) * {{PATCH_CARDINALITY_WITH_HALO_1D}} + (volumeIndex(0) + {{HALO_SIZE}}) * {{NUMBER_OF_DATA}}];
  {{COMPUTE_PRECISION}}* const QOutCenter = &QOut[(volumeIndex(2)) * {{PATCH_CARDINALITY_2D}} + (volumeIndex(1)) * {{PATCH_CARDINALITY_1D}} + (volumeIndex(0)) * {{NUMBER_OF_DATA}}];
{% endif %}

  SIMD()
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
    QOutCenter[unknown] = QInCenter[unknown];
  }

{% if USE_SOURCE %}
  // ===================== Source Term =====================
  {
    {{COMPUTE_PRECISION}} sources[{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(ALIGNMENT)));
{% if STATELESS_SOURCE_TERM %}
    {{FULL_QUALIFIED_SOLVER_NAME}}::sourceTerm(QInCenter, volumeCentre, volumeSize, timeStamp, timeStepSize, sources);
{% else %}
    solver.sourceTerm(QInCenter, volumeCentre, volumeSize, timeStamp, timeStepSize, sources);
{% endif %}

    SIMD(aligned(sources:ALIGNMENT))
    for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; ++unknown) {
      QOutCenter[unknown] += timeStepSize * sources[unknown];
    }
  }
{% endif %}

{% if USE_DIFFUSIVE_SOURCE %}
  // ===================== Diffusive Source Term =====================
  {
    {{COMPUTE_PRECISION}} gradQ[{{DIMENSIONS}} * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}})] __attribute__((aligned(ALIGNMENT)));
    for (int normal = 0; normal < {{DIMENSIONS}}; ++normal) {
      auto volumeIndexRight = volumeIndex;
      volumeIndexRight(normal) += {{HALO_SIZE}};

{% if DIMENSIONS==2 %}
      const {{COMPUTE_PRECISION}}* const QInRight = &QIn[(volumeIndexRight(1) + {{HALO_SIZE}}) * {{PATCH_CARDINALITY_WITH_HALO_1D}} + (volumeIndexRight(0) + {{HALO_SIZE}}) * {{NUMBER_OF_DATA}}];
{% else %}
      const {{COMPUTE_PRECISION}}* const QInRight = &QIn[(volumeIndexRight(2) + {{HALO_SIZE}}) * {{PATCH_CARDINALITY_WITH_HALO_2D}} + (volumeIndexRight(1) + {{HALO_SIZE}}) * {{PATCH_CARDINALITY_WITH_HALO_1D}} + (volumeIndexRight(0) + {{HALO_SIZE}}) * {{NUMBER_OF_DATA}}];
{% endif %}

      const {{COMPUTE_PRECISION}} invDx = static_cast<{{COMPUTE_PRECISION}}>(0.5) * ({{NUMBER_OF_VOLUMES_PER_AXIS}} / patchSize(normal));
      SIMD(aligned(gradQ:ALIGNMENT))
      for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; ++unknown) {
        gradQ[normal * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}) + unknown] = invDx * (QInRight[unknown] - QInCenter[unknown]);
      }
    }

    {{COMPUTE_PRECISION}} diffSource[{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(ALIGNMENT)));
{% if STATELESS_DIFFUSIVE_SOURCE %}
    {{FULL_QUALIFIED_SOLVER_NAME}}::diffusiveSourceTerm(QInCenter, gradQ, volumeCentre, volumeSize, timeStamp, timeStepSize, diffSource);
{% else %}
    solver.diffusiveSourceTerm(QInCenter, gradQ, volumeCentre, volumeSize, timeStamp, timeStepSize, diffSource);
{% endif %}

    SIMD(aligned(diffSource:ALIGNMENT))
    for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; ++unknown) {
      QOutCenter[unknown] += timeStepSize * diffSource[unknown];
    }
  }
{% endif %}

  for (int normal = 0; normal < {{DIMENSIONS}}; normal++) {
    auto volumeIndexLeft  = volumeIndex;
    auto volumeIndexRight = volumeIndex;
    volumeIndexLeft (normal) -= {{HALO_SIZE}};
    volumeIndexRight(normal) += {{HALO_SIZE}};

    const auto volumeCentreLeft  = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}}, volumeIndexLeft);
    const auto volumeCentreRight = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}}, volumeIndexRight);

{% if DIMENSIONS==2 %}
    const {{COMPUTE_PRECISION}}* const QInLeft = &QIn[(volumeIndexLeft(1) + {{HALO_SIZE}}) * {{PATCH_CARDINALITY_WITH_HALO_1D}} + (volumeIndexLeft(0) + {{HALO_SIZE}}) * {{NUMBER_OF_DATA}}];
    const {{COMPUTE_PRECISION}}* const QInRight = &QIn[(volumeIndexRight(1) + {{HALO_SIZE}}) * {{PATCH_CARDINALITY_WITH_HALO_1D}} + (volumeIndexRight(0) + {{HALO_SIZE}}) * {{NUMBER_OF_DATA}}];
{% else %}
    const {{COMPUTE_PRECISION}}* const QInLeft = &QIn[(volumeIndexLeft(2) + {{HALO_SIZE}}) * {{PATCH_CARDINALITY_WITH_HALO_2D}} + (volumeIndexLeft(1) + {{HALO_SIZE}}) * {{PATCH_CARDINALITY_WITH_HALO_1D}} + (volumeIndexLeft(0) + {{HALO_SIZE}}) * {{NUMBER_OF_DATA}}];
    const {{COMPUTE_PRECISION}}* const QInRight = &QIn[(volumeIndexRight(2) + {{HALO_SIZE}}) * {{PATCH_CARDINALITY_WITH_HALO_2D}} + (volumeIndexRight(1) + {{HALO_SIZE}}) * {{PATCH_CARDINALITY_WITH_HALO_1D}} + (volumeIndexRight(0) + {{HALO_SIZE}}) * {{NUMBER_OF_DATA}}];
{% endif %}

{% if USE_NCP %}
    // ===================== Nonconservative Product =====================
    {{COMPUTE_PRECISION}} ncpCenter[{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(ALIGNMENT)));
    {{COMPUTE_PRECISION}} ncpLeft  [{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(ALIGNMENT)));
    {
      {{COMPUTE_PRECISION}} averageQCenter[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(ALIGNMENT)));
      {{COMPUTE_PRECISION}} averageQLeft  [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(ALIGNMENT)));
      {{COMPUTE_PRECISION}} deltaQCenter  [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(ALIGNMENT)));
      {{COMPUTE_PRECISION}} deltaQLeft    [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(ALIGNMENT)));

      SIMD(aligned(averageQCenter,averageQLeft,deltaQCenter,deltaQLeft:ALIGNMENT))
      for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; ++unknown) {
        averageQCenter[unknown] = 0.5 * QInCenter[unknown] + 0.5 * QInRight[unknown];
        averageQLeft  [unknown] = 0.5 * QInLeft[unknown]   + 0.5 * QInCenter[unknown];
        deltaQCenter  [unknown] = QInRight[unknown] - QInCenter[unknown];
        deltaQLeft    [unknown] = QInCenter[unknown] - QInLeft[unknown];
        #if !defined(WITH_GPU)
        assertion(std::isfinite(averageQCenter[unknown]));
        assertion(std::isfinite(averageQLeft[unknown]));
        assertion(std::isfinite(deltaQCenter[unknown]));
        assertion(std::isfinite(deltaQLeft[unknown]));
        #endif
      }

{% if STATELESS_NCP %}
      {{FULL_QUALIFIED_SOLVER_NAME}}::nonconservativeProduct(averageQCenter, deltaQCenter, volumeCentre,      volumeSize, timeStamp, timeStepSize, normal, ncpCenter);
      {{FULL_QUALIFIED_SOLVER_NAME}}::nonconservativeProduct(averageQLeft,   deltaQLeft,   volumeCentreLeft,  volumeSize, timeStamp, timeStepSize, normal, ncpLeft);
{% else %}
      solver.nonconservativeProduct(averageQCenter, deltaQCenter, volumeCentre,     volumeSize, timeStamp, timeStepSize, normal, ncpCenter);
      solver.nonconservativeProduct(averageQLeft,   deltaQLeft,   volumeCentreLeft, volumeSize, timeStamp, timeStepSize, normal, ncpLeft);
{% endif %}
    }
{% endif %}

{% if USE_RIEMANN_SOLVER %}
    // ===================== Riemann Solver =====================
    auto faceCentreLeft(volumeCentre);
    faceCentreLeft(normal)  -= static_cast<{{COMPUTE_PRECISION}}>(0.5) * volumeSize(normal);
    auto faceCentreRight(volumeCentre);
    faceCentreRight(normal) += static_cast<{{COMPUTE_PRECISION}}>(0.5) * volumeSize(normal);

    // Flux buffers for the two faces: (Left,Center) and (Center,Right)
    {{COMPUTE_PRECISION}} leftGoingFluxLeftFace [{{NUMBER_OF_UNKNOWNS}}];
    {{COMPUTE_PRECISION}} rightGoingFluxLeftFace[{{NUMBER_OF_UNKNOWNS}}];
    {{COMPUTE_PRECISION}} leftGoingFluxRightFace[{{NUMBER_OF_UNKNOWNS}}];
    {{COMPUTE_PRECISION}} rightGoingFluxRightFace[{{NUMBER_OF_UNKNOWNS}}];

{% if STATELESS_RIEMANN_SOLVER==True %}
    const {{COMPUTE_PRECISION}} maxWaveSpeedLeft = {{FULL_QUALIFIED_SOLVER_NAME}}::solveRiemannProblem(QInLeft, QInCenter, faceCentreLeft, volumeSize, timeStamp, timeStepSize, normal, leftGoingFluxLeftFace, rightGoingFluxLeftFace);
    const {{COMPUTE_PRECISION}} maxWaveSpeedRight = {{FULL_QUALIFIED_SOLVER_NAME}}::solveRiemannProblem(QInCenter, QInRight, faceCentreRight, volumeSize, timeStamp, timeStepSize, normal, leftGoingFluxRightFace, rightGoingFluxRightFace);
{% else %}
    const {{COMPUTE_PRECISION}} maxWaveSpeedLeft = solver.solveRiemannProblem(QInLeft, QInCenter, faceCentreLeft, volumeSize, timeStamp, timeStepSize, normal, leftGoingFluxLeftFace, rightGoingFluxLeftFace);
    const {{COMPUTE_PRECISION}} maxWaveSpeedRight = solver.solveRiemannProblem(QInCenter, QInRight, faceCentreRight, volumeSize, timeStamp, timeStepSize, normal, leftGoingFluxRightFace, rightGoingFluxRightFace);
{% endif %}
    #if !defined(WITH_GPU)
    assertion(std::isfinite(maxWaveSpeedLeft));
    assertion(std::isfinite(maxWaveSpeedRight));
    #endif
{% if COMPUTE_MAX_EIGENVALUE %}
    maxEigenvalueAfterTimeStep = std::fmax(maxEigenvalueAfterTimeStep, std::fmax(maxWaveSpeedLeft, maxWaveSpeedRight));
{% endif %}

    const {{COMPUTE_PRECISION}} scale = (timeStepSize * {{NUMBER_OF_VOLUMES_PER_AXIS}}) / patchSize(normal);
    SIMD(aligned(leftGoingFluxLeftFace,rightGoingFluxLeftFace,leftGoingFluxRightFace,rightGoingFluxRightFace:ALIGNMENT))
    for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; ++unknown) {
      #if !defined(WITH_GPU)
      assertion(std::isfinite(QInLeft[unknown]));
      assertion(std::isfinite(QInCenter[unknown]));
      assertion(std::isfinite(QInRight[unknown]));
      #endif

      // Flux INTO the center from the left face is the right-going flux of (Left,Center)
      {{COMPUTE_PRECISION}} FL = rightGoingFluxLeftFace[unknown];
      // Flux LEAVING the center to the right face is the left-going flux of (Center,Right)
      {{COMPUTE_PRECISION}} FR = leftGoingFluxRightFace[unknown];

{% if USE_NCP %}
      #if !defined(WITH_GPU)
      assertion(std::isfinite(ncpLeft[unknown]));
      assertion(std::isfinite(ncpCenter[unknown]));
      #endif
      FL -= ncpLeft[unknown];
      FR += ncpCenter[unknown];
{% endif %}

      QOutCenter[unknown] += scale * (FL - FR);

      #if !defined(WITH_GPU)
      assertion(std::isfinite(QOutCenter[unknown]));
      nonCriticalAssertion(QOutCenter[unknown] == QOutCenter[unknown]);
      #endif
    } // unknown
{% else %}
    // ===================== Generic Rusanov =====================
{% if USE_FLUX %}
    {{COMPUTE_PRECISION}} fluxesLeft[{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(ALIGNMENT)));
    {{COMPUTE_PRECISION}} fluxesRight[{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(ALIGNMENT)));
    {
{% if STATELESS_FLUX %}
      {{FULL_QUALIFIED_SOLVER_NAME}}::flux(QInLeft, volumeCentreLeft, volumeSize, timeStamp, timeStepSize, normal, fluxesLeft);
      {{FULL_QUALIFIED_SOLVER_NAME}}::flux(QInRight, volumeCentreRight, volumeSize, timeStamp, timeStepSize, normal, fluxesRight);
{% else %}
      solver.flux(QInLeft, volumeCentreLeft, volumeSize, timeStamp, timeStepSize, normal, fluxesLeft);
      solver.flux(QInRight, volumeCentreRight, volumeSize, timeStamp, timeStepSize, normal, fluxesRight);
{% endif %}
    }
{% endif %}

    {{COMPUTE_PRECISION}} maxEigenvalueLeft = 0.0;
    {{COMPUTE_PRECISION}} maxEigenvalueRight = 0.0;
    {
{% if STATELESS_MAX_EIGENVALUE %}
      maxEigenvalueLeft  = {{FULL_QUALIFIED_SOLVER_NAME}}::maxEigenvalue(QInLeft, volumeCentreLeft, volumeSize, timeStamp, timeStepSize, normal);
      maxEigenvalueRight = {{FULL_QUALIFIED_SOLVER_NAME}}::maxEigenvalue(QInRight, volumeCentreRight, volumeSize, timeStamp, timeStepSize, normal);
      const {{COMPUTE_PRECISION}} maxEigenvalueCenter = {{FULL_QUALIFIED_SOLVER_NAME}}::maxEigenvalue(QInCenter, volumeCentre, volumeSize, timeStamp, timeStepSize, normal);
{% else %}
      maxEigenvalueLeft  = solver.maxEigenvalue(QInLeft, volumeCentreLeft, volumeSize, timeStamp, timeStepSize, normal);
      maxEigenvalueRight = solver.maxEigenvalue(QInRight, volumeCentreRight, volumeSize, timeStamp, timeStepSize, normal);
      const {{COMPUTE_PRECISION}} maxEigenvalueCenter = solver.maxEigenvalue(QInCenter, volumeCentre, volumeSize, timeStamp, timeStepSize, normal);
{% endif %}
      maxEigenvalueLeft  = std::fmax(maxEigenvalueLeft, maxEigenvalueCenter);
      maxEigenvalueRight = std::fmax(maxEigenvalueRight, maxEigenvalueCenter);
      #if !defined(WITH_GPU)
      assertion(std::isfinite(maxEigenvalueLeft));
      assertion(std::isfinite(maxEigenvalueRight));
      #endif
    }

    const {{COMPUTE_PRECISION}} scale = (timeStepSize * {{NUMBER_OF_VOLUMES_PER_AXIS}} * static_cast<{{COMPUTE_PRECISION}}>(0.5)) / patchSize(normal);
    SIMD()
    for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; ++unknown) {
      #if !defined(WITH_GPU)
      assertion(std::isfinite(QInLeft[unknown]));
      assertion(std::isfinite(QInCenter[unknown]));
      assertion(std::isfinite(QInRight[unknown]));
      #endif

      {{COMPUTE_PRECISION}} FL = -maxEigenvalueLeft  * (QInCenter[unknown] - QInLeft[unknown]);
      {{COMPUTE_PRECISION}} FR = -maxEigenvalueRight * (QInRight[unknown] - QInCenter[unknown]);

{% if USE_FLUX %}
      #if !defined(WITH_GPU)
      assertion(std::isfinite(fluxesLeft[unknown]));
      assertion(std::isfinite(fluxesRight[unknown]));
      #endif
      FL += fluxesLeft[unknown];
      FR += fluxesRight[unknown];
{% endif %}

{% if USE_NCP %}
      #if !defined(WITH_GPU)
      assertion(std::isfinite(ncpLeft[unknown]));
      assertion(std::isfinite(ncpCenter[unknown]));
      #endif
      FL -= ncpLeft[unknown];
      FR += ncpCenter[unknown];
{% endif %}

      QOutCenter[unknown] += scale * (FL - FR);

      #if !defined(WITH_GPU)
      assertion(std::isfinite(QOutCenter[unknown]));
      nonCriticalAssertion(QOutCenter[unknown] == QOutCenter[unknown]);
      #endif
    } // unknown
{% endif %}
  } // end loop over directions

{% if COMPUTE_MAX_EIGENVALUE and not USE_RIEMANN_SOLVER %}
  UNROLL(DIMENSIONS)
  for (int normal = 0; normal < {{DIMENSIONS}}; normal++) {
    maxEigenvalueAfterTimeStep = std::fmax(
      maxEigenvalueAfterTimeStep,
{% if STATELESS_MAX_EIGENVALUE %}
      {{FULL_QUALIFIED_SOLVER_NAME}}::maxEigenvalue(QOutCenter, volumeCentre, volumeSize, timeStamp, timeStepSize, normal)
{% else %}
      solver.maxEigenvalue(QOutCenter, volumeCentre, volumeSize, timeStamp, timeStepSize, normal)
{% endif %}
    );
  }
{% endif %}
{% endmacro %}
