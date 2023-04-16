# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .ADERDG   import ADERDG
from .ADERDG   import AbstractADERDGActionSet
from .PDETerms import PDETerms

from peano4.solversteps.ActionSet import ActionSet

import peano4
import exahype2

import jinja2


class UpdateCell(AbstractADERDGActionSet):
  TemplateUpdateCell = jinja2.Template( """
    if ( marker.isRefined() ) return; // TODO(dominic): (probably) only applicable for unigrid simulations

    switch (repositories::{{SOLVER_INSTANCE}}.getSolverState() ) {
      case {{SOLVER_NAME}}::SolverState::GridConstruction:
        assertionMsg( false, "should not be entered" );
        break;
      case {{SOLVER_NAME}}::SolverState::GridInitialisation:
        assertionMsg( false, "should not be entered" );
        break;
      case {{SOLVER_NAME}}::SolverState::Prediction:
        {
          #if Dimensions==2
          double spaceTimeQ[ ({{ORDER}}+1)*({{ORDER}}+1)*({{ORDER}}+1)*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})  ];
          #else
          double spaceTimeQ[ ({{ORDER}}+1)*({{ORDER}}+1)*({{ORDER}}+1)*({{ORDER}}+1)*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})  ];
          #endif
          ::exahype2::aderdg::spaceTimePredictor_PicardLoop_loop_AoS(
            [&](
              const double * __restrict__                 Q,
              const tarch::la::Vector<Dimensions,double>& x,
              double                                      t,
              int                                         normal,
              double * __restrict__                       F
            )->void {
              {% if FLUX_IMPLEMENTATION!="<none>" %}
              repositories::{{SOLVER_INSTANCE}}.flux(Q,x,t,normal,F);
              {% endif %}
            },
            [&](
              const double * __restrict__                 Q,
              const tarch::la::Vector<Dimensions,double>& x,
              double                                      t,
              double * __restrict__                       S
            )->void {
              {% if SOURCES_IMPLEMENTATION!="<none>" %}
              repositories::{{SOLVER_INSTANCE}}.algebraicSource(Q,x,t,S);
              {% endif %}
            },
            [&](
              const double * __restrict__                 Q,
              const double * __restrict__                 deltaQ,
              const tarch::la::Vector<Dimensions,double>& x,
              double                                      t,
              int                                         normal,
              double * __restrict__                       BgradQ
            )->void {
              {% if NCP_IMPLEMENTATION!="<none>" %}
              repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct(Q,deltaQ,x,t,normal,BgradQ);
              {% endif %}
            },
            spaceTimeQ,                           // QOut
            fineGridCell{{SOLVER_NAME}}Q.value,   // QIn
            repositories::{{SOLVER_INSTANCE}}.QuadraturePoints,
            repositories::{{SOLVER_INSTANCE}}.QuadratureWeights,
            repositories::{{SOLVER_INSTANCE}}.StiffnessOperator,            // Kxi,
            repositories::{{SOLVER_INSTANCE}}.InvertedPredictorLhsOperator, // iK1,
            repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft,      // FLCoeff,
            repositories::{{SOLVER_INSTANCE}}.DerivativeOperator,   // dudx, 
            marker.x(),
            marker.h()(0), // we assume cubic/square cells
            repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp(), 
            repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(), 
            {{ORDER}}, 
            {{NUMBER_OF_UNKNOWNS}}, 
            {{NUMBER_OF_AUXILIARY_VARIABLES}},
            1e-8,  // atol,
            {{ "true" if FLUX_IMPLEMENTATION!="<none>" else "false" }},
            {{ "true" if SOURCES_IMPLEMENTATION!="<none>" else "false" }},
            {{ "true" if NCP_IMPLEMENTATION!="<none>" else "false" }}
          );

          ::exahype2::aderdg::corrector_addCellContributions_loop_AoS(
            [&](
              const double * __restrict__                 Q,
              const tarch::la::Vector<Dimensions,double>& x,
              double                                      t,
              int                                         normal,
              double * __restrict__                       F
            )->void {
              {% if FLUX_IMPLEMENTATION!="<none>" %}
              repositories::{{SOLVER_INSTANCE}}.flux(Q,x,t,normal,F);
              {% endif %}
            },
            [&](
              const double * __restrict__                 Q,
              const tarch::la::Vector<Dimensions,double>& x,
              double                                      t,
              double * __restrict__                       S
            )->void {
              {% if SOURCES_IMPLEMENTATION!="<none>" %}
              repositories::{{SOLVER_INSTANCE}}.algebraicSource(Q,x,t,S);
              {% endif %}
            },
            [&](
              const double * __restrict__                 Q,
              const double * __restrict__                 dQ_or_dQdn,
              const tarch::la::Vector<Dimensions,double>& x,
              double                                      t,
              int                                         normal,
              double * __restrict__                       BgradQ
            )->void {
              {% if NCP_IMPLEMENTATION!="<none>" %}
              repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct(Q,dQ_or_dQdn,x,t,normal,BgradQ);
              {% endif %}
            },
            fineGridCell{{SOLVER_NAME}}Q.value,                           //  UOut,
            spaceTimeQ,                                                   //  QIn,
            repositories::{{SOLVER_INSTANCE}}.QuadraturePoints,           //  nodes,
            repositories::{{SOLVER_INSTANCE}}.QuadratureWeights,          //  weights,
            repositories::{{SOLVER_INSTANCE}}.StiffnessOperator,          //  Kxi,
            repositories::{{SOLVER_INSTANCE}}.DerivativeOperator,         //  dudx,
            marker.x(),                                                   //  cellCentre,
            marker.h()(0),                                                //  dx,
            repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp(),          //  t,
            repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),       //  dt,
            {{ORDER}},                                                    //  order,
            {{NUMBER_OF_UNKNOWNS}},                                       //  unknowns,
            {{NUMBER_OF_AUXILIARY_VARIABLES}},                            //  auxiliaryVariables,
            {{ "true" if FLUX_IMPLEMENTATION!="<none>" else "false" }},   //  callFlux,
            {{ "true" if SOURCES_IMPLEMENTATION!="<none>" else "false" }},//  callSource,
            {{ "true" if NCP_IMPLEMENTATION!="<none>" else "false" }}     //  callNonconservativeProduct
          );

          // collect information from the adjacent faces
          double* QHull[Dimensions*2];
          bool    atBoundary[Dimensions*2];
          bool    adjacentToBoundary = false;
          for (int face = 0; face < Dimensions*2; face++) {
             QHull[ face ]       = fineGridFaces{{SOLVER_NAME}}QSolutionExtrapolation(face).value;
             atBoundary [ face ] = fineGridFacesLabel(face).getBoundary() and
                                   not repositories::{{SOLVER_INSTANCE}}.PeriodicBC[face%Dimensions];
             adjacentToBoundary |= atBoundary[ face ];
          }
          
          // hull extrapolation
          ::exahype2::aderdg::spaceTimePredictor_extrapolate_loop_AoS(
            QHull,
            spaceTimeQ,                           // QIn
            repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft,
            repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesRight,
            {{ORDER}}, 
            {{NUMBER_OF_UNKNOWNS}}, 
            {{NUMBER_OF_AUXILIARY_VARIABLES}}
          );
          
          // set Dirichlet boundary conditions
          if ( adjacentToBoundary ) {
            ::exahype2::aderdg::riemann_setBoundaryState_loop_AoS(
              [&](
                 const double * __restrict__                 QInside,
                 double * __restrict__                       QOutside,
                 const tarch::la::Vector<Dimensions,double>& x,
                 double                                      t,
                 int                                         normal
               )-> void {
                 repositories::{{SOLVER_INSTANCE}}.boundaryConditions(QInside,QOutside,x,t,normal);
               },
               QHull,
               repositories::{{SOLVER_INSTANCE}}.QuadraturePoints,
               marker.x(),                                                   //  cellCentre,
               marker.h()(0),                                                //  dx,
               repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp(), 
               repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(), 
               {{ORDER}}, 
               {{NUMBER_OF_UNKNOWNS}}, 
               {{NUMBER_OF_AUXILIARY_VARIABLES}},
               atBoundary
            );
          }
        }
        break;
      case {{SOLVER_NAME}}::SolverState::Correction:
        {
          // collect information from the adjacent faces
          double* QHull[Dimensions*2];
          bool    atBoundary[Dimensions*2];
          for (int face = 0; face < Dimensions*2; face++) {
             QHull      [ face ] = fineGridFaces{{SOLVER_NAME}}QSolutionExtrapolation(face).value;
             atBoundary [ face ] = fineGridFacesLabel(face).getBoundary() and
                                   not repositories::{{SOLVER_INSTANCE}}.PeriodicBC[face%Dimensions];
          }
          
          // compute max eigenvalue per face 
          double maxEigenvaluePerFace[Dimensions*2];
          ::exahype2::aderdg::riemann_maxAbsoluteEigenvalue_loop_AoS(
            [&](
              const double * __restrict__                 Q,
              const tarch::la::Vector<Dimensions,double>& x,
              double                                      t,
              const int                                   direction
            )-> double { 
              return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue(Q,x,t,direction);
            },
            maxEigenvaluePerFace,
            QHull,
            repositories::{{SOLVER_INSTANCE}}.QuadraturePoints,
            marker.x(),                                                   //  cellCentre,
            marker.h()(0),                                                //  dx,
            repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp(), 
            repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(), 
            {{ORDER}}, 
            {{NUMBER_OF_UNKNOWNS}}, 
            {{NUMBER_OF_AUXILIARY_VARIABLES}}
          );
   
          // call Rusanov
          #if Dimensions==2
          double riemannResult[ Dimensions*2*({{ORDER}}+1)*({{NUMBER_OF_UNKNOWNS}}) ];
          #else
          double riemannResult[ Dimensions*2*({{ORDER}}+1)*({{ORDER}}+1)*({{NUMBER_OF_UNKNOWNS}}) ];
          #endif
          ::exahype2::aderdg::rusanovNonlinear_loop_AoS(
            [&](
              const double * __restrict__                 Q,
              const tarch::la::Vector<Dimensions,double>& x,
              double                                      t,
              int                                         normal,
              double * __restrict__                       F
            )->void {
              {% if FLUX_IMPLEMENTATION!="<none>" %}
              repositories::{{SOLVER_INSTANCE}}.flux(Q,x,t,normal,F);
              {% endif %}
            },
            [&](
              const double * __restrict__                 Q,
              const tarch::la::Vector<Dimensions,double>& x,
              double                                      t,
              int                                         normal,
              double * __restrict__                       F
            )->void {
              {% if FLUX_IMPLEMENTATION!="<none>" %}
              repositories::{{SOLVER_INSTANCE}}.flux(Q,x,t,normal,F);
              {% endif %}
            },
            [&](
              const double * __restrict__                 Q,
              const double * __restrict__                 dQ_or_dQdn,
              const tarch::la::Vector<Dimensions,double>& x,
              double                                      t,
              int                                         normal,
              double * __restrict__                       BgradQ
            )->void {
              {% if NCP_IMPLEMENTATION!="<none>" %}
              repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct(Q,dQ_or_dQdn,x,t,normal,BgradQ);
              {% endif %}
            },
            [&](
              const double * __restrict__                 Q,
              const double * __restrict__                 dQ_or_dQdn,
              const tarch::la::Vector<Dimensions,double>& x,
              double                                      t,
              int                                         normal,
              double * __restrict__                       BgradQ
            )->void {
              {% if NCP_IMPLEMENTATION!="<none>" %}
              repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct(Q,dQ_or_dQdn,x,t,normal,BgradQ);
              {% endif %}
            },
            riemannResult,
            QHull, 
            maxEigenvaluePerFace,
            repositories::{{SOLVER_INSTANCE}}.QuadraturePoints,
            repositories::{{SOLVER_INSTANCE}}.QuadratureWeights,
            marker.x(),                                                   //  cellCentre,
            marker.h()(0),                                                //  dx,
            repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp(), 
            repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(), 
            {{ORDER}}, 
            {{NUMBER_OF_UNKNOWNS}}, 
            {{NUMBER_OF_AUXILIARY_VARIABLES}},
            atBoundary,
            {{ "true" if FLUX_IMPLEMENTATION!="<none>" else "false" }},   //  callFlux,
            {{ "true" if NCP_IMPLEMENTATION!="<none>" else "false" }}     //  callNonconservativeProduct
          );

          // add result to solution
          const double maxEigenvalueInCellVolume = 
            ::exahype2::aderdg::corrector_addRiemannContributions_loop_AoS(
              [&](
                double * __restrict__                       Q,
                const tarch::la::Vector<Dimensions,double>& x,
                double                                      t
              )->void {
                repositories::{{SOLVER_INSTANCE}}.adjustSolution(Q,x,t);
              },
              [&](
                const double * __restrict__                 Q,
                const tarch::la::Vector<Dimensions,double>& x,
                double                                      t,
                const int                                   direction
              )-> double { 
                return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue(Q,x,t,direction);
              },
              fineGridCell{{SOLVER_NAME}}Q.value,                         //  UOut,
              riemannResult,
              repositories::{{SOLVER_INSTANCE}}.QuadraturePoints,
              repositories::{{SOLVER_INSTANCE}}.QuadratureWeights,
              repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft,  // FLCoeff,
              marker.x(),                                                 // cellCentre
              marker.h()(0),                                              // dx,
              repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp(),        // x
              repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),     // dt 
              {{ORDER}}, 
              {{NUMBER_OF_UNKNOWNS}}, 
              {{NUMBER_OF_AUXILIARY_VARIABLES}},
              {{ "true" if ADAPTIVE_TIME_STEPPING else "false" }}         //  callMaxEigenvalue
            );
{% if ADAPTIVE_TIME_STEPPING %}
            // compute time step size from 'maxEigenvalueInCellVolume' and/or 'maxEigenvaluePerFace[]'
            const double safetyFactor        = 0.9;
            const double stabilityFactorADER = safetyFactor * {{ADER_PNPM_STABILITY_FACTOR}};
            const double dtNew = safetyFactor * stabilityFactorADER * marker.h()(0) / maxEigenvalueInCellVolume; 
{% endif %}
        }
        break;
      case {{SOLVER_NAME}}::SolverState::Plotting:
        assertionMsg( false, "should not be entered" );
        break;
    }
  """)    
  
  def __init__(self,solver):
    AbstractADERDGActionSet.__init__(self,solver)
    
    
  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)      
      result = self.TemplateUpdateCell.render(**d)
      pass 
    return result


  def get_includes(self):
    return AbstractADERDGActionSet.get_includes(self) + """
#include "exahype2/aderdg/PredictorAoS.h"
#include "exahype2/aderdg/CorrectorAoS.h"
"""



class NonFusedGenericRusanovFixedTimeStepSize( ADERDG ):
  """
  
    Probably the simplest ADER-DG solver implementation you could think off. 
  
  """


  def __init__(self, 
    name, order, unknowns, auxiliary_variables, polynomials, min_h, max_h, time_step_size, 
    flux=PDETerms.User_Defined_Implementation, ncp=None, sources=None,
    plot_grid_properties=False
  ):
    """

      Instantiate ADER-DG in a non-fused variant

    """
    self._time_step_size = time_step_size

    self._flux_implementation                 = PDETerms.None_Implementation
    self._ncp_implementation                  = PDETerms.None_Implementation
    self._eigenvalues_implementation          = PDETerms.User_Defined_Implementation
    self._boundary_conditions_implementation  = PDETerms.User_Defined_Implementation
    self._refinement_criterion_implementation = PDETerms.Empty_Implementation
    self._initial_conditions_implementation   = PDETerms.User_Defined_Implementation
    self._sources_implementation              = PDETerms.None_Implementation

    ADERDG.__init__(self, name, order, unknowns, auxiliary_variables, polynomials, min_h, max_h, plot_grid_properties)

    self.set_implementation(flux=flux,ncp=ncp,sources=sources)
    
    # @todo Sollte false sein
    condition="repositories::{SOLVER_INSTANCE}.getSolverState()=={SOLVER_NAME}::SolverState::{{STEP}}".format(
      SOLVER_INSTANCE=self.get_name_of_global_instance(),
      SOLVER_NAME=self._name)
    self._face_spacetime_solution.generator.send_condition              = condition.format(STEP="Prediction")
    self._face_spacetime_solution.generator.receive_and_merge_condition = condition.format(STEP="Correction")


  def create_data_structures(self):
    ADERDG.create_data_structures(self)


  def create_action_sets(self):
    ADERDG.create_action_sets(self)
    self._action_set_update_cell = UpdateCell(self)

  
  def set_implementation(self,flux=None,ncp=None,sources=None,eigenvalues=None,boundary_conditions=None,refinement_criterion=None,initial_conditions=None):
    """
      If you pass in User_Defined, then the generator will create C++ stubs 
      that you have to befill manually. If you pass in None_Implementation, it 
      will create nop, i.e. no implementation or defaults. Any other string
      is copied 1:1 into the implementation. If you pass in None, then the 
      set value so far won't be overwritten.
      
      Please note that not all options are supported by all solvers. You 
      cannot set ncp and fluxes for the ClawPack Riemann solvers, e.g.
    """
    if flux!=None:
      self._flux_implementation        = flux
    if ncp!=None:
      self._ncp_implementation         = ncp
    if eigenvalues!=None:    
      self._eigenvalues_implementation = eigenvalues
    if sources!=None:    
      self._sources_implementation     = sources  
    if boundary_conditions!=None:
      self._boundary_conditions_implementation        = boundary_conditions
    if refinement_criterion!=None:
      self._refinement_criterion_implementation       = refinement_criterion
    if initial_conditions!=None: 
      self._initial_conditions_implementation         = initial_conditions
    
    self.create_action_sets()

  
  def set_preprocess_reconstructed_patch_kernel(self,kernel):
    self._preprocess_reconstructed_patch = kernel
    self._action_set_update_cell = UpdateCell(self)


  def set_postprocess_updated_patch_kernel(self,kernel):
    self._postprocess_updated_patch = kernel
    self._action_set_update_cell = UpdateCell(self)
     
      
  def add_entries_to_text_replacement_dictionary(self,d):
    """
     
     d: Dictionary of string to string
        in/out argument
    
    """
    d[ "TIME_STEP_SIZE" ]                     = self._time_step_size

    d[ "FLUX_IMPLEMENTATION"]                 = self._flux_implementation
    d[ "NCP_IMPLEMENTATION"]                  = self._ncp_implementation
    d[ "SOURCES_IMPLEMENTATION"]              = self._sources_implementation
    d[ "EIGENVALUES_IMPLEMENTATION"]          = self._eigenvalues_implementation
    d[ "BOUNDARY_CONDITIONS_IMPLEMENTATION"]  = self._boundary_conditions_implementation
    d[ "REFINEMENT_CRITERION_IMPLEMENTATION"] = self._refinement_criterion_implementation
    d[ "INITIAL_CONDITIONS_IMPLEMENTATION"]   = self._initial_conditions_implementation
    
    pass


  def get_user_includes(self):
    return """
#include "exahype2/aderdg/Generic.h" 

#include <algorithm>
"""    


  def add_to_Peano4_datamodel( self, datamodel, verbose ):
    ADERDG.add_to_Peano4_datamodel( self, datamodel, verbose )

  def add_use_data_statements_to_Peano4_solver_step(self, step):
    ADERDG.add_use_data_statements_to_Peano4_solver_step( self, step )

  def add_actions_to_perform_time_step(self, step):
    ADERDG.add_actions_to_perform_time_step( self, step )
