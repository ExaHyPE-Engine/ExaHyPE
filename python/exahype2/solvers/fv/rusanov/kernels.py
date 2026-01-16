# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from enum import Enum
import os
import exahype2
import exahype2.kerneldsl as DSL

class SolverVariant(Enum):
  WithVirtualFunctions = 0
  Stateless            = 1
  Multicore            = 2
  Accelerator          = 3

class KernelVariant(Enum):
  PatchWiseAoS    = 10
  PatchWiseAoSoA  = 11
  PatchWiseSoA    = 12
  BatchedAoS      = 20
  BatchedAoSoA    = 21
  BatchedSoA      = 22
  TaskGraphAoS    = 30
  TaskGraphAoSoA  = 31
  TaskGraphSoA    = 32
  VolumeWiseAoS   = 40
  VolumeWiseAoSoA = 41
  VolumeWiseSoA   = 42

def create_rusanov_kernel_definitions():
  if not os.path.exists("kernels"):
      os.makedirs("kernels")
  
  file = open("kernels/rusanov.h", "w")
  file.write("""#if Dimensions == 2
#include "rusanov2d.h"
#elif Dimensions == 3
#include "rusanov3d.h"
#endif
""")
  
  create_rusanov_kernel_definitions_2d()
  create_rusanov_kernel_definitions_3d()

def create_rusanov_kernel_definitions_2d():
  template_parameters = [DSL.SyntaxTree.Argument("NumberOfVolumesPerAxisInPatch", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("HaloSize", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("NumberOfUnknowns", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("NumberOfAuxiliaryVariables", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("EvaluateFlux", DSL.SyntaxTree.TBoolean()),
                         DSL.SyntaxTree.Argument("EvaluateNonconservativeProduct", DSL.SyntaxTree.TBoolean()),
                         DSL.SyntaxTree.Argument("EvaluateSource", DSL.SyntaxTree.TBoolean()),
                         DSL.SyntaxTree.Argument("EvaluateMaximumEigenvalueAfterTimeStep", DSL.SyntaxTree.TBoolean())]

  functor_arguments = [DSL.SyntaxTree.Argument("flux", DSL.SyntaxTree.TCustom("const FluxFunctor&")),
                       DSL.SyntaxTree.Argument("nonconservativeProduct", DSL.SyntaxTree.TCustom("const NonconservativeProductFunctor&")),
                       DSL.SyntaxTree.Argument("sourceTerm", DSL.SyntaxTree.TCustom("const SourceFunctor&")),
                       DSL.SyntaxTree.Argument("maxEigenvalue", DSL.SyntaxTree.TCustom("const MaxEigenvalueFunctor&"))]

  rusanov_tree = DSL.Parser().parse(exahype2.solvers.fv.rusanov.timeStepWithRusanov2d, template_parameters, functor_arguments, ["exahype2", "fv", "rusanov"])
  rusanov_kernel = rusanov_tree.print_cpp()
  rusanov_call_with_measurement = rusanov_tree.print_definition_with_timer()
  rusanov_kernel_declaration = rusanov_tree.print_declaration()
  rusanov_call_with_measurement_declaration = rusanov_tree.print_declaration_with_timer()

  rusanov_stateless_tree = DSL.Parser().parse(exahype2.solvers.fv.rusanov.timeStepWithRusanov2d, template_parameters, functor_arguments, ["exahype2", "fv", "rusanov"], stateless=True)
  rusanov_stateless_kernel = rusanov_stateless_tree.print_cpp()
  rusanov_stateless_call_with_measurement = rusanov_stateless_tree.print_definition_with_timer()
  rusanov_stateless_kernel_declaration = rusanov_stateless_tree.print_declaration()
  rusanov_stateless_call_with_measurement_declaration = rusanov_stateless_tree.print_declaration_with_timer()

  #rusanov_accelerator_tree = DSL.Parser().parse(exahype2.solvers.fv.rusanov.timeStepWithRusanov2d, template_parameters, functor_arguments, ["exahype2", "fv", "rusanov"], True, True).print_cpp()
  #rusanov_accelerator_kernel = DSL.Parser().parse(exahype2.solvers.fv.rusanov.timeStepWithRusanov2d, template_parameters, functor_arguments, ["exahype2", "fv", "rusanov"], True, True).print_cpp()
  #rusanov_accelerator_call_with_measurement = DSL.Parser().parse(exahype2.solvers.fv.rusanov.timeStepWithRusanov2d, template_parameters, functor_arguments, ["exahype2", "fv", "rusanov"], True, True).print_definition_with_timer()
  #rusanov_accelerator_kernel_declaration = DSL.Parser().parse(exahype2.solvers.fv.rusanov.timeStepWithRusanov2d, template_parameters, functor_arguments, ["exahype2", "fv", "rusanov"], True, True).print_declaration()
  #rusanov_accelerator_call_with_measurement_declaration = DSL.Parser().parse(exahype2.solvers.fv.rusanov.timeStepWithRusanov2d, template_parameters, functor_arguments, ["exahype2", "fv", "rusanov"], True, True).print_declaration_with_timer()

  rusanov_omp_tree = DSL.Parser().parse(exahype2.solvers.fv.rusanov.timeStepWithRusanov2d, template_parameters, functor_arguments, ["exahype2", "fv", "rusanov", "omp"])
  rusanov_omp_kernel = rusanov_omp_tree.print_omp()
  rusanov_omp_call_with_measurement = rusanov_omp_tree.print_definition_with_timer()
  rusanov_omp_kernel_declaration = rusanov_omp_tree.print_declaration()
  rusanov_omp_call_with_measurement_declaration = rusanov_omp_tree.print_declaration_with_timer()

  rusanov_omp_stateless_tree = DSL.Parser().parse(exahype2.solvers.fv.rusanov.timeStepWithRusanov2d, template_parameters, functor_arguments, ["exahype2", "fv", "rusanov", "omp"], stateless=True)
  rusanov_omp_stateless_kernel = rusanov_omp_stateless_tree.print_omp()
  rusanov_omp_stateless_call_with_measurement = rusanov_omp_stateless_tree.print_definition_with_timer()
  rusanov_omp_stateless_kernel_declaration = rusanov_omp_stateless_tree.print_declaration()
  rusanov_omp_stateless_call_with_measurement_declaration = rusanov_omp_stateless_tree.print_declaration_with_timer()

  rusanov_omp_accelerator_tree = DSL.Parser().parse(exahype2.solvers.fv.rusanov.timeStepWithRusanov2d, template_parameters, functor_arguments, ["exahype2", "fv", "rusanov", "omp"], stateless=True, use_accelerator=True)
  rusanov_omp_accelerator_kernel = rusanov_omp_accelerator_tree.print_omp()
  rusanov_omp_accelerator_call_with_measurement = rusanov_omp_accelerator_tree.print_definition_with_timer()
  rusanov_omp_accelerator_kernel_declaration = rusanov_omp_accelerator_tree.print_declaration()
  rusanov_omp_accelerator_call_with_measurement_declaration = rusanov_omp_accelerator_tree.print_declaration_with_timer()

  rusanov_sycl_accelerator_tree = DSL.Parser().parse(exahype2.solvers.fv.rusanov.timeStepWithRusanov2d, template_parameters, functor_arguments, ["exahype2", "fv", "rusanov", "sycl"], stateless=True, use_accelerator=True)
  rusanov_sycl_accelerator_kernel = rusanov_sycl_accelerator_tree.print_sycl()
  rusanov_sycl_accelerator_call_with_measurement = rusanov_sycl_accelerator_tree.print_definition_with_timer()
  rusanov_sycl_accelerator_kernel_declaration = rusanov_sycl_accelerator_tree.print_declaration()
  rusanov_sycl_accelerator_call_with_measurement_declaration = rusanov_sycl_accelerator_tree.print_declaration_with_timer()

  if not os.path.exists("kernels"):
      os.makedirs("kernels")
  
  file = open("kernels/rusanov2d.h", "w")
  file.write(f"""#pragma once
#include "exahype2/CellData.h"
#include "exahype2/VolumeIndex.h"
#include "exahype2/fv/PatchUtils.h"
#include "exahype2/fv/rusanov/Functors.h"
#include "peano4/utils/Loop.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"
#include <fstream>

{rusanov_kernel_declaration}
{rusanov_call_with_measurement_declaration}        

{rusanov_stateless_kernel_declaration}
{rusanov_stateless_call_with_measurement_declaration}

#if defined(SharedOMP)
{rusanov_omp_kernel_declaration}
{rusanov_omp_call_with_measurement_declaration}
{rusanov_omp_stateless_kernel_declaration}
{rusanov_omp_stateless_call_with_measurement_declaration}
#endif

#if defined(GPUOffloadingOMP)
{rusanov_omp_accelerator_kernel_declaration}
{rusanov_omp_accelerator_call_with_measurement_declaration}
#endif

#if defined(GPUOffloadingSYCL)
{rusanov_sycl_accelerator_kernel_declaration}
{rusanov_sycl_accelerator_call_with_measurement_declaration}
#endif

#include "rusanov2d.cpph"
""")
  file.close()

  file = open("kernels/rusanov2d.cpph", "w")
  file.write(rusanov_kernel)
  file.write(rusanov_call_with_measurement)
  file.write(rusanov_stateless_kernel)
  file.write(rusanov_stateless_call_with_measurement)

  file.write("#if defined(SharedOMP)\n")
  file.write(rusanov_omp_kernel)
  file.write(rusanov_omp_call_with_measurement)
  file.write(rusanov_omp_stateless_kernel)
  file.write(rusanov_omp_stateless_call_with_measurement)
  file.write("#endif\n")

  file.write("#if defined(GPUOffloadingOMP)\n")
  file.write(rusanov_omp_accelerator_kernel)
  file.write(rusanov_omp_accelerator_call_with_measurement)
  file.write("#endif\n")

  file.write("#if defined(GPUOffloadingSYCL)\n")
  file.write(rusanov_sycl_accelerator_kernel)
  file.write(rusanov_sycl_accelerator_call_with_measurement)
  file.write("#endif\n")
  file.close()

def create_rusanov_kernel_definitions_3d():
  template_parameters = [DSL.SyntaxTree.Argument("NumberOfVolumesPerAxisInPatch", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("HaloSize", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("NumberOfUnknowns", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("NumberOfAuxiliaryVariables", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("EvaluateFlux", DSL.SyntaxTree.TBoolean()),
                         DSL.SyntaxTree.Argument("EvaluateNonconservativeProduct", DSL.SyntaxTree.TBoolean()),
                         DSL.SyntaxTree.Argument("EvaluateSource", DSL.SyntaxTree.TBoolean()),
                         DSL.SyntaxTree.Argument("EvaluateMaximumEigenvalueAfterTimeStep", DSL.SyntaxTree.TBoolean())]

  functor_arguments = [DSL.SyntaxTree.Argument("flux", DSL.SyntaxTree.TCustom("const FluxFunctor&")),
                       DSL.SyntaxTree.Argument("nonconservativeProduct", DSL.SyntaxTree.TCustom("const NonconservativeProductFunctor&")),
                       DSL.SyntaxTree.Argument("sourceTerm", DSL.SyntaxTree.TCustom("const SourceFunctor&")),
                       DSL.SyntaxTree.Argument("maxEigenvalue", DSL.SyntaxTree.TCustom("const MaxEigenvalueFunctor&"))]

  rusanov_tree = DSL.Parser().parse(exahype2.solvers.fv.rusanov.timeStepWithRusanov3d, template_parameters, functor_arguments, ["exahype2", "fv", "rusanov"])
  rusanov_kernel = rusanov_tree.print_cpp()
  rusanov_call_with_measurement = rusanov_tree.print_definition_with_timer()
  rusanov_kernel_declaration = rusanov_tree.print_declaration()
  rusanov_call_with_measurement_declaration = rusanov_tree.print_declaration_with_timer()

  rusanov_stateless_tree = DSL.Parser().parse(exahype2.solvers.fv.rusanov.timeStepWithRusanov3d, template_parameters, functor_arguments, ["exahype2", "fv", "rusanov"], stateless=True)
  rusanov_stateless_kernel = rusanov_stateless_tree.print_cpp()
  rusanov_stateless_call_with_measurement = rusanov_stateless_tree.print_definition_with_timer()
  rusanov_stateless_kernel_declaration = rusanov_stateless_tree.print_declaration()
  rusanov_stateless_call_with_measurement_declaration = rusanov_stateless_tree.print_declaration_with_timer()

  rusanov_accelerator_kernel = DSL.Parser().parse(exahype2.solvers.fv.rusanov.timeStepWithRusanov3d, template_parameters, functor_arguments, ["exahype2", "fv", "rusanov"], True, True).print_cpp()
  rusanov_accelerator_call_with_measurement = DSL.Parser().parse(exahype2.solvers.fv.rusanov.timeStepWithRusanov3d, template_parameters, functor_arguments, ["exahype2", "fv", "rusanov"], True, True).print_definition_with_timer()
  rusanov_accelerator_kernel_declaration = DSL.Parser().parse(exahype2.solvers.fv.rusanov.timeStepWithRusanov3d, template_parameters, functor_arguments, ["exahype2", "fv", "rusanov"], True, True).print_declaration()
  rusanov_accelerator_call_with_measurement_declaration = DSL.Parser().parse(exahype2.solvers.fv.rusanov.timeStepWithRusanov3d, template_parameters, functor_arguments, ["exahype2", "fv", "rusanov"], True, True).print_declaration_with_timer()

  rusanov_omp_tree = DSL.Parser().parse(exahype2.solvers.fv.rusanov.timeStepWithRusanov3d, template_parameters, functor_arguments, ["exahype2", "fv", "rusanov", "omp"])
  rusanov_omp_kernel = rusanov_omp_tree.print_omp()
  rusanov_omp_call_with_measurement = rusanov_omp_tree.print_definition_with_timer()
  rusanov_omp_kernel_declaration = rusanov_omp_tree.print_declaration()
  rusanov_omp_call_with_measurement_declaration = rusanov_omp_tree.print_declaration_with_timer()

  rusanov_omp_stateless_tree = DSL.Parser().parse(exahype2.solvers.fv.rusanov.timeStepWithRusanov3d, template_parameters, functor_arguments, ["exahype2", "fv", "rusanov", "omp"], stateless=True)
  rusanov_omp_stateless_kernel = rusanov_omp_stateless_tree.print_omp()
  rusanov_omp_stateless_call_with_measurement = rusanov_omp_stateless_tree.print_definition_with_timer()
  rusanov_omp_stateless_kernel_declaration = rusanov_omp_stateless_tree.print_declaration()
  rusanov_omp_stateless_call_with_measurement_declaration = rusanov_omp_stateless_tree.print_declaration_with_timer()

  rusanov_omp_accelerator_tree = DSL.Parser().parse(exahype2.solvers.fv.rusanov.timeStepWithRusanov3d, template_parameters, functor_arguments, ["exahype2", "fv", "rusanov", "omp"], stateless=True, use_accelerator=True)
  rusanov_omp_accelerator_kernel = rusanov_omp_accelerator_tree.print_omp()
  rusanov_omp_accelerator_call_with_measurement = rusanov_omp_accelerator_tree.print_definition_with_timer()
  rusanov_omp_accelerator_kernel_declaration = rusanov_omp_accelerator_tree.print_declaration()
  rusanov_omp_accelerator_call_with_measurement_declaration = rusanov_omp_accelerator_tree.print_declaration_with_timer()

  rusanov_sycl_accelerator_tree = DSL.Parser().parse(exahype2.solvers.fv.rusanov.timeStepWithRusanov3d, template_parameters, functor_arguments, ["exahype2", "fv", "rusanov", "sycl"], stateless=True, use_accelerator=True)
  rusanov_sycl_accelerator_kernel = rusanov_sycl_accelerator_tree.print_sycl()
  rusanov_sycl_accelerator_call_with_measurement = rusanov_sycl_accelerator_tree.print_definition_with_timer()
  rusanov_sycl_accelerator_kernel_declaration = rusanov_sycl_accelerator_tree.print_declaration()
  rusanov_sycl_accelerator_call_with_measurement_declaration = rusanov_sycl_accelerator_tree.print_declaration_with_timer()

  if not os.path.exists("kernels"):
      os.makedirs("kernels")
  
  file = open("kernels/rusanov3d.h", "w")
  file.write(f"""#pragma once
#include "exahype2/CellData.h"
#include "exahype2/VolumeIndex.h"
#include "exahype2/fv/PatchUtils.h"
#include "exahype2/fv/rusanov/Functors.h"
#include "peano4/utils/Loop.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"
#include <fstream>
             
{rusanov_kernel_declaration}
{rusanov_call_with_measurement_declaration}
{rusanov_stateless_kernel_declaration}
{rusanov_stateless_call_with_measurement_declaration}
{rusanov_accelerator_kernel_declaration}
{rusanov_accelerator_call_with_measurement_declaration}

#if defined(SharedOMP)
{rusanov_omp_kernel_declaration}
{rusanov_omp_call_with_measurement_declaration}
{rusanov_omp_stateless_kernel_declaration}
{rusanov_omp_stateless_call_with_measurement_declaration}
#endif

#if defined(GPUOffloadingOMP)
{rusanov_omp_accelerator_kernel_declaration}
{rusanov_omp_accelerator_call_with_measurement_declaration}
#endif

#if defined(GPUOffloadingSYCL)
{rusanov_sycl_accelerator_kernel_declaration}
{rusanov_sycl_accelerator_call_with_measurement_declaration}
#endif

#include "rusanov3d.cpph"
""")
  file.close()

  file = open("kernels/rusanov3d.cpph", "w")
  file.write(rusanov_kernel)
  file.write(rusanov_call_with_measurement)
  file.write(rusanov_stateless_kernel)
  file.write(rusanov_stateless_call_with_measurement)
  file.write(rusanov_accelerator_kernel)
  file.write(rusanov_accelerator_call_with_measurement)

  file.write("#if defined(SharedOMP)\n")
  file.write(rusanov_omp_kernel)
  file.write(rusanov_omp_call_with_measurement)
  file.write(rusanov_omp_stateless_kernel)
  file.write(rusanov_omp_stateless_call_with_measurement)
  file.write("#endif\n")

  file.write("#if defined(GPUOffloadingOMP)\n")
  file.write(rusanov_omp_accelerator_kernel)
  file.write(rusanov_omp_accelerator_call_with_measurement)
  file.write("#endif\n")

  file.write("#if defined(GPUOffloadingSYCL)\n")
  file.write(rusanov_sycl_accelerator_kernel)
  file.write(rusanov_sycl_accelerator_call_with_measurement)
  file.write("#endif\n")

  file.close()

def create_compute_Riemann_kernel_for_Rusanov_dsl(flux_implementation,
                                              ncp_implementation,
                                              source_implementation,
                                              compute_max_eigenvalue_of_next_time_step,
                                              solver_variant: SolverVariant,
                                              kernel_variant: KernelVariant):
  """
  Return only the unqualified function call, i.e., without any namespaces.
  So by setting the right namespace as prefix, you can direct it to particular
  implementations.
  """

  template = "timeStepWithRusanov"

  if solver_variant == SolverVariant.WithVirtualFunctions:
   template += """<
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      {{HALO_SIZE}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %}
    >(patchData,
  [&](
    [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
    [[maybe_unused]] double                                       t,
    [[maybe_unused]] double                                       dt,
    [[maybe_unused]] int                                          normal,
    [[maybe_unused]] double* __restrict__                         F // F[{{NUMBER_OF_UNKNOWNS}}]
  )->void {
    {% if FLUX_IMPLEMENTATION!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.flux(Q, faceCentre, volumeH, t, dt, normal, F);
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
    {% if NCP_IMPLEMENTATION!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct(Q, deltaQ, faceCentre, volumeH, t, dt, normal, BTimesDeltaQ);
    {% endif %}
  },
  [&](
    [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeX,
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
    [[maybe_unused]] double                                       t,
    [[maybe_unused]] double                                       dt,
    [[maybe_unused]] double* __restrict__                         S // S[{{NUMBER_OF_UNKNOWNS}}]
  )->void {
    {% if SOURCE_IMPLEMENTATION!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.sourceTerm(Q, volumeX, volumeH, t, dt, S);
    {% endif %}
  },
  [&](
    [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
    [[maybe_unused]] double                                       t,
    [[maybe_unused]] double                                       dt,
    [[maybe_unused]] int                                          normal,
    [[maybe_unused]] double* __restrict__                         maxEigenvalue
  )->void {
    repositories::{{SOLVER_INSTANCE}}.maxEigenvalue(Q, faceCentre, volumeH, t, dt, normal, maxEigenvalue);
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
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %}
      >(patchData);
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
    {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
    {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %}
    >(gpuPatchData.targetDevice, gpuPatchData);
"""
  else:
    assert False, "Not supported combination: {} x {}".format(solver_variant, kernel_variant)

  result = jinja2.Template( template, undefined=jinja2.DebugUndefined)
  d= {}
  d["FLUX_IMPLEMENTATION"]         = flux_implementation
  d["NCP_IMPLEMENTATION"]          = ncp_implementation
  d["SOURCE_IMPLEMENTATION"]       = source_implementation
  d["COMPUTE_MAX_EIGENVALUE"]      = compute_max_eigenvalue_of_next_time_step
  return result.render(**d)


def create_abstract_solver_declarations(flux_implementation,
                                        ncp_implementation,
                                        eigenvalues_implementation,
                                        source_term_implementation,
                                        pde_terms_without_state):
  Template = jinja2.Template( """
  public:
    {% if EIGENVALUES_IMPLEMENTATION=="<none>" %}
    #error eigenvalue implementation cannot be none
    {% endif %}

    {% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    /**
     * Depending on the implementation, this variant might be slow as it 
     * lacks an inline define. Also, if you don't want to use ipo aggressively,
     * it might be clever to put the implementation into the header.
     *
     * ## SYCL
     *
     * At the moment, SYCL seems to struggle with ipo, even if a function is 
     * never called. So I embed the (empty) implementation directly into the
     * header.
     */
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void maxEigenvalue(
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         maxEigenvalue,
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

    /**
     * Determine max eigenvalue over Jacobian in a given point with solution values
     * (states) Q. All parameters are in.
     *
     * @return Max eigenvalue. Result has to be positive, so we are actually speaking
     *   about the maximum absolute eigenvalue.
     */
    virtual void maxEigenvalue(
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         maxEigenvalue
    ) {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final{% endif %};

    {% if FLUX_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    /**
     * Depending on the implementation, this variant might be slow as it
     * lacks an inline define. Also, if you don't want to use ipo aggressively,
     * it might be clever to put the implementation into the header.
     *
     * ## SYCL
     *
     * At the moment, SYCL seems to struggle with ipo, even if a function is
     * never called. So I embed the (empty) implementation directly into the
     * header.
     */
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void flux(
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         F, // F[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}
    
    {% if FLUX_IMPLEMENTATION!="<none>" %}
    virtual void flux(
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if FLUX_IMPLEMENTATION=="<user-defined>" %}=0{% else %} final {% endif %};
    {% endif %}

    {% if NCP_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    /**
     * Depending on the implementation, this variant might be slow as it
     * lacks an inline define. Also, if you don't want to use ipo aggressively,
     * it might be clever to put the implementation into the header.
     *
     * ## SYCL
     *
     * At the moment, SYCL seems to struggle with ipo, even if a function is
     * never called. So I embed the (empty) implementation directly into the
     * header.
     */
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void nonconservativeProduct(
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         BTimesDeltaQ, // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if NCP_IMPLEMENTATION!="<none>" %}
    virtual void nonconservativeProduct(
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if NCP_IMPLEMENTATION=="<user-defined>" %}=0{% endif %};
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    /**
     * Depending on the implementation, this variant might be slow as it
     * lacks an inline define. Also, if you don't want to use ipo aggressively,
     * it might be clever to put the implementation into the header.
     *
     * ## SYCL
     *
     * At the moment, SYCL seems to struggle with ipo, even if a function is
     * never called. So I embed the (empty) implementation directly into the
     * header.
     */
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void sourceTerm(
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] double* __restrict__                         S, // S[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    virtual void sourceTerm(
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] double* __restrict__                         S // S[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final {% endif %};
    {% endif %}
""", undefined=jinja2.DebugUndefined)

  d= {}
  d[ "FLUX_IMPLEMENTATION"]                 = flux_implementation
  d[ "NCP_IMPLEMENTATION"]                  = ncp_implementation
  d[ "EIGENVALUES_IMPLEMENTATION"]          = eigenvalues_implementation
  d[ "SOURCE_TERM_IMPLEMENTATION"]          = source_term_implementation
  d[ "STATELESS_PDE_TERMS"]                 = pde_terms_without_state
  return Template.render(**d)

def create_abstract_solver_definitions(flux_implementation, ncp_implementation, eigenvalues_implementation, source_term_implementation, pde_terms_without_state):
  Template = jinja2.Template( """
{% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and EIGENVALUES_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         maxEigenvalue
) {
  {{EIGENVALUES_IMPLEMENTATION}}
}
{% endif %}

{% if FLUX_IMPLEMENTATION!="<none>" and FLUX_IMPLEMENTATION!="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         F // F[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{FLUX_IMPLEMENTATION}}
}
{% endif %}

{% if NCP_IMPLEMENTATION!="<none>" and NCP_IMPLEMENTATION!="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{NCP_IMPLEMENTATION}}
}
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
//#if !defined(GPUOffloadingSYCL)
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] double* __restrict__                         S // S[{{NUMBER_OF_UNKNOWNS}}]
) {
  {% if SOURCE_TERM_IMPLEMENTATION!="<empty>" %}
  {{SOURCE_TERM_IMPLEMENTATION}}
  {% else %}
  std::fill_n(S,{{NUMBER_OF_UNKNOWNS}},0.0);
  {% endif %}
}
//#endif
{% endif %}

{% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
//#if !defined(GPUOffloadingSYCL)
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         maxEigenvalue,
  Offloadable
) {
  {{EIGENVALUES_IMPLEMENTATION}};
}
//#endif
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}


{% if FLUX_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
//#if !defined(GPUOffloadingSYCL)
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         F, // F[{{NUMBER_OF_UNKNOWNS}}]
  Offloadable
) {
  {% if FLUX_IMPLEMENTATION=="<none>" %}
  tarch::gpuAbort();
  {% else %}
  {{FLUX_IMPLEMENTATION}}
  {% endif %}
}
//#endif
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}

{% if NCP_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         BTimesDeltaQ, // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] Offloadable
) {
  {% if NCP_IMPLEMENTATION=="<none>" %}
  tarch::gpuAbort();
  {% else %}
  {{NCP_IMPLEMENTATION}}
  {% endif %}
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] double* __restrict__                         S, // S[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] Offloadable
) {
  {% if SOURCE_TERM_IMPLEMENTATION=="<none>" %}
  tarch::gpuAbort();
  {% else %}
  {{SOURCE_TERM_IMPLEMENTATION}}
  {% endif %}
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}
""", undefined=jinja2.DebugUndefined)

  d= {}
  d[ "FLUX_IMPLEMENTATION"]                 = flux_implementation
  d[ "NCP_IMPLEMENTATION"]                  = ncp_implementation
  d[ "EIGENVALUES_IMPLEMENTATION"]          = eigenvalues_implementation
  d[ "SOURCE_TERM_IMPLEMENTATION"]          = source_term_implementation
  d[ "STATELESS_PDE_TERMS"]                 = pde_terms_without_state
  return Template.render(**d)

def create_solver_declarations(flux_implementation, ncp_implementation, eigenvalues_implementation, source_term_implementation, pde_terms_without_state):
  Template = jinja2.Template( """
  public:
    {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
    virtual void sourceTerm(
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] double* __restrict__                         S // S[{{NUMBER_OF_UNKNOWNS}}]
    ) override;
    {% endif %}

    {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
    /**
     * Determine max eigenvalue over Jacobian in a given point with solution values
     * (states) Q. All parameters are in.
     *
     * @return Max eigenvalue. Result has to be positive, so we are actually speaking
     *   about the maximum absolute eigenvalue.
     */
    virtual void maxEigenvalue(
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         maxEigenvalue
    ) override;
    {% endif %}

    {% if FLUX_IMPLEMENTATION=="<user-defined>" %}
    virtual void flux(
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) override;
    {% endif %}

    {% if NCP_IMPLEMENTATION=="<user-defined>" %}
    virtual void nonconservativeProduct(
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
    ) override;
    {% endif %}

    {% if STATELESS_PDE_TERMS  and SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
    /**
     * To obtain the best performance, I recommend to man inline command to 
     * this signature and to copy the implementation into the header. So it would
     * read 
     *
     * static inline void sourceTerm( ... ) {
     *  code here
     * }
     *
     * The GPU offloading requires static functions. As we cannot overload the
     * original (virtual) function with a static alternative, we do the
     * TBB trick and overload by adding an additional enum. It has no semantics
     * but helps the compiler to distinguish the different function variants.
     */
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void sourceTerm(
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] double* __restrict__                         S, // S[{{NUMBER_OF_UNKNOWNS}}]
      [[maybe_unused]] Offloadable
    );
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void maxEigenvalue(
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         maxEigenvalue,
      [[maybe_unused]] Offloadable
    );
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if STATELESS_PDE_TERMS  and FLUX_IMPLEMENTATION=="<user-defined>" %}
    /**
     * To obtain the best performance, I recommend to man inline command to
     * this signature and to copy the implementation into the header. So it would
     * read
     *
     * static inline void flux( ... ) {
     *  code here
     * }
     *
     * The GPU offloading requires static functions. As we cannot overload the
     * original (virtual) function with a static alternative, we do the
     * TBB trick and overload by adding an additional enum. It has no semantics
     * but helps the compiler to distinguish the different function variants.
     */
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void flux(
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         F, // F[{{NUMBER_OF_UNKNOWNS}}]
      [[maybe_unused]] Offloadable
    );
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if STATELESS_PDE_TERMS and NCP_IMPLEMENTATION=="<user-defined>" %}
    /**
     * To obtain the best performance, I recommend to man inline command to
     * this signature and to copy the implementation into the header. So it would
     * read
     *
     * static inline void nonconservativeProduct( ... ) {
     *  code here
     * }
     *
     * The GPU offloading requires static functions. As we cannot overload the
     * original (virtual) function with a static alternative, we do the
     * TBB trick and overload by adding an additional enum. It has no semantics
     * but helps the compiler to distinguish the different function variants.
     */
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void nonconservativeProduct(
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         BTimesDeltaQ, // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
      [[maybe_unused]] Offloadable
    );
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}
""", undefined=jinja2.DebugUndefined)
  d= {}
  d[ "FLUX_IMPLEMENTATION"]                 = flux_implementation
  d[ "NCP_IMPLEMENTATION"]                  = ncp_implementation
  d[ "EIGENVALUES_IMPLEMENTATION"]          = eigenvalues_implementation
  d[ "SOURCE_TERM_IMPLEMENTATION"]          = source_term_implementation
  d[ "STATELESS_PDE_TERMS"]                 = pde_terms_without_state
  return Template.render(**d)

def create_solver_definitions(flux_implementation, ncp_implementation, eigenvalues_implementation, source_term_implementation, pde_terms_without_state):
  Template = jinja2.Template( """
{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         maxEigenvalue
) {
  logTraceInWith4Arguments( "maxEigenvalue(...)", faceCentre, volumeH, t, normal );
  // @todo implement
  logTraceOut( "maxEigenvalue(...)" );
}
{% endif %}

{% if FLUX_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         F // F[{{NUMBER_OF_UNKNOWNS}}]
) {
  logTraceInWith4Arguments("flux(...)", faceCentre, volumeH, t, normal);
  // @todo Implement
  logTraceOut("flux(...)");
}
{% endif %}

{% if NCP_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         BTimesDeltaQ // BTimesDeltaQQ[{{NUMBER_OF_UNKNOWNS}}]
) {
  logTraceInWith4Arguments("nonconservativeProduct(...)", faceCentre, volumeH, t, normal);
  // @todo Implement
  logTraceOut("nonconservativeProduct(...)");
}
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeX,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] double* __restrict__                         S // S[{{NUMBER_OF_UNKNOWNS}}]
) {
  logTraceInWith4Arguments("sourceTerm(...)", volumeX, volumeH, t, dt);

  // @todo Implement and ensure that all entries of S are properly set
  for (int i = 0; i < NumberOfUnknowns; i++) {
    S[i] = 0.0;
  }

  logTraceOut("sourceTerm(...)");
}
{% endif %}


{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         maxEigenvalue,
  Offloadable
) {
  // @todo implement
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}

{% if FLUX_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         F, // F[{{NUMBER_OF_UNKNOWNS}}]
  Offloadable
) {
  // @todo Implement
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}

{% if NCP_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         BTimesDeltaQ, // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] Offloadable
) {
  // @todo Implement
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] double* __restrict__                         S, // S[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] Offloadable
) {
  // @todo Implement but ensure that all entries of S are properly set
  for (int i = 0; i < NumberOfUnknowns; i++) {
    S[i] = 0.0;
  }
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}

""", undefined=jinja2.DebugUndefined)
  d= {}
  d[ "FLUX_IMPLEMENTATION"]                 = flux_implementation
  d[ "NCP_IMPLEMENTATION"]                  = ncp_implementation
  d[ "EIGENVALUES_IMPLEMENTATION"]          = eigenvalues_implementation
  d[ "SOURCE_TERM_IMPLEMENTATION"]          = source_term_implementation
  d[ "STATELESS_PDE_TERMS"]                 = pde_terms_without_state
  return Template.render(**d)
