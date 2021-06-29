#include "{{CLASSNAME}}.h"

// user includes
{{INCLUDES}}

#include "exahype2/fv/Generic.h"
#include "exahype2/fv/Rusanov.h"
#include "exahype2/EnclaveBookkeeping.h"
#include "exahype2/EnclaveTask.h"

#include "peano4/utils/Loop.h"

#include "tarch/multicore/SmartScheduler.h"


tarch::logging::Log                {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );
int                                {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_enclaveTaskId(
  tarch::multicore::registerSmartMPITask<{{NAMESPACE | join("::")}}::{{CLASSNAME}}>(
    peano4::parallel::Tasks::getTaskType("{{NAMESPACE | join("::")}}::{{CLASSNAME}}")
  )
);


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::applyKernelToCell(
  const ::peano4::datamanagement::CellMarker& marker,
  double                                      t,
  double                                      dt,
  double* __restrict__                        reconstructedPatch,
  double* __restrict__                        targetPatch
) {
  {{PREPROCESS_RECONSTRUCTED_PATCH}}

  ::exahype2::fv::copyPatch(
    reconstructedPatch,
    targetPatch,
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}},
    {{NUMBER_OF_VOLUMES_PER_AXIS}},
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
    {{NUMBER_OF_VOLUMES_PER_AXIS}},
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}},
    reconstructedPatch,
    targetPatch
  );

  {{FREE_SKELETON_MEMORY}}
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
    {{NUMBER_OF_VOLUMES_PER_AXIS}},
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}},
    targetPatch
  );

  repositories::{{SOLVER_INSTANCE}}.setMaximumEigenvalue( maxEigenvalue );
}





{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(
  const ::peano4::datamanagement::CellMarker& marker,
  double                                      t,
  double                                      dt,
  double* __restrict__                        reconstructedPatch
):
  ::exahype2::EnclaveTask(
    _enclaveTaskId,
    marker,
    t,
    dt,
    reconstructedPatch,
    #if Dimensions==2
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}},
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}},
    #else
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}},
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}},
    #endif
    [&](double* reconstructedPatch, double* targetPatch, const ::peano4::datamanagement::CellMarker& marker, double t, double dt) -> void {
          {{PREPROCESS_RECONSTRUCTED_PATCH}}

          ::exahype2::fv::copyPatch(
            reconstructedPatch,
            targetPatch,
            {{NUMBER_OF_UNKNOWNS}},
            {{NUMBER_OF_AUXILIARY_VARIABLES}},
            {{NUMBER_OF_VOLUMES_PER_AXIS}},
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
            {{NUMBER_OF_VOLUMES_PER_AXIS}},
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
            {{NUMBER_OF_VOLUMES_PER_AXIS}},
            {{NUMBER_OF_UNKNOWNS}},
            {{NUMBER_OF_AUXILIARY_VARIABLES}},
            targetPatch
          );

          repositories::{{SOLVER_INSTANCE}}.setMaximumEigenvalue( maxEigenvalue );
        }
  )
  #ifdef UseSmartMPI
  , smartmpi::Task(_enclaveTaskId)
  #endif
{}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isSmartMPITask() const {
  #ifdef UseSmartMPI
  return true;
  #else
  return false;
  #endif
}


#ifdef UseSmartMPI
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::runLocally() {
  run();
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::sendTaskInputToRank(int rank, int tag, MPI_Comm communicator) {
  assertionMsg("have to implement this", false);
//  int numberOfInputValues =
/*
  _numberOfResultValues(numberOfResultValues),
  MPI_Send( _inputValues )


  int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm)
  _inputValues,_outputValues,_marker,_t,_dt
*/
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveTaskInputFromRank(int rank, int tag, MPI_Comm communicator) {
  assertionMsg("have to implement this", false);

}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::runLocallyAndSendTaskOutputToRank(int rank, int tag, MPI_Comm communicator) {
  _outputValues = tarch::allocateMemory( _numberOfResultValues, tarch::MemoryLocation::Heap );

  _functor(_inputValues,_outputValues,_marker,_t,_dt);
  tarch::freeMemory(_inputValues,tarch::MemoryLocation::Heap );

  assertionMsg("have to implement this", false);

}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveTaskOutputFromRank(int rank, int tag, MPI_Comm communicator) {
  assertionMsg("some lines here are not implemented", false);

  ::exahype2::EnclaveBookkeeping::getInstance().finishedTask(getTaskId(),_numberOfResultValues,_outputValues);

}
#endif
