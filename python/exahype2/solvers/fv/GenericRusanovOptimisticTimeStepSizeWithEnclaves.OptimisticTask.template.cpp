#include "{{CLASSNAME}}.h"

// user includes
{{INCLUDES}}

#include "exahype2/fv/Generic.h"
#include "exahype2/fv/Rusanov.h"
#include "exahype2/EnclaveBookkeeping.h"
#include "exahype2/EnclaveTask.h"


#include "peano4/utils/Loop.h"


#include <algorithm>


tarch::logging::Log                {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );
int                                {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_optimisticTaskId( peano4::parallel::Tasks::getTaskType("{{NAMESPACE | join("::")}}::{{CLASSNAME}}") );


double* {{NAMESPACE | join("::")}}::{{CLASSNAME}}::copyPatchData( double* __restrict__ patchData) {
  #if Dimensions==2
  constexpr int Number = {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}};
  #else
  constexpr int Number = {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}};
  #endif

  double* result = tarch::allocateMemory(Number,tarch::MemoryLocation::Heap );
  std::copy_n(patchData, Number, result);
  return result;
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(
  const ::peano4::datamanagement::CellMarker& marker,
  double                                      t,
  double                                      dt,
  double                                      predictedTimeStepSize,
  double* __restrict__                        patchData
):
  ::exahype2::EnclaveTask(
    _optimisticTaskId,
    marker,
    t+dt,
    predictedTimeStepSize,
    copyPatchData( patchData ),
    #if Dimensions==2
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}},
    {{NUMBER_OF_INNER_DOUBLE_VALUES_IN_PATCH_2D}},
    #else
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}},
    {{NUMBER_OF_INNER_DOUBLE_VALUES_IN_PATCH_3D}},
    #endif
    [&](double* reconstructedPatch, double* targetPatch, const ::peano4::datamanagement::CellMarker& marker, double t, double dt) -> void {
          {{PREPROCESS_RECONSTRUCTED_PATCH}}

          ::exahype2::fv::copyPatch(
            reconstructedPatch,
            targetPatch,
            {{NUMBER_OF_UNKNOWNS}},
            {{NUMBER_OF_AUXILIARY_VARIABLES}},
            {{NUMBER_OF_VOLUMES_PER_AXIS}}-2,
            1 // halo size
          );

          {% if USE_SPLIT_LOOP %}
          #if Dimensions==2
          ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d_SplitLoop(
          #else
          ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d_SplitLoop(
          #endif
          {% else %}
          #if Dimensions==2
          ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d(
          #else
          ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d(
          #endif
          {% endif %}
            [&](
              const double* __restrict__                   QL,
              const double* __restrict__                   QR,
              const tarch::la::Vector<Dimensions,double>&  x,
              double                                       dx,
              double                                       t,
              double                                       dt,
              int                                          normal,
              double                                       FL[],
              double                                       FR[]
            ) -> void {
            ::exahype2::fv::splitRusanov1d(
              [] (
               const double * __restrict__ Q,
               const tarch::la::Vector<Dimensions,double>&  faceCentre,
               const tarch::la::Vector<Dimensions,double>&  volumeH,
               double                                       t,
               double                                       dt,
               int                                          normal,
               double * __restrict__                        F
              ) -> void {
                {% if FLUX_IMPLEMENTATION=="<none>" %}
                for (int i=0; i<{{NUMBER_OF_UNKNOWNS}}; i++) F[i] = 0.0;
                {% else %}
                repositories::{{SOLVER_INSTANCE}}.flux( Q, faceCentre, volumeH, t, normal, F );
                {% endif %}
              },
              [] (
                const double* __restrict__                   Q,
                const double * __restrict__                  deltaQ,
                const tarch::la::Vector<Dimensions,double>&  faceCentre,
                const tarch::la::Vector<Dimensions,double>&  volumeH,
                double                                       t,
                double                                       dt,
                int                                          normal,
                double                                       BgradQ[]
              ) -> void {
                {% if NCP_IMPLEMENTATION!="<none>" %}
                repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct( Q, deltaQ, faceCentre, volumeH, t, normal, BgradQ );
                {% endif %}
              },
              [] (
                const double* __restrict__                   Q,
                const tarch::la::Vector<Dimensions,double>&  faceCentre,
                const tarch::la::Vector<Dimensions,double>&  volumeH,
                double                                       t,
                double                                       dt,
                int                                          normal
              ) -> double {
                return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
              },
              QL, QR, x, dx, t, dt, normal,
              {{NUMBER_OF_UNKNOWNS}},
              {{NUMBER_OF_AUXILIARY_VARIABLES}},
              FL,FR,
              {% if FLUX_IMPLEMENTATION=="<none>" %}
              true,
              {% else %}
              false,
              {% endif %}
              {% if NCP_IMPLEMENTATION=="<none>" %}
              true
              {% else %}
              false
              {% endif %}
            );
            },
          [&](
            const double * __restrict__                  Q,
            const tarch::la::Vector<Dimensions,double>&  x,
            double                                       dx,
            double                                       t,
            double                                       dt,
            double * __restrict__                        S
          ) -> void {
            repositories::{{SOLVER_INSTANCE}}.sourceTerm(
              Q,
              x, dx, t, dt,
              S
            );
          },
            marker.x(),
            marker.h(),
            t,
            dt,
            {{NUMBER_OF_VOLUMES_PER_AXIS}}-2,
            {{NUMBER_OF_UNKNOWNS}},
            {{NUMBER_OF_AUXILIARY_VARIABLES}},
            reconstructedPatch,
            targetPatch
          );

          {{POSTPROCESS_UPDATED_PATCH}}

          double maxEigenvalue = ::exahype2::fv::maxEigenvalue_AoS(
            [] (
              const double * __restrict__                  Q,
              const tarch::la::Vector<Dimensions,double>&  faceCentre,
              const tarch::la::Vector<Dimensions,double>&  volumeH,
              double                                       t,
              double                                       dt,
              int                                          normal
            ) -> double {
              return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
            },
            marker.x(),
            marker.h(),
            t,
            dt,
            {{NUMBER_OF_VOLUMES_PER_AXIS}}-2,
            {{NUMBER_OF_UNKNOWNS}},
            {{NUMBER_OF_AUXILIARY_VARIABLES}},
            targetPatch
          );

          repositories::{{SOLVER_INSTANCE}}.setMaximumEigenvalue( maxEigenvalue );
        }
  )
{
  logDebug( "{{CLASSNAME}}(...)", "spawn optimistic task for " << marker.toString() << " with t=" << (t+dt) << ", dt=" << predictedTimeStepSize );
}



void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::mergeTaskOutcomeIntoPatch(
  int                    taskNumber,
  double* __restrict__   patch
) {
  std::pair<int, double*> optimisticTaskOutcome = ::exahype2::EnclaveBookkeeping::getInstance().waitForTaskToTerminateAndReturnResult(taskNumber);

  #if Dimensions==2
  assertionEquals( optimisticTaskOutcome.first, {{NUMBER_OF_INNER_DOUBLE_VALUES_IN_PATCH_2D}} );
  #else
  assertionEquals( optimisticTaskOutcome.first, {{NUMBER_OF_INNER_DOUBLE_VALUES_IN_PATCH_3D}} );
  #endif

  ::exahype2::fv::insertPatch(
    optimisticTaskOutcome.second,
    patch,
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}},
    {{NUMBER_OF_VOLUMES_PER_AXIS}},
    1
  );

  tarch::freeMemory( optimisticTaskOutcome.second, tarch::MemoryLocation::Heap );
}



void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::applyKernelToCellBoundary(
  const ::peano4::datamanagement::CellMarker& marker,
  double                                      t,
  double                                      dt,
  double* __restrict__                        reconstructedPatch,
  double* __restrict__                        patchData
) {
  logDebug( "applyKernelToCellBoundary(...)", "update boundary of cell " << marker.toString() << " with t=" << t << ", dt=" << dt );

  {% if PREPROCESS_RECONSTRUCTED_PATCH!="" %}
  assertionMsg(false, "optimistic time stepping does not support pre- and postprocessing" );
  {% endif %}

  auto maskOutInnerVolumes = [&](const tarch::la::Vector<Dimensions, int>& i) -> bool {
    return tarch::la::oneEquals( i, 0 ) or tarch::la::oneEquals( i, {{NUMBER_OF_VOLUMES_PER_AXIS}}-1 );
  };

  ::exahype2::fv::copyPatch(
    reconstructedPatch,
    patchData,
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}},
    {{NUMBER_OF_VOLUMES_PER_AXIS}},
    1,
    maskOutInnerVolumes
  );

  ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS(
    [&](
      const double* __restrict__                   QL,
      const double* __restrict__                   QR,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       dx,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double                                       FL[],
      double                                       FR[]
    ) -> void {
    ::exahype2::fv::splitRusanov1d(
      [] (
       const double * __restrict__ Q,
       const tarch::la::Vector<Dimensions,double>&  faceCentre,
       const tarch::la::Vector<Dimensions,double>&  volumeH,
       double                                       t,
       double                                       dt,
       int                                          normal,
       double * __restrict__                        F
      ) -> void {
        {% if FLUX_IMPLEMENTATION=="<none>" %}
        for (int i=0; i<{{NUMBER_OF_UNKNOWNS}}; i++) F[i] = 0.0;
        {% else %}
        repositories::{{SOLVER_INSTANCE}}.flux( Q, faceCentre, volumeH, t, normal, F );
        {% endif %}
      },
      [] (
        const double* __restrict__                   Q,
        const double * __restrict__                  deltaQ,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double                                       BgradQ[]
      ) -> void {
        {% if NCP_IMPLEMENTATION!="<none>" %}
        repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct( Q, deltaQ, faceCentre, volumeH, t, normal, BgradQ );
        {% endif %}
      },
      [] (
        const double* __restrict__                   Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal
      ) -> double {
        return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
      },
      QL, QR, x, dx, t, dt, normal,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      FL,FR,
      {% if FLUX_IMPLEMENTATION=="<none>" %}
      true,
      {% else %}
      false,
      {% endif %}
      {% if NCP_IMPLEMENTATION=="<none>" %}
      true
      {% else %}
      false
      {% endif %}
    );
    },
  [&](
    const double * __restrict__                  Q,
    const tarch::la::Vector<Dimensions,double>&  x,
    double                                       dx,
    double                                       t,
    double                                       dt,
    double * __restrict__                        S
  ) -> void {
    repositories::{{SOLVER_INSTANCE}}.sourceTerm(
      Q,
      x, dx, t, dt,
      S
    );
  },
    marker.x(),
    marker.h(),
    t,
    dt,
    {{NUMBER_OF_VOLUMES_PER_AXIS}},
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}},
    reconstructedPatch,
    patchData,
    maskOutInnerVolumes
  );

  double maxEigenvalue = ::exahype2::fv::maxEigenvalue_AoS(
    [] (
      const double* __restrict__                   Q,
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal
    ) -> double {
      return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
    },
    marker.x(),
    marker.h(),
    t,
    dt,
    {{NUMBER_OF_VOLUMES_PER_AXIS}},
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}},
    patchData,
    maskOutInnerVolumes
  );

  repositories::{{SOLVER_INSTANCE}}.setMaximumEigenvalue( maxEigenvalue );

  {% if POSTPROCESS_UPDATED_PATCH!="" %}
  assertionMsg(false, "optimistic time stepping does not support pre- and postprocessing" );
  {% endif %}
}
