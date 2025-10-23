# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.PDETerms  import PDETerms

import jinja2
from enum import Enum
import os
import exahype2
import exahype2.kerneldsl as DSL


class SolverVariant(Enum):
  WithVirtualFunctions = 0
  Stateless            = 1
  Accelerator          = 2


class RiemannKernelVariant(Enum):
  PatchWiseAoSStack                = 0
  PatchWiseAoSoAStack              = 1
  PatchWiseSoAStack                = 2
  BatchedAoSStack                  = 10
  BatchedAoSoAStack                = 11
  BatchedSoAStack                  = 12
  PatchWiseSplitTermsAoSStack      = 20
  PatchWiseSplitTermsAoSoAStack    = 21
  PatchWiseAoSHeap                 = 30
  PatchWiseAoSoAHeap               = 31
  PatchWiseSoAHeap                 = 32
  BatchedAoSHeap                   = 40
  BatchedAoSoAHeap                 = 41
  BatchedSoAHeap                   = 42
  PatchWiseSplitTermsAoSHeap       = 50
  PatchWiseSplitTermsAoSoAHeap     = 51
  TaskGraphAoSHeap                 = 60
  TaskGraphAoSoAHeap               = 61
  TaskGraphSoAHeap                 = 62
  PatchWiseInSitu                  = 90
  BatchedInSitu                    = 91

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
    

def create_musclhancock_kernel_definitions():
  if not os.path.exists("kernels"):
      os.makedirs("kernels")
  
  file = open("kernels/musclhancock.h", "w")
  file.write("""#if Dimensions == 2
#include "musclhancock2d.h"
#elif Dimensions == 3
#include "musclhancock3d.h"
#endif
""")
  
  create_musclhancock_kernel_definitions_2d()
  create_musclhancock_kernel_definitions_3d()

def create_musclhancock_kernel_definitions_2d():
  template_parameters = [DSL.SyntaxTree.Argument("NV", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("H", DSL.SyntaxTree.TInteger()),
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

  musclhancock_kernel_tree = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock2d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock"], stateless=False)
  musclhancock_kernel = musclhancock_kernel_tree.print_cpp()
  musclhancock_call_with_measurement = musclhancock_kernel_tree.print_definition_with_timer()
  musclhancock_kernel_declaration = musclhancock_kernel_tree.print_declaration()
  musclhancock_call_with_measurement_declaration = musclhancock_kernel_tree.print_declaration_with_timer()

  musclhancock_stateless_kernel_tree = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock2d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock"], stateless=True)
  musclhancock_stateless_kernel = musclhancock_stateless_kernel_tree.print_cpp()
  musclhancock_stateless_call_with_measurement = musclhancock_stateless_kernel_tree.print_definition_with_timer()
  musclhancock_stateless_kernel_declaration = musclhancock_stateless_kernel_tree.print_declaration()
  musclhancock_stateless_call_with_measurement_declaration = musclhancock_stateless_kernel_tree.print_declaration_with_timer()

#  musclhancock_stateless_kernel = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock2d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock"], True).print_cpp()
#  musclhancock_stateless_call_with_measurement = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock2d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock"], True).print_definition_with_timer()
#  musclhancock_stateless_kernel_declaration = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock2d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock"], True).print_declaration()
#  musclhancock_stateless_call_with_measurement_declaration = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock2d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock"], True).print_declaration_with_timer()
#
#  musclhancock_accelerator_kernel = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock2d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock"], True, True).print_cpp()
#  musclhancock_accelerator_call_with_measurement = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock2d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock"], True, True).print_definition_with_timer()
#  musclhancock_accelerator_kernel_declaration = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock2d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock"], True, True).print_declaration()
#  musclhancock_accelerator_call_with_measurement_declaration = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock2d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock"], True, True).print_declaration_with_timer()
#
#  musclhancock_omp_kernel = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock2d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"]).print_omp()
#  musclhancock_omp_call_with_measurement = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock2d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"]).print_definition_with_timer()
#  musclhancock_omp_kernel_declaration = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock2d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"]).print_declaration()
#  musclhancock_omp_call_with_measurement_declaration = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock2d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"]).print_declaration_with_timer()
#
#  musclhancock_omp_stateless_kernel = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock2d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"], True).print_omp()
#  musclhancock_omp_stateless_call_with_measurement = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock2d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"], True).print_definition_with_timer()
#  musclhancock_omp_stateless_kernel_declaration = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock2d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"], True).print_declaration()
#  musclhancock_omp_stateless_call_with_measurement_declaration = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock2d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"], True).print_declaration_with_timer()
#
#  musclhancock_omp_accelerator_kernel = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock2d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"], True, True).print_omp()
#  musclhancock_omp_accelerator_call_with_measurement = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock2d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"], True, True).print_definition_with_timer()
#  musclhancock_omp_accelerator_kernel_declaration = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock2d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"], True, True).print_declaration()
#  musclhancock_omp_accelerator_call_with_measurement_declaration = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock2d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"], True, True).print_declaration_with_timer()

  if not os.path.exists("kernels"):
      os.makedirs("kernels")
  
  file = open("kernels/musclhancock2d.h", "w")
  file.write(f"""#pragma once
#include "exahype2/CellData.h"
#include "exahype2/VolumeIndex.h"
#include "exahype2/fv/PatchUtils.h"
#include "exahype2/fv/musclhancock/Functors.h"
#include "peano4/utils/Loop.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"

{musclhancock_kernel_declaration}
{musclhancock_call_with_measurement_declaration}   

{musclhancock_stateless_kernel_declaration}
{musclhancock_stateless_call_with_measurement_declaration}"""
#{musclhancock_accelerator_kernel_declaration}
#{musclhancock_accelerator_call_with_measurement_declaration}
#
##if defined(SharedOMP)
#{musclhancock_omp_kernel_declaration}
#{musclhancock_omp_call_with_measurement_declaration}
#{musclhancock_omp_stateless_kernel_declaration}
#{musclhancock_omp_stateless_call_with_measurement_declaration}
##endif
#
##if defined(GPUOffloadingOMP)
#{musclhancock_omp_accelerator_kernel_declaration}
#{musclhancock_omp_accelerator_call_with_measurement_declaration}
##endif

"""#include "musclhancock2d.cpph"
""")
  file.close()

  file = open("kernels/musclhancock2d.cpph", "w")
  file.write(musclhancock_kernel)
  file.write(musclhancock_call_with_measurement)
  file.write(musclhancock_stateless_kernel)
  file.write(musclhancock_stateless_call_with_measurement)
#  file.write(musclhancock_accelerator_kernel)
#  file.write(musclhancock_accelerator_call_with_measurement)
#
#  file.write("#if defined(SharedOMP)\n")
#  file.write(musclhancock_omp_kernel)
#  file.write(musclhancock_omp_call_with_measurement)
#  file.write(musclhancock_omp_stateless_kernel)
#  file.write(musclhancock_omp_stateless_call_with_measurement)
#  file.write("#endif\n")
#
#  file.write("#if defined(GPUOffloadingOMP)\n")
#  file.write(musclhancock_omp_accelerator_kernel)
#  file.write(musclhancock_omp_accelerator_call_with_measurement)
#  file.write("#endif\n")
  file.close()

def create_musclhancock_kernel_definitions_3d():
  template_parameters = [DSL.SyntaxTree.Argument("NV", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("H", DSL.SyntaxTree.TInteger()),
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

  musclhancock_kernel_tree = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock3d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock"])
  musclhancock_kernel = musclhancock_kernel_tree.print_cpp()
  musclhancock_call_with_measurement = musclhancock_kernel_tree.print_definition_with_timer()
  musclhancock_kernel_declaration = musclhancock_kernel_tree.print_declaration()
  musclhancock_call_with_measurement_declaration = musclhancock_kernel_tree.print_declaration_with_timer()

#  musclhancock_stateless_kernel = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock3d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock"], True).print_cpp()
#  musclhancock_stateless_call_with_measurement = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock3d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock"], True).print_definition_with_timer()
#  musclhancock_stateless_kernel_declaration = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock3d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock"], True).print_declaration()
#  musclhancock_stateless_call_with_measurement_declaration = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock3d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock"], True).print_declaration_with_timer()#

#  musclhancock_accelerator_kernel = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock3d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock"], True, True).print_cpp()
#  musclhancock_accelerator_call_with_measurement = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock3d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock"], True, True).print_definition_with_timer()
#  musclhancock_accelerator_kernel_declaration = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock3d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock"], True, True).print_declaration()
#  musclhancock_accelerator_call_with_measurement_declaration = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock3d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock"], True, True).print_declaration_with_timer()#

#  musclhancock_omp_kernel = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock3d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"]).print_omp()
#  musclhancock_omp_call_with_measurement = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock3d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"]).print_definition_with_timer()
#  musclhancock_omp_kernel_declaration = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock3d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"]).print_declaration()
#  musclhancock_omp_call_with_measurement_declaration = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock3d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"]).print_declaration_with_timer()#

#  musclhancock_omp_stateless_kernel = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock3d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"], True).print_omp()
#  musclhancock_omp_stateless_call_with_measurement = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock3d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"], True).print_definition_with_timer()
#  musclhancock_omp_stateless_kernel_declaration = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock3d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"], True).print_declaration()
#  musclhancock_omp_stateless_call_with_measurement_declaration = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock3d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"], True).print_declaration_with_timer()#

#  musclhancock_omp_accelerator_kernel = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock3d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"], True, True).print_omp()
#  musclhancock_omp_accelerator_call_with_measurement = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock3d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"], True, True).print_definition_with_timer()
#  musclhancock_omp_accelerator_kernel_declaration = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock3d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"], True, True).print_declaration()
#  musclhancock_omp_accelerator_call_with_measurement_declaration = DSL.Parser().parse(exahype2.solvers.fv.musclhancock.timeStepWithMUSCLHancock3d, template_parameters, functor_arguments, ["exahype2", "fv", "musclhancock", "omp"], True, True).print_declaration_with_timer()

  if not os.path.exists("kernels"):
      os.makedirs("kernels")
  
  file = open("kernels/musclhancock3d.h", "w")
  file.write(f"""#pragma once
#include "exahype2/CellData.h"
#include "exahype2/VolumeIndex.h"
#include "exahype2/fv/PatchUtils.h"
#include "exahype2/fv/musclhancock/Functors.h"
#include "peano4/utils/Loop.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"

{musclhancock_kernel_declaration}
{musclhancock_call_with_measurement_declaration}"""        

#{musclhancock_stateless_kernel_declaration}
#{musclhancock_stateless_call_with_measurement_declaration}
#{musclhancock_accelerator_kernel_declaration}
#{musclhancock_accelerator_call_with_measurement_declaration}
#
##if defined(SharedOMP)
#{musclhancock_omp_kernel_declaration}
#{musclhancock_omp_call_with_measurement_declaration}
#{musclhancock_omp_stateless_kernel_declaration}
#{musclhancock_omp_stateless_call_with_measurement_declaration}
##endif
#
##if defined(GPUOffloadingOMP)
#{musclhancock_omp_accelerator_kernel_declaration}
#{musclhancock_omp_accelerator_call_with_measurement_declaration}
##endif

"""#include "musclhancock3d.cpph"
""")
  file.close()

  file = open("kernels/musclhancock3d.cpph", "w")
  file.write(musclhancock_kernel)
  file.write(musclhancock_call_with_measurement)
#  file.write(musclhancock_stateless_kernel)
#  file.write(musclhancock_stateless_call_with_measurement)
#  file.write(musclhancock_accelerator_kernel)
#  file.write(musclhancock_accelerator_call_with_measurement)
#
#  file.write("#if defined(SharedOMP)\n")
#  file.write(musclhancock_omp_kernel)
#  file.write(musclhancock_omp_call_with_measurement)
#  file.write(musclhancock_omp_stateless_kernel)
#  file.write(musclhancock_omp_stateless_call_with_measurement)
#  file.write("#endif\n")
#
#  file.write("#if defined(GPUOffloadingOMP)\n")
#  file.write(musclhancock_omp_accelerator_kernel)
#  file.write(musclhancock_omp_accelerator_call_with_measurement)
#  file.write("#endif\n")
  file.close()

def create_compute_Riemann_kernel_for_MUSCLHancock_dsl(flux_implementation,
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

  template = "timeStepWithMUSCLHancock"

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
    >(targetDevice, patchData);
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

def create_compute_Riemann_kernel_for_MusclHancock(flux_implementation, 
                                              ncp_implementation, 
                                              source_implementation, 
                                              compute_max_eigenvalue_of_next_time_step, 
                                              solver_variant,
                                              riemann_kernel_variant):
  """
  
  I return only the unqualified function call, i.e. without any namespaces.
  So by setting the right namespace as prefix, you can direct it to particular
  implementations.
  
  solver_variant: SolverVariant
  
  riemann_kernel_variant: RiemannKernelVariant
  
  """  
  RiemannKernelCalls = {
    RiemannKernelVariant.PatchWiseAoSStack:              "timeStepWithMUSCLHancock_patchwise_call_stack",
    RiemannKernelVariant.PatchWiseAoSoAStack:            "timeStepWithMUSCLHancock_patchwise_call_stack",
    RiemannKernelVariant.PatchWiseSoAStack:              "timeStepWithMUSCLHancock_patchwise_call_stack",
    RiemannKernelVariant.BatchedAoSStack:                "timeStepWithMUSCLHancock_batched_call_stack",
    RiemannKernelVariant.BatchedAoSoAStack:              "timeStepWithMUSCLHancock_batched_call_stack",
    RiemannKernelVariant.BatchedSoAStack:                "timeStepWithMUSCLHancock_batched_call_stack",
    RiemannKernelVariant.PatchWiseSplitTermsAoSStack:    "timeStepWithMUSCLHancock_patchwise_split_terms_call_stack",
    RiemannKernelVariant.PatchWiseSplitTermsAoSoAStack:  "timeStepWithMUSCLHancock_patchwise_split_terms_call_stack",
    RiemannKernelVariant.PatchWiseAoSHeap:               "timeStepWithMUSCLHancock_patchwise_heap",
    RiemannKernelVariant.PatchWiseAoSoAHeap:             "timeStepWithMUSCLHancock_patchwise_heap",
    RiemannKernelVariant.PatchWiseSoAHeap:               "timeStepWithMUSCLHancock_patchwise_heap",
    RiemannKernelVariant.BatchedAoSHeap:                 "timeStepWithMUSCLHancock_batched_heap",
    RiemannKernelVariant.BatchedAoSoAHeap:               "timeStepWithMUSCLHancock_batched_heap",
    RiemannKernelVariant.BatchedSoAHeap:                 "timeStepWithMUSCLHancock_batched_heap",
    RiemannKernelVariant.TaskGraphAoSHeap:               "timeStepWithMUSCLHancock_batched_heap",
    RiemannKernelVariant.TaskGraphAoSoAHeap:             "timeStepWithMUSCLHancock_batched_heap",
    RiemannKernelVariant.TaskGraphSoAHeap:               "timeStepWithMUSCLHancock_batched_heap",
    RiemannKernelVariant.PatchWiseSplitTermsAoSHeap:     "timeStepWithMUSCLHancock_patchwise_split_terms_heap",
    RiemannKernelVariant.PatchWiseSplitTermsAoSoAHeap:   "timeStepWithMUSCLHancock_patchwise_split_terms_heap",
    RiemannKernelVariant.PatchWiseInSitu:                "timeStepWithMUSCLHancock_patchwise_in_situ",
    RiemannKernelVariant.BatchedInSitu:                  "timeStepWithMUSCLHancock_patchwise_in_situ"
  }
  
  EnumeratorTemplateTypes = {
    RiemannKernelVariant.PatchWiseAoSStack:              "::exahype2::enumerator::AoSLexicographicEnumerator",
    RiemannKernelVariant.PatchWiseAoSoAStack:            "::exahype2::enumerator::AoSoALexicographicEnumerator",
    RiemannKernelVariant.PatchWiseSoAStack:              "::exahype2::enumerator::SoALexicographicEnumerator",
    RiemannKernelVariant.BatchedAoSStack:                "::exahype2::enumerator::AoSLexicographicEnumerator",
    RiemannKernelVariant.BatchedAoSoAStack:              "::exahype2::enumerator::AoSoALexicographicEnumerator",
    RiemannKernelVariant.BatchedSoAStack:                "::exahype2::enumerator::SoALexicographicEnumerator",
    RiemannKernelVariant.PatchWiseSplitTermsAoSStack:    "::exahype2::enumerator::AoSLexicographicEnumerator",
    RiemannKernelVariant.PatchWiseSplitTermsAoSoAStack:  "::exahype2::enumerator::AoSoALexicographicEnumerator",
    RiemannKernelVariant.PatchWiseAoSHeap:               "::exahype2::enumerator::AoSLexicographicEnumerator",
    RiemannKernelVariant.PatchWiseAoSoAHeap:             "::exahype2::enumerator::AoSoALexicographicEnumerator",
    RiemannKernelVariant.PatchWiseSoAHeap:               "::exahype2::enumerator::SoALexicographicEnumerator",
    RiemannKernelVariant.BatchedAoSHeap:                 "::exahype2::enumerator::AoSLexicographicEnumerator",
    RiemannKernelVariant.BatchedAoSoAHeap:               "::exahype2::enumerator::AoSoALexicographicEnumerator",
    RiemannKernelVariant.BatchedSoAHeap:                 "::exahype2::enumerator::SoALexicographicEnumerator",
    RiemannKernelVariant.TaskGraphAoSHeap:               "::exahype2::enumerator::AoSLexicographicEnumerator",
    RiemannKernelVariant.TaskGraphAoSoAHeap:             "::exahype2::enumerator::AoSoALexicographicEnumerator",
    RiemannKernelVariant.TaskGraphSoAHeap:               "::exahype2::enumerator::SoALexicographicEnumerator",
    RiemannKernelVariant.PatchWiseSplitTermsAoSHeap:     "::exahype2::enumerator::AoSLexicographicEnumerator",
    RiemannKernelVariant.PatchWiseSplitTermsAoSoAHeap:   "::exahype2::enumerator::AoSoALexicographicEnumerator",
    RiemannKernelVariant.PatchWiseInSitu:           None,
    RiemannKernelVariant.BatchedInSitu:             None
  }

  template  = RiemannKernelCalls[riemann_kernel_variant]
  
  if solver_variant == SolverVariant.WithVirtualFunctions:
    template += """_functors(
  patchData,
  {{NUMBER_OF_VOLUMES_PER_AXIS}},
  {{HALO_SIZE}},
  {{NUMBER_OF_UNKNOWNS}},
  {{NUMBER_OF_AUXILIARY_VARIABLES}},
  {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
  {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
  {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
  {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %},
  [&](
    const double * __restrict__ Q,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double * __restrict__ F
  )->void {
    {% if FLUX_IMPLEMENTATION!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.flux( Q, faceCentre, volumeH, t, dt, normal, F );
    {% endif %}
  },
  [&](
    const double * __restrict__                  Q,
    const double * __restrict__     deltaQ,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double * __restrict__ BTimesDeltaQ
  )->void {
    {% if NCP_IMPLEMENTATION!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct( Q, deltaQ, faceCentre, volumeH, t, dt, normal, BTimesDeltaQ );
    {% endif %}
  },
  [&](
    const double * __restrict__ Q,
    const tarch::la::Vector<Dimensions,double>&  volumeX,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    double * __restrict__ S
  )->void {
    {% if SOURCE_IMPLEMENTATION!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.sourceTerm( Q, volumeX, volumeH, t, dt, S );
    {% endif %}
  },
  [&](
    const double * __restrict__ Q,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal
  )->double {
    return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, dt, normal);
  }
);
  """
  if solver_variant == SolverVariant.Stateless and EnumeratorTemplateTypes[riemann_kernel_variant]==None:
    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_VOLUMES_PER_AXIS}},{{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}>(
      patchData,
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %}
    );
  """
  if solver_variant == SolverVariant.Accelerator and EnumeratorTemplateTypes[riemann_kernel_variant]==None:
    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_VOLUMES_PER_AXIS}},{{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}>(
      targetDevice,
      patchData,
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %}
    );
  """
  if solver_variant == SolverVariant.Stateless and EnumeratorTemplateTypes[riemann_kernel_variant]!=None:
    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_VOLUMES_PER_AXIS}},{{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}, {{TEMP_DATA_ENUMERATOR}}>(
      patchData,
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %}
    );
  """
  if solver_variant == SolverVariant.Accelerator and EnumeratorTemplateTypes[riemann_kernel_variant]!=None:
    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_VOLUMES_PER_AXIS}},{{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}, {{TEMP_DATA_ENUMERATOR}}>(
      targetDevice,
      patchData,
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %}
    );
  """
        

  result = jinja2.Template( template, undefined=jinja2.DebugUndefined)
  d= {}
  d[ "FLUX_IMPLEMENTATION" ]         = flux_implementation
  d[ "NCP_IMPLEMENTATION" ]          = ncp_implementation
  d[ "SOURCE_IMPLEMENTATION" ]       = source_implementation
  d[ "COMPUTE_MAX_EIGENVALUE" ]      = compute_max_eigenvalue_of_next_time_step
  #if EnumeratorTemplateTypes[riemann_kernel_variant]==None:
  #  d[ "TEMP_DATA_ENUMERATOR" ]      = "::exahype2::fv::enumerator::AoSoALexicographicEnumerator"
  #else:
  d[ "TEMP_DATA_ENUMERATOR" ]      = EnumeratorTemplateTypes[riemann_kernel_variant]
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
