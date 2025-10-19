// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% macro ComputeFluxesAndPerformTimeStep() %}
  const auto volumeSize = ::exahype2::fv::getVolumeSize(patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}});
  const auto volumeIndex = ::exahype2::volumeIndex(x, y {% if DIMENSIONS==3 %}, z {% endif %});
  const auto volumeCentre = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}}, volumeIndex);

{% if DIMENSIONS==2 %}
  const {{STORAGE_PRECISION}}* const QInCenter = &QIn[(volumeIndex(1) + {{HALO_SIZE}}) * {{PATCH_CARDINALITY_WITH_HALO_1D}} + (volumeIndex(0) + {{HALO_SIZE}}) * {{NUMBER_OF_DATA}}];
  {{STORAGE_PRECISION}}* const QOutCenter = &QOut[(volumeIndex(1)) * {{PATCH_CARDINALITY_1D}} + (volumeIndex(0)) * {{NUMBER_OF_DATA}}];
{% else %}
  const {{STORAGE_PRECISION}}* const QInCenter = &QIn[(volumeIndex(2) + {{HALO_SIZE}}) * {{PATCH_CARDINALITY_WITH_HALO_2D}} + (volumeIndex(1) + {{HALO_SIZE}}) * {{PATCH_CARDINALITY_WITH_HALO_1D}} + (volumeIndex(0) + {{HALO_SIZE}}) * {{NUMBER_OF_DATA}}];
  {{STORAGE_PRECISION}}* const QOutCenter = &QOut[(volumeIndex(2)) * {{PATCH_CARDINALITY_2D}} + (volumeIndex(1)) * {{PATCH_CARDINALITY_1D}} + (volumeIndex(0)) * {{NUMBER_OF_DATA}}];
{% endif %}

  #if defined(WITH_GPU)
  {{STORAGE_PRECISION}} QC[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(ALIGNMENT)));
  {{STORAGE_PRECISION}} QL[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(ALIGNMENT)));
  {{STORAGE_PRECISION}} QR[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(ALIGNMENT)));
  {{STORAGE_PRECISION}} QO[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(ALIGNMENT)));

  SIMD(aligned(QC:ALIGNMENT))
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
    QC[unknown] = QInCenter[unknown];
  }
  #else
  const {{STORAGE_PRECISION}}* const QC = QInCenter;
  {{STORAGE_PRECISION}}* const QO = QOutCenter;
  assertion(QC);
  assertion(QO);
  #endif

  SIMD(/*aligned(QO,QC:ALIGNMENT)*/)
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
    QO[unknown] = QC[unknown];
  }

{% if USE_SOURCE %}
  // ===================== Source Term =====================
  {
    {{COMPUTE_PRECISION}} sources[{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(ALIGNMENT)));
{% if STATELESS_SOURCE_TERM %}
    {{FULL_QUALIFIED_SOLVER_NAME}}::sourceTerm(QC, volumeCentre, volumeSize, timeStamp, timeStepSize, sources);
{% else %}
    solver.sourceTerm(QC, volumeCentre, volumeSize, timeStamp, timeStepSize, sources);
{% endif %}

    SIMD(aligned(/*QO,*/sources:ALIGNMENT))
    for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; ++unknown) {
      QO[unknown] += timeStepSize * sources[unknown];
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

      #if !defined(WITH_GPU)
      assertion(QInRight);
      #endif

      const {{COMPUTE_PRECISION}} invDx = static_cast<{{COMPUTE_PRECISION}}>(0.5) * ({{NUMBER_OF_VOLUMES_PER_AXIS}} / patchSize(normal));
      SIMD(aligned(gradQ:ALIGNMENT))
      for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; ++unknown) {
        gradQ[normal * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}) + unknown] = invDx * (QInRight[unknown] - QC[unknown]);
      }
    }

    {{COMPUTE_PRECISION}} diffSource[{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(ALIGNMENT)));
{% if STATELESS_DIFFUSIVE_SOURCE_TERM %}
    {{FULL_QUALIFIED_SOLVER_NAME}}::diffusiveSourceTerm(QC, gradQ, volumeCentre, volumeSize, timeStamp, timeStepSize, diffSource);
{% else %}
    solver.diffusiveSourceTerm(QC, gradQ, volumeCentre, volumeSize, timeStamp, timeStepSize, diffSource);
{% endif %}

    SIMD(aligned(/*QO,*/diffSource:ALIGNMENT))
    for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; ++unknown) {
      QO[unknown] += timeStepSize * diffSource[unknown];
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
    const auto indexLeft = (volumeIndexLeft(1) + {{HALO_SIZE}}) * {{PATCH_CARDINALITY_WITH_HALO_1D}} + (volumeIndexLeft(0) + {{HALO_SIZE}}) * {{NUMBER_OF_DATA}};
    const auto indexRight = (volumeIndexRight(1) + {{HALO_SIZE}}) * {{PATCH_CARDINALITY_WITH_HALO_1D}} + (volumeIndexRight(0) + {{HALO_SIZE}}) * {{NUMBER_OF_DATA}};
{% else %}
    const auto indexLeft = (volumeIndexLeft(2) + {{HALO_SIZE}}) * {{PATCH_CARDINALITY_WITH_HALO_2D}} + (volumeIndexLeft(1) + {{HALO_SIZE}}) * {{PATCH_CARDINALITY_WITH_HALO_1D}} + (volumeIndexLeft(0) + {{HALO_SIZE}}) * {{NUMBER_OF_DATA}};
    const auto indexRight = (volumeIndexRight(2) + {{HALO_SIZE}}) * {{PATCH_CARDINALITY_WITH_HALO_2D}} + (volumeIndexRight(1) + {{HALO_SIZE}}) * {{PATCH_CARDINALITY_WITH_HALO_1D}} + (volumeIndexRight(0) + {{HALO_SIZE}}) * {{NUMBER_OF_DATA}};
{% endif %}

    #if defined(WITH_GPU)
    const {{STORAGE_PRECISION}}* const QInLeft = &QIn[indexLeft];
    const {{STORAGE_PRECISION}}* const QInRight = &QIn[indexRight];
    SIMD(aligned(QL:ALIGNMENT))
    for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
      QL[unknown] = QInLeft[unknown];
    }
    SIMD(aligned(QR:ALIGNMENT))
    for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
      QR[unknown] = QInRight[unknown];
    }
    #else
    const {{STORAGE_PRECISION}}* const QL = &QIn[indexLeft];
    const {{STORAGE_PRECISION}}* const QR = &QIn[indexRight];
    assertion(QL);
    assertion(QR);
    #endif

{% if USE_RIEMANN_SOLVER %}
    // ===================== Riemann Solver =====================
    auto faceCentreLeft(volumeCentre);
    faceCentreLeft(normal)  -= static_cast<{{COMPUTE_PRECISION}}>(0.5) * volumeSize(normal);
    auto faceCentreRight(volumeCentre);
    faceCentreRight(normal) += static_cast<{{COMPUTE_PRECISION}}>(0.5) * volumeSize(normal);

    // Flux buffers for the two faces: (Left,Center) and (Center,Right)
    {{COMPUTE_PRECISION}} leftGoingFluxLeftFace[{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(ALIGNMENT)));
    {{COMPUTE_PRECISION}} rightGoingFluxLeftFace[{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(ALIGNMENT)));
    {{COMPUTE_PRECISION}} leftGoingFluxRightFace[{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(ALIGNMENT)));
    {{COMPUTE_PRECISION}} rightGoingFluxRightFace[{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(ALIGNMENT)));

{% if STATELESS_RIEMANN_SOLVER==True %}
    const {{COMPUTE_PRECISION}} maxWaveSpeedLeft = {{FULL_QUALIFIED_SOLVER_NAME}}::solveRiemannProblem(QL, QC, faceCentreLeft, volumeSize, timeStamp, timeStepSize, normal, leftGoingFluxLeftFace, rightGoingFluxLeftFace);
    const {{COMPUTE_PRECISION}} maxWaveSpeedRight = {{FULL_QUALIFIED_SOLVER_NAME}}::solveRiemannProblem(QC, QR, faceCentreRight, volumeSize, timeStamp, timeStepSize, normal, leftGoingFluxRightFace, rightGoingFluxRightFace);
{% else %}
    const {{COMPUTE_PRECISION}} maxWaveSpeedLeft = solver.solveRiemannProblem(QL, QC, faceCentreLeft, volumeSize, timeStamp, timeStepSize, normal, leftGoingFluxLeftFace, rightGoingFluxLeftFace);
    const {{COMPUTE_PRECISION}} maxWaveSpeedRight = solver.solveRiemannProblem(QC, QR, faceCentreRight, volumeSize, timeStamp, timeStepSize, normal, leftGoingFluxRightFace, rightGoingFluxRightFace);
{% endif %}
    #if !defined(WITH_GPU)
    assertion(std::isfinite(maxWaveSpeedLeft));
    assertion(std::isfinite(maxWaveSpeedRight));
    #endif
{% if COMPUTE_MAX_EIGENVALUE %}
    maxEigenvalueAfterTimeStep = std::fmax(maxEigenvalueAfterTimeStep, std::fmax(maxWaveSpeedLeft, maxWaveSpeedRight));
{% endif %}

    const {{COMPUTE_PRECISION}} scale = (timeStepSize * {{NUMBER_OF_VOLUMES_PER_AXIS}}) / patchSize(normal);
    SIMD(aligned(leftGoingFluxLeftFace,rightGoingFluxLeftFace,leftGoingFluxRightFace,rightGoingFluxRightFace/*,QC,QL,QR,QO*/:ALIGNMENT))
    for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; ++unknown) {
      #if !defined(WITH_GPU)
      assertion(std::isfinite(QL[unknown]));
      assertion(std::isfinite(QC[unknown]));
      assertion(std::isfinite(QR[unknown]));
      #endif

      // Flux into the center from the left face is the right-going flux of (Left,Center)
      const {{COMPUTE_PRECISION}} FL = rightGoingFluxLeftFace[unknown];
      // Flux leaving the center to the right face is the left-going flux of (Center,Right)
      const {{COMPUTE_PRECISION}} FR = leftGoingFluxRightFace[unknown];

      QO[unknown] += scale * (FL - FR);

      #if !defined(WITH_GPU)
      assertion(std::isfinite(QO[unknown]));
      nonCriticalAssertion(QO[unknown] == QO[unknown]);
      #endif
    } // unknown
{% else %}
    // ===================== Generic Rusanov =====================
{% if USE_FLUX %}
    // ===================== Flux =====================
    {{COMPUTE_PRECISION}} fluxesLeft[{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(ALIGNMENT)));
    {{COMPUTE_PRECISION}} fluxesRight[{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(ALIGNMENT)));
    {
{% if STATELESS_FLUX %}
      {{FULL_QUALIFIED_SOLVER_NAME}}::flux(QL, volumeCentreLeft, volumeSize, timeStamp, timeStepSize, normal, fluxesLeft);
      {{FULL_QUALIFIED_SOLVER_NAME}}::flux(QR, volumeCentreRight, volumeSize, timeStamp, timeStepSize, normal, fluxesRight);
{% else %}
      solver.flux(QL, volumeCentreLeft, volumeSize, timeStamp, timeStepSize, normal, fluxesLeft);
      solver.flux(QR, volumeCentreRight, volumeSize, timeStamp, timeStepSize, normal, fluxesRight);
{% endif %}
    }
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

      SIMD(aligned(averageQCenter,averageQLeft,deltaQCenter,deltaQLeft/*,QC,QL,QR*/:ALIGNMENT))
      for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; ++unknown) {
        averageQCenter[unknown] = 0.5 * QC[unknown] + 0.5 * QR[unknown];
        averageQLeft  [unknown] = 0.5 * QL[unknown] + 0.5 * QC[unknown];
        deltaQCenter  [unknown] = QR[unknown] - QC[unknown];
        deltaQLeft    [unknown] = QC[unknown] - QL[unknown];
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

    {{COMPUTE_PRECISION}} maxEigenvalueLeft = 0.0;
    {{COMPUTE_PRECISION}} maxEigenvalueRight = 0.0;
    {
{% if STATELESS_MAX_EIGENVALUE %}
      maxEigenvalueLeft  = {{FULL_QUALIFIED_SOLVER_NAME}}::maxEigenvalue(QL, volumeCentreLeft, volumeSize, timeStamp, timeStepSize, normal);
      maxEigenvalueRight = {{FULL_QUALIFIED_SOLVER_NAME}}::maxEigenvalue(QR, volumeCentreRight, volumeSize, timeStamp, timeStepSize, normal);
      const {{COMPUTE_PRECISION}} maxEigenvalueCenter = {{FULL_QUALIFIED_SOLVER_NAME}}::maxEigenvalue(QC, volumeCentre, volumeSize, timeStamp, timeStepSize, normal);
{% else %}
      maxEigenvalueLeft  = solver.maxEigenvalue(QL, volumeCentreLeft, volumeSize, timeStamp, timeStepSize, normal);
      maxEigenvalueRight = solver.maxEigenvalue(QR, volumeCentreRight, volumeSize, timeStamp, timeStepSize, normal);
      const {{COMPUTE_PRECISION}} maxEigenvalueCenter = solver.maxEigenvalue(QC, volumeCentre, volumeSize, timeStamp, timeStepSize, normal);
{% endif %}
      maxEigenvalueLeft  = std::fmax(maxEigenvalueLeft, maxEigenvalueCenter);
      maxEigenvalueRight = std::fmax(maxEigenvalueRight, maxEigenvalueCenter);
      #if !defined(WITH_GPU)
      assertion(std::isfinite(maxEigenvalueLeft));
      assertion(std::isfinite(maxEigenvalueRight));
      #endif
    }

    const {{COMPUTE_PRECISION}} scale = (timeStepSize * {{NUMBER_OF_VOLUMES_PER_AXIS}} * static_cast<{{COMPUTE_PRECISION}}>(0.5)) / patchSize(normal);
    //SIMD(aligned(QC,QL,QR,QO:ALIGNMENT))
    for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; ++unknown) {
      #if !defined(WITH_GPU)
      assertion(std::isfinite(QL[unknown]));
      assertion(std::isfinite(QC[unknown]));
      assertion(std::isfinite(QR[unknown]));
      #endif

      {{COMPUTE_PRECISION}} FL = -maxEigenvalueLeft  * (QC[unknown] - QL[unknown]);
      {{COMPUTE_PRECISION}} FR = -maxEigenvalueRight * (QR[unknown] - QC[unknown]);

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

      QO[unknown] += scale * (FL - FR);

      #if !defined(WITH_GPU)
      assertion(std::isfinite(QO[unknown]));
      nonCriticalAssertion(QO[unknown] == QO[unknown]);
      #endif
    } // unknown
{% endif %}
  } // end loop over directions

{% if COMPUTE_MAX_EIGENVALUE and not USE_RIEMANN_SOLVER %}
  UNROLL({{DIMENSIONS}})
  for (int normal = 0; normal < {{DIMENSIONS}}; normal++) {
    maxEigenvalueAfterTimeStep = std::fmax(
      maxEigenvalueAfterTimeStep,
{% if STATELESS_MAX_EIGENVALUE %}
      {{FULL_QUALIFIED_SOLVER_NAME}}::maxEigenvalue(QO, volumeCentre, volumeSize, timeStamp, timeStepSize, normal)
{% else %}
      solver.maxEigenvalue(QO, volumeCentre, volumeSize, timeStamp, timeStepSize, normal)
{% endif %}
    );
  }
{% endif %}

  #if defined(WITH_GPU)
  SIMD(aligned(QO:ALIGNMENT))
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
    QOutCenter[unknown] = QO[unknown];
  }
  #endif
{% endmacro %}
