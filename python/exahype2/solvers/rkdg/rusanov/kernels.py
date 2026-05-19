# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import exahype2
import exahype2.kerneldsl as DSL
import os

from exahype2.solvers.rkdg.actionsets.ProjectLinearCombinationOfEstimatesOntoFaces import FaceProjections
from exahype2.solvers.rkdg.actionsets.ProjectLinearCombinationOfEstimatesOntoFaces import compute_number_of_face_projection_quantities

def create_Riemann_solver_kernel_definitions():
  if not os.path.exists("kernels"):
      os.makedirs("kernels")
  
  file = open("kernels/dg.h", "a")
  file.write("""#if Dimensions == 2
#include "dg_riemann2d.h"
#elif Dimensions == 3
#include "dg_riemann3d.h"
#endif
""")
  
  create_Riemann_solver_kernel_definitions_2d()
  create_Riemann_solver_kernel_definitions_3d() 

def create_Riemann_solver_kernel_definitions_2d():
  template_parameters = [DSL.SyntaxTree.Argument("DGOrder", DSL.SyntaxTree.TInteger()),
                        DSL.SyntaxTree.Argument("NumberOfUnknowns", DSL.SyntaxTree.TInteger()),
                        DSL.SyntaxTree.Argument("NumberOfAuxiliaryVariables", DSL.SyntaxTree.TInteger()),
                        DSL.SyntaxTree.Argument("EvaluateFlux", DSL.SyntaxTree.TBoolean()),
                        DSL.SyntaxTree.Argument("EvaluateNonconservativeProduct", DSL.SyntaxTree.TBoolean())]
  
  functor_arguments = [DSL.SyntaxTree.Argument("flux", DSL.SyntaxTree.TCustom("const Flux&")),
                       DSL.SyntaxTree.Argument("nonconservativeProduct", DSL.SyntaxTree.TCustom("const NonConservativeProduct&"))]
  
  dg_riemann_x_stateless_tree = DSL.Parser().parse(exahype2.solvers.rkdg.rusanov.solveRiemannProblemX2D, template_parameters, functor_arguments, ["exahype2", "dg", "rusanov"], stateless=True, use_accelerator=False)
  dg_riemann_x_stateless_kernel = dg_riemann_x_stateless_tree.print_cpp()
  dg_riemann_x_stateless_call_with_measurement = dg_riemann_x_stateless_tree.print_definition_with_timer()
  dg_riemann_x_stateless_kernel_declaration = dg_riemann_x_stateless_tree.print_declaration()
  dg_riemann_x_stateless_call_with_measurement_declaration = dg_riemann_x_stateless_tree.print_declaration_with_timer()

  dg_riemann_y_stateless_tree = DSL.Parser().parse(exahype2.solvers.rkdg.rusanov.solveRiemannProblemY2D, template_parameters, functor_arguments, ["exahype2", "dg", "rusanov"], stateless=True, use_accelerator=False)
  dg_riemann_y_stateless_kernel = dg_riemann_y_stateless_tree.print_cpp()
  dg_riemann_y_stateless_call_with_measurement = dg_riemann_y_stateless_tree.print_definition_with_timer()
  dg_riemann_y_stateless_kernel_declaration = dg_riemann_y_stateless_tree.print_declaration()
  dg_riemann_y_stateless_call_with_measurement_declaration = dg_riemann_y_stateless_tree.print_declaration_with_timer()

  if not os.path.exists("kernels"):
      os.makedirs("kernels")
  
  file = open("kernels/dg_riemann2d.h", "w")
  file.write(f"""#pragma once
#include "exahype2/CellData.h"
#include "exahype2/VolumeIndex.h"
#include "exahype2/dg/DGUtils.h"
#include "exahype2/dg/Functors.h"
#include "peano4/utils/Loop.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"
#include <fstream>
             
{dg_riemann_x_stateless_kernel_declaration}
{dg_riemann_x_stateless_call_with_measurement_declaration}

{dg_riemann_y_stateless_kernel_declaration}
{dg_riemann_y_stateless_call_with_measurement_declaration}

namespace exahype2::dg::rusanov {{
template <class SolverType,int DGOrder,int NumberOfUnknowns,int NumberOfAuxiliaryVariables,bool EvaluateFlux,bool EvaluateNonconservativeProduct>
    void solveRiemannProblemStateless(const tarch::la::Vector<Dimensions,double>& faceCentre, const tarch::la::Vector<Dimensions,double>& cellSize, double t, double dt, int faceNumber, const double* __restrict__ faceData, double* __restrict__ solution);
}}

#include "dg_riemann2d.cpph"
""")
  file.close()

  file = open("kernels/dg_riemann2d.cpph", "w")
  file.write(dg_riemann_x_stateless_kernel)
  file.write(dg_riemann_x_stateless_call_with_measurement)
  file.write(dg_riemann_y_stateless_kernel)
  file.write(dg_riemann_y_stateless_call_with_measurement)
  file.write("""
namespace exahype2::dg::rusanov {
    template <class SolverType,int DGOrder,int NumberOfUnknowns,int NumberOfAuxiliaryVariables,bool EvaluateFlux,bool EvaluateNonconservativeProduct>
    void solveRiemannProblemStateless(const tarch::la::Vector<Dimensions,double>& faceCentre, const tarch::la::Vector<Dimensions,double>& cellSize, double t, double dt, int faceNumber,const double* __restrict__ faceData, double* __restrict__ solution){
        switch(faceNumber % Dimensions) {
        case 0:
          solveRiemannProblemXStateless<
            SolverType,
            DGOrder,
            NumberOfUnknowns,
            NumberOfAuxiliaryVariables,
            EvaluateFlux,
            EvaluateNonconservativeProduct
          >(
            faceCentre,
            cellSize,
            t,
            dt,
            faceData,
            solution
          );
          break;
        case 1:
          solveRiemannProblemYStateless<
            SolverType,
            DGOrder,
            NumberOfUnknowns,
            NumberOfAuxiliaryVariables,
            EvaluateFlux,
            EvaluateNonconservativeProduct
          >(
            faceCentre,
            cellSize,
            t,
            dt,
            faceData,
            solution
          );
          break;
        }
      }
}\n""")

  file.close()

def create_Riemann_solver_kernel_definitions_3d():
  template_parameters = [DSL.SyntaxTree.Argument("DGOrder", DSL.SyntaxTree.TInteger()),
                        DSL.SyntaxTree.Argument("NumberOfUnknowns", DSL.SyntaxTree.TInteger()),
                        DSL.SyntaxTree.Argument("NumberOfAuxiliaryVariables", DSL.SyntaxTree.TInteger()),
                        DSL.SyntaxTree.Argument("EvaluateFlux", DSL.SyntaxTree.TBoolean()),
                        DSL.SyntaxTree.Argument("EvaluateNonconservativeProduct", DSL.SyntaxTree.TBoolean())]
  
  functor_arguments = [DSL.SyntaxTree.Argument("flux", DSL.SyntaxTree.TCustom("const Flux&")),
                       DSL.SyntaxTree.Argument("nonconservativeProduct", DSL.SyntaxTree.TCustom("const NonConservativeProduct&"))]
  
  dg_riemann_x_stateless_tree = DSL.Parser().parse(exahype2.solvers.rkdg.rusanov.solveRiemannProblemX3D, template_parameters, functor_arguments, ["exahype2", "dg", "rusanov"], stateless=True, use_accelerator=False)
  dg_riemann_x_stateless_kernel = dg_riemann_x_stateless_tree.print_cpp()
  dg_riemann_x_stateless_call_with_measurement = dg_riemann_x_stateless_tree.print_definition_with_timer()
  dg_riemann_x_stateless_kernel_declaration = dg_riemann_x_stateless_tree.print_declaration()
  dg_riemann_x_stateless_call_with_measurement_declaration = dg_riemann_x_stateless_tree.print_declaration_with_timer()

  dg_riemann_y_stateless_tree = DSL.Parser().parse(exahype2.solvers.rkdg.rusanov.solveRiemannProblemY3D, template_parameters, functor_arguments, ["exahype2", "dg", "rusanov"], stateless=True, use_accelerator=False)
  dg_riemann_y_stateless_kernel = dg_riemann_y_stateless_tree.print_cpp()
  dg_riemann_y_stateless_call_with_measurement = dg_riemann_y_stateless_tree.print_definition_with_timer()
  dg_riemann_y_stateless_kernel_declaration = dg_riemann_y_stateless_tree.print_declaration()
  dg_riemann_y_stateless_call_with_measurement_declaration = dg_riemann_y_stateless_tree.print_declaration_with_timer()

  dg_riemann_z_stateless_tree = DSL.Parser().parse(exahype2.solvers.rkdg.rusanov.solveRiemannProblemZ3D, template_parameters, functor_arguments, ["exahype2", "dg", "rusanov"], stateless=True, use_accelerator=False)
  dg_riemann_z_stateless_kernel = dg_riemann_z_stateless_tree.print_cpp()
  dg_riemann_z_stateless_call_with_measurement = dg_riemann_z_stateless_tree.print_definition_with_timer()
  dg_riemann_z_stateless_kernel_declaration = dg_riemann_z_stateless_tree.print_declaration()
  dg_riemann_z_stateless_call_with_measurement_declaration = dg_riemann_z_stateless_tree.print_declaration_with_timer()

  if not os.path.exists("kernels"):
      os.makedirs("kernels")
  
  file = open("kernels/dg_riemann3d.h", "w")
  file.write(f"""#pragma once
#include "exahype2/CellData.h"
#include "exahype2/VolumeIndex.h"
#include "exahype2/dg/DGUtils.h"
#include "exahype2/dg/Functors.h"
#include "peano4/utils/Loop.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"
#include <fstream>
             
{dg_riemann_x_stateless_kernel_declaration}
{dg_riemann_x_stateless_call_with_measurement_declaration}

{dg_riemann_y_stateless_kernel_declaration}
{dg_riemann_y_stateless_call_with_measurement_declaration}

{dg_riemann_z_stateless_kernel_declaration}
{dg_riemann_z_stateless_call_with_measurement_declaration}

namespace exahype2::dg::rusanov {{
template <class SolverType,int DGOrder,int NumberOfUnknowns,int NumberOfAuxiliaryVariables,bool EvaluateFlux,bool EvaluateNonconservativeProduct>
    void solveRiemannProblemStateless(const tarch::la::Vector<Dimensions,double>& faceCentre, const tarch::la::Vector<Dimensions,double>& cellSize, double t, double dt, int faceNumber, const double* __restrict__ faceData, double* __restrict__ solution);
}}

#include "dg_riemann3d.cpph"
""")
  file.close()

  file = open("kernels/dg_riemann3d.cpph", "w")
  file.write(dg_riemann_x_stateless_kernel)
  file.write(dg_riemann_x_stateless_call_with_measurement)
  file.write(dg_riemann_y_stateless_kernel)
  file.write(dg_riemann_y_stateless_call_with_measurement)
  file.write(dg_riemann_z_stateless_kernel)
  file.write(dg_riemann_z_stateless_call_with_measurement)
  file.write("""
namespace exahype2::dg::rusanov {
    template <class SolverType,int DGOrder,int NumberOfUnknowns,int NumberOfAuxiliaryVariables,bool EvaluateFlux,bool EvaluateNonconservativeProduct>
    void solveRiemannProblemStateless(const tarch::la::Vector<Dimensions,double>& faceCentre, const tarch::la::Vector<Dimensions,double>& cellSize, double t, double dt, int faceNumber, const double* __restrict__ faceData, double* __restrict__ solution){
        switch(faceNumber % Dimensions) {
        case 0:
          solveRiemannProblemXStateless<
            SolverType,
            DGOrder,
            NumberOfUnknowns,
            NumberOfAuxiliaryVariables,
            EvaluateFlux,
            EvaluateNonconservativeProduct
          >(
            faceCentre,
            cellSize,
            t,
            dt,
            faceData,
            solution
          );
          break;
        case 1:
          solveRiemannProblemYStateless<
            SolverType,
            DGOrder,
            NumberOfUnknowns,
            NumberOfAuxiliaryVariables,
            EvaluateFlux,
            EvaluateNonconservativeProduct
          >(
            faceCentre,
            cellSize,
            t,
            dt,
            faceData,
            solution
          );
          break;
        case 2:
          solveRiemannProblemZStateless<
            SolverType,
            DGOrder,
            NumberOfUnknowns,
            NumberOfAuxiliaryVariables,
            EvaluateFlux,
            EvaluateNonconservativeProduct
          >(
            faceCentre,
            cellSize,
            t,
            dt,
            faceData,
            solution
          );
          break;
        }
      }
    }\n""")
  
  file.close()

def create_Riemann_solver_call_dsl(polynomial_basis,
                               face_projections: FaceProjections):
  if isinstance( polynomial_basis, exahype2.solvers.GaussLegendreBasis) and face_projections==FaceProjections.Solution:
    return """solveRiemannProblemStateless<
        {{SOLVER_NAME}},
        {{ORDER}},
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{ "true" if FLUX_IMPLEMENTATION!="<none>" else "false" }}, //useFlux
        {{ "true" if NCP_IMPLEMENTATION!="<none>" else "false" }}   //useNCP
      >(
        marker.x(),
        marker.h(),
        timeStamp,
        timeStepSize,
        marker.getSelectedFaceNumber(),
        fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.value,
        fineGridFace{{UNKNOWN_IDENTIFIER}}RiemannSolution.value
      );
"""

def create_Riemann_solver_call(polynomial_basis,
                               face_projections: FaceProjections):
  """
  number_of_face_projections: Integer
    How many quantities are to be projected onto the face. If you pass in one,
    this means that only the left and right values are projected onto the face.
    If you pass in two, we store the values plus the projections of the
    derivative along the normal per face.
  """

  if isinstance( polynomial_basis, exahype2.solvers.GaussLegendreBasis) and face_projections==FaceProjections.Solution:
    return """solveRiemannProblem_pointwise_in_situ(
      [&](
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__                        F
      )->void {
        {% if FLUX_IMPLEMENTATION!="<none>" %}
        repositories::{{SOLVER_INSTANCE}}.flux(Q,x,t,dt,normal,F);
        {% endif %}
      }, //flux
      [&](
          const double * __restrict__                  Q,
          const double * __restrict__                  deltaQ,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double * __restrict__                        F
        ) -> void {
          {% if NCP_IMPLEMENTATION!="<none>" %}
          repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct(Q, deltaQ, x, t, dt, normal, F);
          {% endif %}
      }, //ncp
      [&](
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double*                                      maxEigenvalue
        ) -> void {
          repositories::{{SOLVER_INSTANCE}}.maxEigenvalue(Q, x, t, dt, normal, maxEigenvalue);
      }, // maxEigenvalue
      marker.x(),
      marker.h(),
      timeStamp,
      timeStepSize,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      marker.getSelectedFaceNumber(),
      repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
      {{ "true" if FLUX_IMPLEMENTATION!="<none>" else "false" }}, //useFlux
      {{ "true" if NCP_IMPLEMENTATION!="<none>" else "false" }},   //useNCP
      fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.value,
      fineGridFace{{UNKNOWN_IDENTIFIER}}RiemannSolution.value
    );
"""
  elif isinstance( polynomial_basis, exahype2.solvers.GaussLegendreBasis) and number_of_face_projections==2:
    return """solveRiemannProblem_pointwise_in_situ_with_gradient_projection(
      [&](
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__                        F
      )->void {
        {% if FLUX_IMPLEMENTATION!="<none>" %}
        repositories::{{SOLVER_INSTANCE}}.flux(Q,x,t,dt,normal,F);
        {% endif %}
      }, //flux
      [&](
          const double * __restrict__                  Q,
          const double * __restrict__                  deltaQ,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double * __restrict__                        F
        ) -> void {
          {% if NCP_IMPLEMENTATION!="<none>" %}
          repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct(Q, deltaQ, x, t, dt, normal, F);
          {% endif %}          
      }, //ncp
      [&](
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double*                                      maxEigenvalue
        ) -> void {
          repositories::{{SOLVER_INSTANCE}}.maxEigenvalue(Q, x, t, dt, normal, maxEigenvalue);
      }, // maxEigenvalue
      marker.x(),
      marker.h(),
      timeStamp,
      timeStepSize,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      marker.getSelectedFaceNumber(),
      repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
      {{ "true" if FLUX_IMPLEMENTATION!="<none>" else "false" }}, //useFlux
      {{ "true" if NCP_IMPLEMENTATION!="<none>" else "false" }},   //useNCP
      fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.value,
      fineGridFace{{UNKNOWN_IDENTIFIER}}RiemannSolution.value
    );
"""
  else:
    assert False, "not implemented"
    return "#not implemented"
