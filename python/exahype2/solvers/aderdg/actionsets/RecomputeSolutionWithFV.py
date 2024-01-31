# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractAderDGActionSet import AbstractAderDGActionSet

import peano4.solversteps
import jinja2
class RecomputeSolutionWithFV(AbstractAderDGActionSet):
  TemplateRecomputeSolutionWithFV = """


  if ( {{SEND_PREDICATE}} ) {

      celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker ownStatus = fineGridCell{{SOLVER_NAME}}CellLabel.getTroubled_Marker();

      bool sendFaceProjection = ownStatus>=celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker::FV_TO_DG;
      for(int d=0; d<2*Dimensions; d++){
        sendFaceProjection |= fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getTroubled_Marker()>= facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::DG_TO_FV;
      }

      //If there's no need to send anything, skip this step
      if(!sendFaceProjection) return;

      double* luh = fineGridCell{{SOLVER_NAME}}Q_old.value;
      double* newQWithoutHalo =  fineGridCell{{SOLVER_NAME}}Q_fv.value;

      //converts DG into fv
      generated::kernels::limiter::projectOnFVLimiterSpaceWithoutHalo(luh, newQWithoutHalo);

      // @todo Might want to use routine from Projection.h in toolbox.
      for(int d=0; d<Dimensions; d++) {
        /**
         * d-loop over all dimensions except d. The vector k's entry d is set
         * to 0. We start with the left/bottom face, i.e. the one closer to the 
         * coordinate system's origin.
         */
        dfore(k,{{2*ORDER+1}},d,0) {
          for (int i=0; i<1; i++) {
            tarch::la::Vector<Dimensions,int> patchCell   = k;
            tarch::la::Vector<Dimensions,int> overlapCell = k;
            patchCell(d)   = i;
            overlapCell(d) = i+1;
            
            int patchCellSerialised   = peano4::utils::dLinearised(patchCell,{{2*ORDER+1}});
            int overlapCellSerialised = toolbox::blockstructured::serialiseVoxelIndexInOverlap(overlapCell,{{2*ORDER+1}},1,d);
            for (int j=0; j<{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}; j++) {
              fineGridFaces{{SOLVER_NAME}}Q_FV_Projections(d).value[overlapCellSerialised*{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}+j] = 
                newQWithoutHalo[patchCellSerialised*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})+j];
            }
    
            patchCell(d)   = i+{{2*ORDER+1}}-1;
            overlapCell(d) = i;
            
            patchCellSerialised   = peano4::utils::dLinearised(patchCell,{{2*ORDER+1}});
            overlapCellSerialised = toolbox::blockstructured::serialiseVoxelIndexInOverlap(overlapCell,{{2*ORDER+1}},1,d);
            for (int j=0; j<{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}; j++) {
              fineGridFaces{{SOLVER_NAME}}Q_FV_Projections(d+Dimensions).value[overlapCellSerialised*{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}+j] = 
                newQWithoutHalo[patchCellSerialised*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})+j];
            }
          }
        }
      }

  }

  auto serialisePatchIndex = [](tarch::la::Vector<Dimensions,int> overlapCell, int normal) {
    int base   = 1;
    int result = 0;
    for (int d=0; d<Dimensions; d++) {
      result += overlapCell(d) * base;
      if (d==normal) {
        base *= 1*2;
      }
      else {
        base *= {{2*ORDER+1}};
      }
    }
    return result;
  };

  if ( {{RECOMPUTE_PREDICATE}} ) {

    //Should now have received projections, will use them to perform one step of FV

    // needs to declare and define timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}

    double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

    //reset timestamp to previous one if it's ben updated in the corrector step already
    if(fineGridCell{{SOLVER_NAME}}CellLabel.getTroubled_Marker()<celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker::TROUBLED){
      timeStamp -= timeStepSize;
    }

    const int NumberOfVariables  = {{NUMBER_OF_UNKNOWNS}};
    const int NumberOfParameters = {{NUMBER_OF_AUXILIARY_VARIABLES}};

    #if Dimensions==2
    constexpr int fvBasisSizeWithHalo = (2*{{ORDER}}+3)*(2*{{ORDER}}+3);
    #else
    constexpr int fvBasisSizeWithHalo = (2*{{ORDER}}+3)*(2*{{ORDER}}+3)*(2*{{ORDER}}+3);
    #endif

    double* luh = fineGridCell{{SOLVER_NAME}}Q.value;
    double* newQWithoutHalo =  fineGridCell{{SOLVER_NAME}}Q_fv.value;
    double oldQWithHalo[fvBasisSizeWithHalo*(NumberOfVariables+NumberOfParameters)];

    generated::kernels::limiter::projectOnFVSpaceWithHaloFromFVWithoutHalo(fineGridCell{{SOLVER_NAME}}Q_fv.value, oldQWithHalo);

    //
    // Bring in the auxiliary patches, i.e. befill halo
    //
    for(int d=0; d<Dimensions; d++) {
      // d-loop over all dimensions except d. The vector k's entry d is set
      // to 0. We start with the left/bottom face, i.e. the one closer to the 
      // coordinate system's origin.
      //
      dfore(k,{{2*ORDER+1}},d,0) {
        for (int i=0; i<1; i++) {
          tarch::la::Vector<Dimensions,int> destinationCell = k + tarch::la::Vector<Dimensions,int>(1);
          tarch::la::Vector<Dimensions,int> sourceCell      = k;
          destinationCell(d) = i;
          sourceCell(d)      = i;
          
          int destinationCellSerialised   = peano4::utils::dLinearised(destinationCell,{{2*ORDER+1}} + 2*1);
          int sourceCellSerialised        = serialisePatchIndex(sourceCell,d);

          for (int j=0; j<{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}; j++) {
            oldQWithHalo[ destinationCellSerialised*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})+j ] = fineGridFaces{{SOLVER_NAME}}Q_FV_Projections(d).value[ sourceCellSerialised*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})+j ];
          }

          destinationCell(d) = i+{{2*ORDER+1}}+1;
          sourceCell(d)      = i+1;

          destinationCellSerialised   = peano4::utils::dLinearised(destinationCell,{{2*ORDER+1}} + 2*1);
          sourceCellSerialised        = serialisePatchIndex(sourceCell,d);
          for (int j=0; j<{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}; j++) {
            oldQWithHalo[ destinationCellSerialised*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})+j ] = fineGridFaces{{SOLVER_NAME}}Q_FV_Projections(d+Dimensions).value[ sourceCellSerialised*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})+j ];
          }
        }
      }
      logTraceOut( "touchCellFirstTime(...)::loopOverFace" );
    }

  ::exahype2::CellData  patchData( oldQWithHalo, marker.x(), marker.h(), timeStamp, timeStepSize, newQWithoutHalo );

  ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapFunctors<
  {{2*ORDER+1}},
  1, //Halo_size
  {{NUMBER_OF_UNKNOWNS}},
  {{NUMBER_OF_AUXILIARY_VARIABLES}},
  {% if USE_FLUX=="<none>" %} false {% else %} true {% endif %}, //use_flux
  {% if USE_NCP=="<none>" %} false {% else %} true {% endif %}, //use_ncp
  {% if USE_SOURCE=="<none>" %} false {% else %} true {% endif %}, //use_source
  false, //compute_max_eigenvalue
  ::exahype2::enumerator::AoSLexicographicEnumerator>(
    patchData,
  [&](
    [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
    [[maybe_unused]] double                                       t,
    [[maybe_unused]] double                                       dt,
    [[maybe_unused]] int                                          normal,
    [[maybe_unused]] double* __restrict__                         F // F[{{NUMBER_OF_UNKNOWNS}}]
  )->void {
    {% if USE_FLUX!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.flux( Q, faceCentre, volumeH, t, dt, normal, F );
    {% endif %}
  },
  [&](
    [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
    [[maybe_unused]] double                                       t,
    [[maybe_unused]] double                                       dt,
    [[maybe_unused]] int                                          normal,
    [[maybe_unused]] double* __restrict__                         BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
  )->void {
    {% if USE_NCP!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct( Q, deltaQ, faceCentre, volumeH, t, dt, normal, BTimesDeltaQ );
    {% endif %}  },
  [&](
    [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeX,
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
    [[maybe_unused]] double                                       t,
    [[maybe_unused]] double                                       dt,
    [[maybe_unused]] double* __restrict__                         S // S[{{NUMBER_OF_UNKNOWNS}}]
  )->void {
    {% if USE_SOURCE!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.algebraicSource(volumeX, t, Q, S);
    {% endif %}
  },
  [&](
    [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
    [[maybe_unused]] double                                       t,
    [[maybe_unused]] double                                       dt,
    [[maybe_unused]] int                                          normal
  )->double {
    return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, dt, normal);
  }
);


    generated::kernels::limiter::projectOnDGSpaceFromFVWithoutHalo(newQWithoutHalo, luh);

    {{COMPUTE_NEW_TIME_STEP_SIZE}}
  
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp+timeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(newTimeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
      
    repositories::{{SOLVER_INSTANCE}}.update(newTimeStepSize, timeStamp+timeStepSize, marker.h()(0) );

  }


"""
  
  def __init__(self,solver,send_guard,recompute_guard):
    super(RecomputeSolutionWithFV,self).__init__(solver)
    self.send_guard               = send_guard
    self.recompute_guard          = recompute_guard
  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "SEND_PREDICATE" ]           = self.send_guard
      d[ "RECOMPUTE_PREDICATE"]       = self.recompute_guard
      result = jinja2.Template(self.TemplateRecomputeSolutionWithFV).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")

  def get_includes(self):
      return (
          super(RecomputeSolutionWithFV, self).get_includes()
          + """
#include "../generated/kernels/limiter/fv/Kernels.h"
#include "toolbox/blockstructured/Enumeration.h"
#include "exahype2/fv/rusanov/Rusanov.h"
    """
      )