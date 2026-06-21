// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% macro ComputeFluxesAndPerformTimeStep(enable_trace=False) %}
  auto volumeIndex = ::exahype2::fv::toVolumeIndex(x, y {% if DIMENSIONS==3 %}, z {% endif %});
  auto [volumeSize, volumeCentre] = ::exahype2::fv::getVolumeSizeAndCentre<DIMENSIONS, {{NUMBER_OF_VOLUMES_PER_AXIS}}>(
    patchCentre, patchSize, volumeIndex
  );

{% if DIMENSIONS==2 %}
  const {{STORAGE_PRECISION}}* const NOALIAS QInCenter  = &QIn [volumeIndex(1) * {{PATCH_CARDINALITY_1D}} + volumeIndex(0) * {{NUMBER_OF_DATA}}];
  {{STORAGE_PRECISION}}*       const NOALIAS QOutCenter = &QOut[volumeIndex(1) * {{PATCH_CARDINALITY_1D}} + volumeIndex(0) * {{NUMBER_OF_DATA}}];
{% else %}
  const {{STORAGE_PRECISION}}* const NOALIAS QInCenter  = &QIn [volumeIndex(2) * {{PATCH_CARDINALITY_2D}} + volumeIndex(1) * {{PATCH_CARDINALITY_1D}} + volumeIndex(0) * {{NUMBER_OF_DATA}}];
  {{STORAGE_PRECISION}}*       const NOALIAS QOutCenter = &QOut[volumeIndex(2) * {{PATCH_CARDINALITY_2D}} + volumeIndex(1) * {{PATCH_CARDINALITY_1D}} + volumeIndex(0) * {{NUMBER_OF_DATA}}];
{% endif %}

  #if defined(PEANO_WITH_GPU_BACKEND)
  {{STORAGE_PRECISION}} QC[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
  {{STORAGE_PRECISION}} QL[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
  {{STORAGE_PRECISION}} QR[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
  {{STORAGE_PRECISION}} QO[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));

  SIMD(aligned(QC:PEANO_WITH_ALIGNMENT))
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
    QC[unknown] = QInCenter[unknown];
  }
  #else
  const {{STORAGE_PRECISION}}* const NOALIAS QC = QInCenter;
  {{STORAGE_PRECISION}}*       const NOALIAS QO = QOutCenter;
  assertion(QC);
  assertion(QO);
  #endif

  // QO = QC (all unknowns + aux vars).
  SIMD(/*aligned(QO,QC:PEANO_WITH_ALIGNMENT)*/)
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
    QO[unknown] = QC[unknown];
  }

{% if USE_SOURCE %}
  // ===================== Source Term =====================
{% if enable_trace %}
  traceEnter("Source term evaluation");
{% endif %}
  {
    {{COMPUTE_PRECISION}} sources[{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
{% if STATELESS_SOURCE_TERM %}
    {{FULL_QUALIFIED_SOLVER_NAME}}::sourceTerm(QC, volumeCentre, volumeSize, timeStamp, timeStepSize, sources);
{% else %}
    solver.sourceTerm(QC, volumeCentre, volumeSize, timeStamp, timeStepSize, sources);
{% endif %}

    SIMD(aligned(/*QO,*/sources:PEANO_WITH_ALIGNMENT))
    for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; ++unknown) {
      QO[unknown] += timeStepSize * sources[unknown];
    }
  }
{% if enable_trace %}
  traceLeave("Source term evaluation");
{% endif %}
{% endif %}

{% if USE_DIFFUSIVE_SOURCE %}
  // ===================== Diffusive Source Term =====================
{% if enable_trace %}
  traceEnter("Diffusive source term evaluation");
{% endif %}
  {
    {{COMPUTE_PRECISION}} gradQ[{{DIMENSIONS}} * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}})] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
    for (int normal = 0; normal < {{DIMENSIONS}}; ++normal) {
      // Forward difference: needs the right neighbour. Interior -> QIn;
      // otherwise the right face of this patch (faceQIn[normal + DIMENSIONS]).
      const {{STORAGE_PRECISION}}* NOALIAS QInRight = nullptr;
      if (volumeIndex(normal) + {{HALO_SIZE}} < {{NUMBER_OF_VOLUMES_PER_AXIS}}) {
        auto volumeIndexRight = volumeIndex;
        volumeIndexRight(normal) += {{HALO_SIZE}};
{% if DIMENSIONS==2 %}
        QInRight = &QIn[volumeIndexRight(1) * {{PATCH_CARDINALITY_1D}} + volumeIndexRight(0) * {{NUMBER_OF_DATA}}];
{% else %}
        QInRight = &QIn[volumeIndexRight(2) * {{PATCH_CARDINALITY_2D}} + volumeIndexRight(1) * {{PATCH_CARDINALITY_1D}} + volumeIndexRight(0) * {{NUMBER_OF_DATA}}];
{% endif %}
      } else {
        tarch::la::Vector<DIMENSIONS, int> overlapCell = volumeIndex;
        overlapCell(normal) = volumeIndex(normal) + 2 * {{HALO_SIZE}} - {{NUMBER_OF_VOLUMES_PER_AXIS}};
        const int overlapIdx = toolbox::blockstructured::serialiseVoxelIndexInOverlap(
          overlapCell, {{NUMBER_OF_VOLUMES_PER_AXIS}}, {{HALO_SIZE}}, normal
        );
        QInRight = &faceQIn[normal + {{DIMENSIONS}}][overlapIdx * {{NUMBER_OF_DATA}}];
      }

      const {{COMPUTE_PRECISION}} invDx = static_cast<{{COMPUTE_PRECISION}}>(0.5) * ({{NUMBER_OF_VOLUMES_PER_AXIS}} / patchSize(normal));
      SIMD(aligned(gradQ:PEANO_WITH_ALIGNMENT))
      for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; ++unknown) {
        gradQ[normal * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}) + unknown] = invDx * (QInRight[unknown] - QC[unknown]);
      }
    }

    {{COMPUTE_PRECISION}} diffSource[{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
{% if STATELESS_DIFFUSIVE_SOURCE_TERM %}
    {{FULL_QUALIFIED_SOLVER_NAME}}::diffusiveSourceTerm(QC, gradQ, volumeCentre, volumeSize, timeStamp, timeStepSize, diffSource);
{% else %}
    solver.diffusiveSourceTerm(QC, gradQ, volumeCentre, volumeSize, timeStamp, timeStepSize, diffSource);
{% endif %}

    SIMD(aligned(/*QO,*/diffSource:PEANO_WITH_ALIGNMENT))
    for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; ++unknown) {
      QO[unknown] += timeStepSize * diffSource[unknown];
    }
  }
{% if enable_trace %}
  traceLeave("Diffusive source term evaluation");
{% endif %}
{% endif %}

  for (int normal = 0; normal < {{DIMENSIONS}}; normal++) {
    auto volumeIndexLeft  = volumeIndex;
    auto volumeIndexRight = volumeIndex;
    volumeIndexLeft (normal) -= {{HALO_SIZE}};
    volumeIndexRight(normal) += {{HALO_SIZE}};

    auto [volumeSizeLeft, volumeCentreLeft] = ::exahype2::fv::getVolumeSizeAndCentre<DIMENSIONS, {{NUMBER_OF_VOLUMES_PER_AXIS}}>(
      patchCentre, patchSize, volumeIndexLeft
    );

    auto [volumeSizeRight, volumeCentreRight] = ::exahype2::fv::getVolumeSizeAndCentre<DIMENSIONS, {{NUMBER_OF_VOLUMES_PER_AXIS}}>(
      patchCentre, patchSize, volumeIndexRight
    );

    // ----- Fetch QL: left neighbour along `normal`. -----
    const {{STORAGE_PRECISION}}* NOALIAS QInLeft = nullptr;
    if (volumeIndexLeft(normal) >= 0) {
      // Interior.
{% if DIMENSIONS==2 %}
      QInLeft = &QIn[volumeIndexLeft(1) * {{PATCH_CARDINALITY_1D}} + volumeIndexLeft(0) * {{NUMBER_OF_DATA}}];
{% else %}
      QInLeft = &QIn[volumeIndexLeft(2) * {{PATCH_CARDINALITY_2D}} + volumeIndexLeft(1) * {{PATCH_CARDINALITY_1D}} + volumeIndexLeft(0) * {{NUMBER_OF_DATA}}];
{% endif %}
    } else {
      // Halo: left face of this patch == faceQIn[normal].
      tarch::la::Vector<DIMENSIONS, int> overlapCell = volumeIndex;
      overlapCell(normal) = volumeIndex(normal); // in [0, HALO_SIZE)
      const int overlapIdx = toolbox::blockstructured::serialiseVoxelIndexInOverlap(
        overlapCell, {{NUMBER_OF_VOLUMES_PER_AXIS}}, {{HALO_SIZE}}, normal
      );
      QInLeft = &faceQIn[normal][overlapIdx * {{NUMBER_OF_DATA}}];
    }

    // ----- Fetch QR: right neighbour along `normal`. -----
    const {{STORAGE_PRECISION}}* NOALIAS QInRight = nullptr;
    if (volumeIndexRight(normal) < {{NUMBER_OF_VOLUMES_PER_AXIS}}) {
      // Interior.
{% if DIMENSIONS==2 %}
      QInRight = &QIn[volumeIndexRight(1) * {{PATCH_CARDINALITY_1D}} + volumeIndexRight(0) * {{NUMBER_OF_DATA}}];
{% else %}
      QInRight = &QIn[volumeIndexRight(2) * {{PATCH_CARDINALITY_2D}} + volumeIndexRight(1) * {{PATCH_CARDINALITY_1D}} + volumeIndexRight(0) * {{NUMBER_OF_DATA}}];
{% endif %}
    } else {
      // Halo: right face of this patch == faceQIn[normal + DIMENSIONS].
      tarch::la::Vector<DIMENSIONS, int> overlapCell = volumeIndex;
      overlapCell(normal) = volumeIndex(normal) + 2 * {{HALO_SIZE}} - {{NUMBER_OF_VOLUMES_PER_AXIS}};
      const int overlapIdx = toolbox::blockstructured::serialiseVoxelIndexInOverlap(
        overlapCell, {{NUMBER_OF_VOLUMES_PER_AXIS}}, {{HALO_SIZE}}, normal
      );
      QInRight = &faceQIn[normal + {{DIMENSIONS}}][overlapIdx * {{NUMBER_OF_DATA}}];
    }

    #if defined(PEANO_WITH_GPU_BACKEND)
    SIMD(aligned(QL:PEANO_WITH_ALIGNMENT))
    for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
      QL[unknown] = QInLeft[unknown];
    }
    SIMD(aligned(QR:PEANO_WITH_ALIGNMENT))
    for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
      QR[unknown] = QInRight[unknown];
    }
    #else
    const {{STORAGE_PRECISION}}* const NOALIAS QL = QInLeft;
    const {{STORAGE_PRECISION}}* const NOALIAS QR = QInRight;
    assertion(QL);
    assertion(QR);
    #endif

{% if USE_RIEMANN_SOLVER %}
    // ===================== Riemann Solver =====================
{% if enable_trace %}
    traceEnter("Riemann solver");
{% endif %}

    auto faceCentreLeft(volumeCentre);
    faceCentreLeft(normal)  -= static_cast<{{COMPUTE_PRECISION}}>(0.5) * volumeSize(normal);
    auto faceCentreRight(volumeCentre);
    faceCentreRight(normal) += static_cast<{{COMPUTE_PRECISION}}>(0.5) * volumeSize(normal);

    // Flux buffers for the two faces: (Left,Center) and (Center,Right)
    {{COMPUTE_PRECISION}} leftGoingFluxLeftFace  [{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
    {{COMPUTE_PRECISION}} rightGoingFluxLeftFace [{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
    {{COMPUTE_PRECISION}} leftGoingFluxRightFace [{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
    {{COMPUTE_PRECISION}} rightGoingFluxRightFace[{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));

{% if STATELESS_RIEMANN_SOLVER==True %}
    const {{COMPUTE_PRECISION}} maxWaveSpeedLeft  = {{FULL_QUALIFIED_SOLVER_NAME}}::solveRiemannProblem(QL, QC, faceCentreLeft,  volumeSize, timeStamp, timeStepSize, normal, leftGoingFluxLeftFace,  rightGoingFluxLeftFace);
    const {{COMPUTE_PRECISION}} maxWaveSpeedRight = {{FULL_QUALIFIED_SOLVER_NAME}}::solveRiemannProblem(QC, QR, faceCentreRight, volumeSize, timeStamp, timeStepSize, normal, leftGoingFluxRightFace, rightGoingFluxRightFace);
{% else %}
    const {{COMPUTE_PRECISION}} maxWaveSpeedLeft  = solver.solveRiemannProblem(QL, QC, faceCentreLeft,  volumeSize, timeStamp, timeStepSize, normal, leftGoingFluxLeftFace,  rightGoingFluxLeftFace);
    const {{COMPUTE_PRECISION}} maxWaveSpeedRight = solver.solveRiemannProblem(QC, QR, faceCentreRight, volumeSize, timeStamp, timeStepSize, normal, leftGoingFluxRightFace, rightGoingFluxRightFace);
{% endif %}
    assertion(std::isfinite(maxWaveSpeedLeft));
    assertion(std::isfinite(maxWaveSpeedRight));
{% if COMPUTE_MAX_EIGENVALUE %}
    maxEigenvalueAfterTimeStep = std::fmax(maxEigenvalueAfterTimeStep, std::fmax(maxWaveSpeedLeft, maxWaveSpeedRight));
{% endif %}

    const {{COMPUTE_PRECISION}} scale = (timeStepSize * {{NUMBER_OF_VOLUMES_PER_AXIS}}) / patchSize(normal);
    SIMD(aligned(leftGoingFluxLeftFace,rightGoingFluxLeftFace,leftGoingFluxRightFace,rightGoingFluxRightFace/*,QC,QL,QR,QO*/:PEANO_WITH_ALIGNMENT))
    for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; ++unknown) {
      assertion(std::isfinite(QL[unknown]));
      assertion(std::isfinite(QC[unknown]));
      assertion(std::isfinite(QR[unknown]));

      // Flux into the center from the left face is the right-going flux of (Left,Center)
      const {{COMPUTE_PRECISION}} FL = rightGoingFluxLeftFace[unknown];
      // Flux leaving the center to the right face is the left-going flux of (Center,Right)
      const {{COMPUTE_PRECISION}} FR = leftGoingFluxRightFace[unknown];

      QO[unknown] += scale * (FL - FR);

      assertion(std::isfinite(QO[unknown]));
    } // unknown
{% if enable_trace %}
    traceLeave("Riemann solver");
{% endif %}
{% else %}
    // ===================== Generic Rusanov =====================
{% if enable_trace %}
    traceEnter("Generic Rusanov");
{% endif %}

{% if USE_FLUX %}
    // ===================== Flux =====================
{% if enable_trace %}
    traceEnter("Flux evaluation");
{% endif %}
    {{COMPUTE_PRECISION}} fluxesLeft[{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
    {{COMPUTE_PRECISION}} fluxesRight[{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
    {
{% if STATELESS_FLUX %}
      {{FULL_QUALIFIED_SOLVER_NAME}}::flux(QL, volumeCentreLeft,  volumeSize, timeStamp, timeStepSize, normal, fluxesLeft);
      {{FULL_QUALIFIED_SOLVER_NAME}}::flux(QR, volumeCentreRight, volumeSize, timeStamp, timeStepSize, normal, fluxesRight);
{% else %}
      solver.flux(QL, volumeCentreLeft,  volumeSize, timeStamp, timeStepSize, normal, fluxesLeft);
      solver.flux(QR, volumeCentreRight, volumeSize, timeStamp, timeStepSize, normal, fluxesRight);
{% endif %}
    }
{% if enable_trace %}
    traceLeave("Flux evaluation");
{% endif %}
{% endif %}

{% if USE_NCP %}
    // ===================== Nonconservative Product =====================
{% if enable_trace %}
    traceEnter("Nonconservative product evaluation");
{% endif %}
    {{COMPUTE_PRECISION}} ncpCenter[{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
    {{COMPUTE_PRECISION}} ncpLeft  [{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
    {
      {{COMPUTE_PRECISION}} averageQCenter[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
      {{COMPUTE_PRECISION}} averageQLeft  [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
      {{COMPUTE_PRECISION}} deltaQCenter  [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
      {{COMPUTE_PRECISION}} deltaQLeft    [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));

      SIMD(aligned(averageQCenter,averageQLeft,deltaQCenter,deltaQLeft/*,QC,QL,QR*/:PEANO_WITH_ALIGNMENT))
      for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; ++unknown) {
        averageQCenter[unknown] = 0.5 * QC[unknown] + 0.5 * QR[unknown];
        averageQLeft  [unknown] = 0.5 * QL[unknown] + 0.5 * QC[unknown];
        deltaQCenter  [unknown] = QR[unknown] - QC[unknown];
        deltaQLeft    [unknown] = QC[unknown] - QL[unknown];
        assertion(std::isfinite(averageQCenter[unknown]));
        assertion(std::isfinite(averageQLeft[unknown]));
        assertion(std::isfinite(deltaQCenter[unknown]));
        assertion(std::isfinite(deltaQLeft[unknown]));
      }

{% if STATELESS_NCP %}
      {{FULL_QUALIFIED_SOLVER_NAME}}::nonconservativeProduct(averageQCenter, deltaQCenter, volumeCentre,     volumeSize, timeStamp, timeStepSize, normal, ncpCenter);
      {{FULL_QUALIFIED_SOLVER_NAME}}::nonconservativeProduct(averageQLeft,   deltaQLeft,   volumeCentreLeft, volumeSize, timeStamp, timeStepSize, normal, ncpLeft);
{% else %}
      solver.nonconservativeProduct(averageQCenter, deltaQCenter, volumeCentre,     volumeSize, timeStamp, timeStepSize, normal, ncpCenter);
      solver.nonconservativeProduct(averageQLeft,   deltaQLeft,   volumeCentreLeft, volumeSize, timeStamp, timeStepSize, normal, ncpLeft);
{% endif %}
    }
{% if enable_trace %}
    traceLeave("Nonconservative product evaluation");
{% endif %}
{% endif %}

    {{COMPUTE_PRECISION}} maxEigenvalueLeft  = 0.0;
    {{COMPUTE_PRECISION}} maxEigenvalueRight = 0.0;
{% if enable_trace %}
    traceEnter("Max eigenvalue evaluation");
{% endif %}
    {
{% if STATELESS_MAX_EIGENVALUE %}
      maxEigenvalueLeft  = {{FULL_QUALIFIED_SOLVER_NAME}}::maxEigenvalue(QL, volumeCentreLeft,  volumeSize, timeStamp, timeStepSize, normal);
      maxEigenvalueRight = {{FULL_QUALIFIED_SOLVER_NAME}}::maxEigenvalue(QR, volumeCentreRight, volumeSize, timeStamp, timeStepSize, normal);
      const {{COMPUTE_PRECISION}} maxEigenvalueCenter = {{FULL_QUALIFIED_SOLVER_NAME}}::maxEigenvalue(QC, volumeCentre, volumeSize, timeStamp, timeStepSize, normal);
{% else %}
      maxEigenvalueLeft  = solver.maxEigenvalue(QL, volumeCentreLeft,  volumeSize, timeStamp, timeStepSize, normal);
      maxEigenvalueRight = solver.maxEigenvalue(QR, volumeCentreRight, volumeSize, timeStamp, timeStepSize, normal);
      const {{COMPUTE_PRECISION}} maxEigenvalueCenter = solver.maxEigenvalue(QC, volumeCentre, volumeSize, timeStamp, timeStepSize, normal);
{% endif %}
      maxEigenvalueLeft  = std::fmax(maxEigenvalueLeft,  maxEigenvalueCenter);
      maxEigenvalueRight = std::fmax(maxEigenvalueRight, maxEigenvalueCenter);
      assertion(std::isfinite(maxEigenvalueLeft));
      assertion(std::isfinite(maxEigenvalueRight));
    }
{% if enable_trace %}
    traceLeave("Max eigenvalue evaluation");
{% endif %}

    const {{COMPUTE_PRECISION}} scale = (timeStepSize * {{NUMBER_OF_VOLUMES_PER_AXIS}} * static_cast<{{COMPUTE_PRECISION}}>(0.5)) / patchSize(normal);
    //SIMD(aligned(QC,QL,QR,QO:PEANO_WITH_ALIGNMENT)) // TODO: Missing alignment for fluxesLeft and fluxesRight
    UNROLL({{DIMENSIONS}})
    for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; ++unknown) {
      assertion(std::isfinite(QL[unknown]));
      assertion(std::isfinite(QC[unknown]));
      assertion(std::isfinite(QR[unknown]));

      {{COMPUTE_PRECISION}} FL = -maxEigenvalueLeft  * (QC[unknown] - QL[unknown]);
      {{COMPUTE_PRECISION}} FR = -maxEigenvalueRight * (QR[unknown] - QC[unknown]);

{% if USE_FLUX %}
      assertion(std::isfinite(fluxesLeft[unknown]));
      assertion(std::isfinite(fluxesRight[unknown]));
      FL += fluxesLeft[unknown];
      FR += fluxesRight[unknown];
{% endif %}

{% if USE_NCP %}
      assertion(std::isfinite(ncpLeft[unknown]));
      assertion(std::isfinite(ncpCenter[unknown]));
      FL -= ncpLeft[unknown];
      FR += ncpCenter[unknown];
{% endif %}

      QO[unknown] += scale * (FL - FR);

      assertion(std::isfinite(QO[unknown]));
    } // unknown
{% if enable_trace %}
    traceLeave("Generic Rusanov");
{% endif %}
{% endif %}
  } // end loop over directions

{% if COMPUTE_MAX_EIGENVALUE and not USE_RIEMANN_SOLVER %}
{% if enable_trace %}
  traceEnter("Final max eigenvalue evaluation");
{% endif %}
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
{% if enable_trace %}
  traceLeave("Final max eigenvalue evaluation");
{% endif %}
{% endif %}

  #if defined(PEANO_WITH_GPU_BACKEND)
  SIMD(aligned(QO:PEANO_WITH_ALIGNMENT))
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
    QOutCenter[unknown] = QO[unknown];
  }
  #endif

  // ===================== Project QOut onto faces =====================
  for (int normal = 0; normal < {{DIMENSIONS}}; normal++) {
    if (volumeIndex(normal) < {{HALO_SIZE}}) {
      // Left boundary: write to faceQOut[normal]
      tarch::la::Vector<DIMENSIONS, int> overlapCell = volumeIndex;
      overlapCell(normal) = volumeIndex(normal) + {{HALO_SIZE}};
      const int overlapIdx = toolbox::blockstructured::serialiseVoxelIndexInOverlap(
        overlapCell, {{NUMBER_OF_VOLUMES_PER_AXIS}}, {{HALO_SIZE}}, normal
      );
      {{STORAGE_PRECISION}}* const NOALIAS faceQOutData = faceQOut[normal];
      SIMD(/*aligned(faceQOutData,QOutCenter:PEANO_WITH_ALIGNMENT)*/)
      for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
        faceQOutData[overlapIdx * {{NUMBER_OF_DATA}} + unknown] = QOutCenter[unknown];
      }
    }
    if (volumeIndex(normal) >= {{NUMBER_OF_VOLUMES_PER_AXIS}} - {{HALO_SIZE}}) {
      // Right boundary: write to faceQOut[normal + DIMENSIONS]
      tarch::la::Vector<DIMENSIONS, int> overlapCell = volumeIndex;
      overlapCell(normal) = volumeIndex(normal) - ({{NUMBER_OF_VOLUMES_PER_AXIS}} - {{HALO_SIZE}});
      const int overlapIdx = toolbox::blockstructured::serialiseVoxelIndexInOverlap(
        overlapCell, {{NUMBER_OF_VOLUMES_PER_AXIS}}, {{HALO_SIZE}}, normal
      );
      {{STORAGE_PRECISION}}* const NOALIAS faceQOutData = faceQOut[normal + {{DIMENSIONS}}];
      SIMD(/*aligned(faceQOutData,QOutCenter:PEANO_WITH_ALIGNMENT)*/)
      for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
        faceQOutData[overlapIdx * {{NUMBER_OF_DATA}} + unknown] = QOutCenter[unknown];
      }
    }
  }
{% endmacro %}
