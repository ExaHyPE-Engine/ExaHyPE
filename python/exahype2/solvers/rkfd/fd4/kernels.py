# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.PDETerms  import PDETerms

import jinja2

from enum import Enum
import os
import exahype2
import exahype2.dslhype as DSL

from exahype2.solvers.rkfd.kernels import SolverVariant    
from exahype2.solvers.rkfd.kernels import KernelVariant    

def create_FD4InExaGRyPE_kernel_definitions_3d():
  template_parameters = [DSL.SyntaxTree.Argument("NC", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("H", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("NumberOfUnknowns", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("NumberOfAuxiliaryVariables", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("KOsigma", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("EvaluateFlux", DSL.SyntaxTree.TBoolean()),
                         DSL.SyntaxTree.Argument("EvaluateNonconservativeProduct", DSL.SyntaxTree.TBoolean()),
                         DSL.SyntaxTree.Argument("EvaluateSource", DSL.SyntaxTree.TBoolean()),
                         DSL.SyntaxTree.Argument("copyOldTimeStepAndScaleWithTimeStepSize", DSL.SyntaxTree.TBoolean()), #not use actually
                         DSL.SyntaxTree.Argument("SecondOrderFormulation", DSL.SyntaxTree.TBoolean()),
                         DSL.SyntaxTree.Argument("FirstOrderFormulation", DSL.SyntaxTree.TBoolean())]

  functor_arguments = [DSL.SyntaxTree.Argument("flux", DSL.SyntaxTree.TCustom("const Flux&")),
                       DSL.SyntaxTree.Argument("nonconservativeProduct", DSL.SyntaxTree.TCustom("const NonconservativeProduct&")),
                       DSL.SyntaxTree.Argument("sourceTerm", DSL.SyntaxTree.TCustom("const Source&"))]

  FD4InExaGRyPE_tree = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4InExaGRyPE, template_parameters, functor_arguments, ["exahype2", "fd", "fd4"])
  FD4InExaGRyPE_kernel = FD4InExaGRyPE_tree.print_cpp()
  FD4InExaGRyPE_call_with_measurement = FD4InExaGRyPE_tree.print_definition_with_timer()
  FD4InExaGRyPE_kernel_declaration = FD4InExaGRyPE_tree.print_declaration()
  FD4InExaGRyPE_call_with_measurement_declaration = FD4InExaGRyPE_tree.print_declaration_with_timer()

  FD4InExaGRyPE_stateless_tree = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4InExaGRyPE, template_parameters, functor_arguments, ["exahype2", "fd", "fd4"], stateless=True)
  FD4InExaGRyPE_stateless_kernel = FD4InExaGRyPE_stateless_tree.print_cpp()
  FD4InExaGRyPE_stateless_call_with_measurement = FD4InExaGRyPE_stateless_tree.print_definition_with_timer()
  FD4InExaGRyPE_stateless_kernel_declaration = FD4InExaGRyPE_stateless_tree.print_declaration()
  FD4InExaGRyPE_stateless_call_with_measurement_declaration = FD4InExaGRyPE_stateless_tree.print_declaration_with_timer()

  #FD4InExaGRyPE_accelerator_tree = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4InExaGRyPE, template_parameters, functor_arguments, ["exahype2", "fd", "fd4"], True, True).print_cpp()
  #FD4InExaGRyPE_accelerator_kernel = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4InExaGRyPE, template_parameters, functor_arguments, ["exahype2", "fd", "fd4"], True, True).print_cpp()
  #FD4InExaGRyPE_accelerator_call_with_measurement = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4InExaGRyPE, template_parameters, functor_arguments, ["exahype2", "fd", "fd4"], True, True).print_definition_with_timer()
  #FD4InExaGRyPE_accelerator_kernel_declaration = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4InExaGRyPE, template_parameters, functor_arguments, ["exahype2", "fd", "fd4"], True, True).print_declaration()
  #FD4InExaGRyPE_accelerator_call_with_measurement_declaration = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4InExaGRyPE, template_parameters, functor_arguments, ["exahype2", "fd", "fd4"], True, True).print_declaration_with_timer()

  FD4InExaGRyPE_omp_tree = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4InExaGRyPE, template_parameters, functor_arguments, ["exahype2", "fd", "fd4", "omp"])
  FD4InExaGRyPE_omp_kernel = FD4InExaGRyPE_omp_tree.print_omp()
  FD4InExaGRyPE_omp_call_with_measurement = FD4InExaGRyPE_omp_tree.print_definition_with_timer()
  FD4InExaGRyPE_omp_kernel_declaration = FD4InExaGRyPE_omp_tree.print_declaration()
  FD4InExaGRyPE_omp_call_with_measurement_declaration = FD4InExaGRyPE_omp_tree.print_declaration_with_timer()

  FD4InExaGRyPE_omp_stateless_tree = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4InExaGRyPE, template_parameters, functor_arguments, ["exahype2", "fd", "fd4", "omp"], stateless=True)
  FD4InExaGRyPE_omp_stateless_kernel = FD4InExaGRyPE_omp_stateless_tree.print_omp()
  FD4InExaGRyPE_omp_stateless_call_with_measurement = FD4InExaGRyPE_omp_stateless_tree.print_definition_with_timer()
  FD4InExaGRyPE_omp_stateless_kernel_declaration = FD4InExaGRyPE_omp_stateless_tree.print_declaration()
  FD4InExaGRyPE_omp_stateless_call_with_measurement_declaration = FD4InExaGRyPE_omp_stateless_tree.print_declaration_with_timer()

  FD4InExaGRyPE_omp_accelerator_tree = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4InExaGRyPE, template_parameters, functor_arguments, ["exahype2", "fd", "fd4", "omp"], stateless=True, use_accelerator=True)
  FD4InExaGRyPE_omp_accelerator_kernel = FD4InExaGRyPE_omp_accelerator_tree.print_omp()
  FD4InExaGRyPE_omp_accelerator_call_with_measurement = FD4InExaGRyPE_omp_accelerator_tree.print_definition_with_timer()
  FD4InExaGRyPE_omp_accelerator_kernel_declaration = FD4InExaGRyPE_omp_accelerator_tree.print_declaration()
  FD4InExaGRyPE_omp_accelerator_call_with_measurement_declaration = FD4InExaGRyPE_omp_accelerator_tree.print_declaration_with_timer()

  FD4InExaGRyPE_sycl_accelerator_tree = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4InExaGRyPE, template_parameters, functor_arguments, ["exahype2", "fd", "fd4", "sycl"], stateless=True, use_accelerator=True)
  FD4InExaGRyPE_sycl_accelerator_kernel = FD4InExaGRyPE_sycl_accelerator_tree.print_sycl()
  FD4InExaGRyPE_sycl_accelerator_call_with_measurement = FD4InExaGRyPE_sycl_accelerator_tree.print_definition_with_timer()
  FD4InExaGRyPE_sycl_accelerator_kernel_declaration = FD4InExaGRyPE_sycl_accelerator_tree.print_declaration()
  FD4InExaGRyPE_sycl_accelerator_call_with_measurement_declaration = FD4InExaGRyPE_sycl_accelerator_tree.print_declaration_with_timer()

  template_parameters = [DSL.SyntaxTree.Argument("NC", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("NumberOfUnknowns", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("RKOrder", DSL.SyntaxTree.TInteger())]
  functor_arguments = [DSL.SyntaxTree.Argument("RKWeights", DSL.SyntaxTree.TCustom("const double*"))]

  ComputeLinearCombination_omp_accelerator_tree = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.ComputeLinearCombination, template_parameters, functor_arguments, ["exahype2", "fd", "fd4", "omp"], stateless=True, use_accelerator=True)
  ComputeLinearCombination_omp_accelerator_kernel = ComputeLinearCombination_omp_accelerator_tree.print_omp()
  ComputeLinearCombination_omp_accelerator_call_with_measurement = ComputeLinearCombination_omp_accelerator_tree.print_definition_with_timer()
  ComputeLinearCombination_omp_accelerator_kernel_declaration = ComputeLinearCombination_omp_accelerator_tree.print_declaration()
  ComputeLinearCombination_omp_accelerator_call_with_measurement_declaration = ComputeLinearCombination_omp_accelerator_tree.print_declaration_with_timer()

  if not os.path.exists("kernels"):
      os.makedirs("kernels")
  
  file = open("kernels/FD4InExaGRyPE.h", "w")
  file.write(f"""#pragma once
#include "exahype2/CellData.h"
#include "exahype2/VolumeIndex.h"
#include "exahype2/fd/PatchUtils.h"
#include "exahype2/fd/Functors.h"
#include "peano4/utils/Loop.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"
             
{FD4InExaGRyPE_kernel_declaration}
{FD4InExaGRyPE_call_with_measurement_declaration}
{FD4InExaGRyPE_stateless_kernel_declaration}
{FD4InExaGRyPE_stateless_call_with_measurement_declaration}

#if defined(SharedOMP)
{FD4InExaGRyPE_omp_kernel_declaration}
{FD4InExaGRyPE_omp_call_with_measurement_declaration}
{FD4InExaGRyPE_omp_stateless_kernel_declaration}
{FD4InExaGRyPE_omp_stateless_call_with_measurement_declaration}
#endif

#if defined(GPUOffloadingOMP)
{FD4InExaGRyPE_omp_accelerator_kernel_declaration}
{FD4InExaGRyPE_omp_accelerator_call_with_measurement_declaration}
{ComputeLinearCombination_omp_accelerator_kernel_declaration}
{ComputeLinearCombination_omp_accelerator_call_with_measurement_declaration}
#endif

#if defined(GPUOffloadingSYCL)
{FD4InExaGRyPE_sycl_accelerator_kernel_declaration}
{FD4InExaGRyPE_sycl_accelerator_call_with_measurement_declaration}
#endif

#include "FD4InExaGRyPE.cpph"
""")
  file.close()

  file = open("kernels/FD4InExaGRyPE.cpph", "w")
  file.write(FD4InExaGRyPE_kernel)
  file.write(FD4InExaGRyPE_call_with_measurement)
  file.write(FD4InExaGRyPE_stateless_kernel)
  file.write(FD4InExaGRyPE_stateless_call_with_measurement)

  file.write("#if defined(SharedOMP)\n")
  file.write(FD4InExaGRyPE_omp_kernel)
  file.write(FD4InExaGRyPE_omp_call_with_measurement)
  file.write(FD4InExaGRyPE_omp_stateless_kernel)
  file.write(FD4InExaGRyPE_omp_stateless_call_with_measurement)
  file.write("#endif\n")

  file.write("#if defined(GPUOffloadingOMP)\n")
  file.write(FD4InExaGRyPE_omp_accelerator_kernel)
  file.write(FD4InExaGRyPE_omp_accelerator_call_with_measurement)
  file.write(ComputeLinearCombination_omp_accelerator_kernel)
  file.write(ComputeLinearCombination_omp_accelerator_call_with_measurement)
  file.write("#endif\n")

  file.write("#if defined(GPUOffloadingSYCL)\n")
  file.write(FD4InExaGRyPE_sycl_accelerator_kernel)
  file.write(FD4InExaGRyPE_sycl_accelerator_call_with_measurement)
  file.write("#endif\n")
  file.close()

def create_DeriCalInExaGRyPE_kernel_definitions_3d():
  template_parameters = [DSL.SyntaxTree.Argument("NC", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("H", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("NumberOfUnknowns", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("NumberOfAuxiliaryVariables", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("SecondOrderFormulation", DSL.SyntaxTree.TBoolean())]

  functor_arguments = []

  DeriCallInExaGRyPE_tree = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.DerivativesCalInExaGRyPE, template_parameters, functor_arguments, ["exahype2", "fd", "fd4"])
  DeriCallInExaGRyPE_kernel = DeriCallInExaGRyPE_tree.print_cpp()
  DeriCallInExaGRyPE_call_with_measurement = DeriCallInExaGRyPE_tree.print_definition_with_timer()
  DeriCallInExaGRyPE_kernel_declaration = DeriCallInExaGRyPE_tree.print_declaration()
  DeriCallInExaGRyPE_call_with_measurement_declaration = DeriCallInExaGRyPE_tree.print_declaration_with_timer()

  DeriCallInExaGRyPE_stateless_tree = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.DerivativesCalInExaGRyPE, template_parameters, functor_arguments, ["exahype2", "fd", "fd4"], stateless=True)
  DeriCallInExaGRyPE_stateless_kernel = DeriCallInExaGRyPE_stateless_tree.print_cpp()
  DeriCallInExaGRyPE_stateless_call_with_measurement = DeriCallInExaGRyPE_stateless_tree.print_definition_with_timer()
  DeriCallInExaGRyPE_stateless_kernel_declaration = DeriCallInExaGRyPE_stateless_tree.print_declaration()
  DeriCallInExaGRyPE_stateless_call_with_measurement_declaration = DeriCallInExaGRyPE_stateless_tree.print_declaration_with_timer()

  #DeriCallInExaGRyPE_accelerator_tree = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.DerivativesCalInExaGRyPE, template_parameters, functor_arguments, ["exahype2", "fd", "fd4"], True, True).print_cpp()
  #DeriCallInExaGRyPE_accelerator_kernel = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.DerivativesCalInExaGRyPE, template_parameters, functor_arguments, ["exahype2", "fd", "fd4"], True, True).print_cpp()
  #DeriCallInExaGRyPE_accelerator_call_with_measurement = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.DerivativesCalInExaGRyPE, template_parameters, functor_arguments, ["exahype2", "fd", "fd4"], True, True).print_definition_with_timer()
  #DeriCallInExaGRyPE_accelerator_kernel_declaration = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.DerivativesCalInExaGRyPE, template_parameters, functor_arguments, ["exahype2", "fd", "fd4"], True, True).print_declaration()
  #DeriCallInExaGRyPE_accelerator_call_with_measurement_declaration = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.DerivativesCalInExaGRyPE, template_parameters, functor_arguments, ["exahype2", "fd", "fd4"], True, True).print_declaration_with_timer()

  DeriCallInExaGRyPE_omp_tree = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.DerivativesCalInExaGRyPE, template_parameters, functor_arguments, ["exahype2", "fd", "fd4", "omp"])
  DeriCallInExaGRyPE_omp_kernel = DeriCallInExaGRyPE_omp_tree.print_omp()
  DeriCallInExaGRyPE_omp_call_with_measurement = DeriCallInExaGRyPE_omp_tree.print_definition_with_timer()
  DeriCallInExaGRyPE_omp_kernel_declaration = DeriCallInExaGRyPE_omp_tree.print_declaration()
  DeriCallInExaGRyPE_omp_call_with_measurement_declaration = DeriCallInExaGRyPE_omp_tree.print_declaration_with_timer()

  DeriCallInExaGRyPE_omp_stateless_tree = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.DerivativesCalInExaGRyPE, template_parameters, functor_arguments, ["exahype2", "fd", "fd4", "omp"], stateless=True)
  DeriCallInExaGRyPE_omp_stateless_kernel = DeriCallInExaGRyPE_omp_stateless_tree.print_omp()
  DeriCallInExaGRyPE_omp_stateless_call_with_measurement = DeriCallInExaGRyPE_omp_stateless_tree.print_definition_with_timer()
  DeriCallInExaGRyPE_omp_stateless_kernel_declaration = DeriCallInExaGRyPE_omp_stateless_tree.print_declaration()
  DeriCallInExaGRyPE_omp_stateless_call_with_measurement_declaration = DeriCallInExaGRyPE_omp_stateless_tree.print_declaration_with_timer()

  DeriCallInExaGRyPE_omp_accelerator_tree = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.DerivativesCalInExaGRyPE, template_parameters, functor_arguments, ["exahype2", "fd", "fd4", "omp"], stateless=True, use_accelerator=True)
  DeriCallInExaGRyPE_omp_accelerator_kernel = DeriCallInExaGRyPE_omp_accelerator_tree.print_omp()
  DeriCallInExaGRyPE_omp_accelerator_call_with_measurement = DeriCallInExaGRyPE_omp_accelerator_tree.print_definition_with_timer()
  DeriCallInExaGRyPE_omp_accelerator_kernel_declaration = DeriCallInExaGRyPE_omp_accelerator_tree.print_declaration()
  DeriCallInExaGRyPE_omp_accelerator_call_with_measurement_declaration = DeriCallInExaGRyPE_omp_accelerator_tree.print_declaration_with_timer()

  DeriCallInExaGRyPE_sycl_accelerator_tree = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.DerivativesCalInExaGRyPE, template_parameters, functor_arguments, ["exahype2", "fd", "fd4", "sycl"], stateless=True, use_accelerator=True)
  DeriCallInExaGRyPE_sycl_accelerator_kernel = DeriCallInExaGRyPE_sycl_accelerator_tree.print_sycl()
  DeriCallInExaGRyPE_sycl_accelerator_call_with_measurement = DeriCallInExaGRyPE_sycl_accelerator_tree.print_definition_with_timer()
  DeriCallInExaGRyPE_sycl_accelerator_kernel_declaration = DeriCallInExaGRyPE_sycl_accelerator_tree.print_declaration()
  DeriCallInExaGRyPE_sycl_accelerator_call_with_measurement_declaration = DeriCallInExaGRyPE_sycl_accelerator_tree.print_declaration_with_timer()

  template_parameters = [DSL.SyntaxTree.Argument("NC", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("NumberOfUnknowns", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("RKOrder", DSL.SyntaxTree.TInteger())]
  functor_arguments = [DSL.SyntaxTree.Argument("RKWeights", DSL.SyntaxTree.TCustom("const double*"))]

  ComputeLinearCombination_omp_accelerator_tree = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.ComputeLinearCombination, template_parameters, functor_arguments, ["exahype2", "fd", "fd4", "omp"], stateless=True, use_accelerator=True)
  ComputeLinearCombination_omp_accelerator_kernel = ComputeLinearCombination_omp_accelerator_tree.print_omp()
  ComputeLinearCombination_omp_accelerator_call_with_measurement = ComputeLinearCombination_omp_accelerator_tree.print_definition_with_timer()
  ComputeLinearCombination_omp_accelerator_kernel_declaration = ComputeLinearCombination_omp_accelerator_tree.print_declaration()
  ComputeLinearCombination_omp_accelerator_call_with_measurement_declaration = ComputeLinearCombination_omp_accelerator_tree.print_declaration_with_timer()

  if not os.path.exists("kernels"):
      os.makedirs("kernels")
  
  file = open("kernels/DeriCallInExaGRyPE.h", "w")
  file.write(f"""#pragma once
#include "exahype2/CellData.h"
#include "exahype2/VolumeIndex.h"
#include "exahype2/fd/PatchUtils.h"
#include "exahype2/fd/Functors.h"
#include "peano4/utils/Loop.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"
             
{DeriCallInExaGRyPE_kernel_declaration}
{DeriCallInExaGRyPE_call_with_measurement_declaration}
{DeriCallInExaGRyPE_stateless_kernel_declaration}
{DeriCallInExaGRyPE_stateless_call_with_measurement_declaration}

#if defined(SharedOMP)
{DeriCallInExaGRyPE_omp_kernel_declaration}
{DeriCallInExaGRyPE_omp_call_with_measurement_declaration}
{DeriCallInExaGRyPE_omp_stateless_kernel_declaration}
{DeriCallInExaGRyPE_omp_stateless_call_with_measurement_declaration}
#endif

#if defined(GPUOffloadingOMP)
{DeriCallInExaGRyPE_omp_accelerator_kernel_declaration}
{DeriCallInExaGRyPE_omp_accelerator_call_with_measurement_declaration}
{ComputeLinearCombination_omp_accelerator_kernel_declaration}
{ComputeLinearCombination_omp_accelerator_call_with_measurement_declaration}
#endif

#if defined(GPUOffloadingSYCL)
{DeriCallInExaGRyPE_sycl_accelerator_kernel_declaration}
{DeriCallInExaGRyPE_sycl_accelerator_call_with_measurement_declaration}
#endif

#include "DeriCallInExaGRyPE.cpph"
""")
  file.close()

  file = open("kernels/DeriCallInExaGRyPE.cpph", "w")
  file.write(DeriCallInExaGRyPE_kernel)
  file.write(DeriCallInExaGRyPE_call_with_measurement)
  file.write(DeriCallInExaGRyPE_stateless_kernel)
  file.write(DeriCallInExaGRyPE_stateless_call_with_measurement)

  file.write("#if defined(SharedOMP)\n")
  file.write(DeriCallInExaGRyPE_omp_kernel)
  file.write(DeriCallInExaGRyPE_omp_call_with_measurement)
  file.write(DeriCallInExaGRyPE_omp_stateless_kernel)
  file.write(DeriCallInExaGRyPE_omp_stateless_call_with_measurement)
  file.write("#endif\n")

  file.write("#if defined(GPUOffloadingOMP)\n")
  file.write(DeriCallInExaGRyPE_omp_accelerator_kernel)
  file.write(DeriCallInExaGRyPE_omp_accelerator_call_with_measurement)
  file.write(ComputeLinearCombination_omp_accelerator_kernel)
  file.write(ComputeLinearCombination_omp_accelerator_call_with_measurement)
  file.write("#endif\n")

  file.write("#if defined(GPUOffloadingSYCL)\n")
  file.write(DeriCallInExaGRyPE_sycl_accelerator_kernel)
  file.write(DeriCallInExaGRyPE_sycl_accelerator_call_with_measurement)
  file.write("#endif\n")
  file.close()

def create_FD4_kernel_definitions_3d(patch_size, unknowns, auxiliary_variables, ko_sigma, flux_implementation, ncp_implementation, source_term_implementation, compute_max_eigenvalue_of_next_time_step):
  template_parameters = [DSL.SyntaxTree.Argument("NC", DSL.SyntaxTree.TInteger(), "applications::exahype2::ccz4::AbstractCCZ4"),
                         DSL.SyntaxTree.Argument("H", DSL.SyntaxTree.TInteger(), "applications::exahype2"),
                         DSL.SyntaxTree.Argument("NumberOfUnknowns", DSL.SyntaxTree.TInteger(), "applications::exahype2::ccz4::AbstractCCZ4"),
                         DSL.SyntaxTree.Argument("NumberOfAuxiliaryVariables", DSL.SyntaxTree.TInteger(), "applications::exahype2::ccz4::AbstractCCZ4"),
                         DSL.SyntaxTree.Argument("KOsigma", DSL.SyntaxTree.TInteger(), "applications::exahype2::ccz4::AbstractCCZ4"),
                         DSL.SyntaxTree.Argument("EvaluateFlux", DSL.SyntaxTree.TBoolean(), "applications::exahype2::ccz4"),
                         DSL.SyntaxTree.Argument("EvaluateNonconservativeProduct", DSL.SyntaxTree.TBoolean(), "applications::exahype2::ccz4"),
                         DSL.SyntaxTree.Argument("EvaluateSource", DSL.SyntaxTree.TBoolean(), "applications::exahype2::ccz4"),
                         DSL.SyntaxTree.Argument("copyOldTimeStepAndScaleWithTimeStepSize", DSL.SyntaxTree.TBoolean())] #not use actually

  functor_arguments = [DSL.SyntaxTree.Argument("flux", DSL.SyntaxTree.TCustom("const Flux&"), "(!llvm.ptr, !llvm.ptr, !llvm.ptr, f64, f64, i32, !llvm.ptr) -> ()", is_function=True),
                       DSL.SyntaxTree.Argument("nonconservativeProduct", DSL.SyntaxTree.TCustom("const NonconservativeProduct&"), "(!llvm.ptr, !llvm.ptr, !llvm.ptr, !llvm.ptr, f64, f64, i32, !llvm.ptr) -> ()", is_function=True),
                       DSL.SyntaxTree.Argument("sourceTerm", DSL.SyntaxTree.TCustom("const Source&"), "(!llvm.ptr, !llvm.ptr, !llvm.ptr, f64, f64, !llvm.ptr) -> ()", is_function=True)]

  FD4_tree = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4, template_parameters, functor_arguments, ["exahype2", "fd", "fd4"])
  FD4_kernel = FD4_tree.print_cpp()
  FD4_call_with_measurement = FD4_tree.print_definition_with_timer()
  FD4_kernel_declaration = FD4_tree.print_declaration()
  FD4_call_with_measurement_declaration = FD4_tree.print_declaration_with_timer()

  FD4_stateless_tree = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4, template_parameters, functor_arguments, ["exahype2", "fd", "fd4"], stateless=True)
  FD4_stateless_kernel = FD4_stateless_tree.print_cpp()
  FD4_stateless_call_with_measurement = FD4_stateless_tree.print_definition_with_timer()
  FD4_stateless_kernel_declaration = FD4_stateless_tree.print_declaration()
  FD4_stateless_call_with_measurement_declaration = FD4_stateless_tree.print_declaration_with_timer()

  #FD4_accelerator_tree = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4, template_parameters, functor_arguments, ["exahype2", "fd", "fd4"], True, True).print_cpp()
  #FD4_accelerator_kernel = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4, template_parameters, functor_arguments, ["exahype2", "fd", "fd4"], True, True).print_cpp()
  #FD4_accelerator_call_with_measurement = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4, template_parameters, functor_arguments, ["exahype2", "fd", "fd4"], True, True).print_definition_with_timer()
  #FD4_accelerator_kernel_declaration = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4, template_parameters, functor_arguments, ["exahype2", "fd", "fd4"], True, True).print_declaration()
  #FD4_accelerator_call_with_measurement_declaration = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4, template_parameters, functor_arguments, ["exahype2", "fd", "fd4"], True, True).print_declaration_with_timer()

  FD4_omp_tree = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4, template_parameters, functor_arguments, ["exahype2", "fd", "fd4", "omp"])
  FD4_omp_kernel = FD4_omp_tree.print_omp()
  FD4_omp_call_with_measurement = FD4_omp_tree.print_definition_with_timer()
  FD4_omp_kernel_declaration = FD4_omp_tree.print_declaration()
  FD4_omp_call_with_measurement_declaration = FD4_omp_tree.print_declaration_with_timer()

  FD4_omp_stateless_tree = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4, template_parameters, functor_arguments, ["exahype2", "fd", "fd4", "omp"], stateless=True)
  FD4_omp_stateless_kernel = FD4_omp_stateless_tree.print_omp()
  FD4_omp_stateless_call_with_measurement = FD4_omp_stateless_tree.print_definition_with_timer()
  FD4_omp_stateless_kernel_declaration = FD4_omp_stateless_tree.print_declaration()
  FD4_omp_stateless_call_with_measurement_declaration = FD4_omp_stateless_tree.print_declaration_with_timer()

  FD4_omp_accelerator_tree = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4, template_parameters, functor_arguments, ["exahype2", "fd", "fd4", "omp"], stateless=True, use_accelerator=True)
  FD4_omp_accelerator_kernel = FD4_omp_accelerator_tree.print_omp()
  FD4_omp_accelerator_call_with_measurement = FD4_omp_accelerator_tree.print_definition_with_timer()
  FD4_omp_accelerator_kernel_declaration = FD4_omp_accelerator_tree.print_declaration()
  FD4_omp_accelerator_call_with_measurement_declaration = FD4_omp_accelerator_tree.print_declaration_with_timer()

  FD4_sycl_accelerator_tree = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4, template_parameters, functor_arguments, ["exahype2", "fd", "fd4", "sycl"], stateless=True, use_accelerator=True)
  FD4_sycl_accelerator_kernel = FD4_sycl_accelerator_tree.print_sycl()
  FD4_sycl_accelerator_call_with_measurement = FD4_sycl_accelerator_tree.print_definition_with_timer()
  FD4_sycl_accelerator_kernel_declaration = FD4_sycl_accelerator_tree.print_declaration()
  FD4_sycl_accelerator_call_with_measurement_declaration = FD4_sycl_accelerator_tree.print_declaration_with_timer()

  # These function definitions are used to generate declarations in the MLIR bridge files
  FD4_mlir_function_definition = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4, template_parameters, functor_arguments, ["exahype2", "fd", "fd4", "mlir"])
  FD4_mlir_stateless_function_definition = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4, template_parameters, functor_arguments, ["exahype2", "fd", "fd4", "mlir"], stateless=True)
  FD4_mlir_stateless_omp_function_definition = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4, template_parameters, functor_arguments, ["exahype2", "fd", "fd4", "mlir", "omp"], stateless=True)
  FD4_mlir_gpu_function_definition = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4, template_parameters, functor_arguments, ["exahype2", "fd", "fd4", "mlir", "gpu"], stateless=True, use_accelerator=True)
  
  FD4_mlir_kernel = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4, template_parameters, functor_arguments, ["exahype2", "fd", "fd4"]).print_mlir()
  FD4_mlir_omp_kernel = DSL.Parser().parse(exahype2.solvers.rkfd.fd4.timeStepWithFD4, template_parameters, functor_arguments, ["exahype2", "fd", "fd4"], mlir_omp=True).print_mlir()

  mlir_cpu_function_definitions = [
    FD4_mlir_function_definition,
    FD4_mlir_stateless_function_definition,
    FD4_mlir_stateless_omp_function_definition
  ]
  
  if not os.path.exists("kernels"):
      os.makedirs("kernels")
  
  solver_includes = """
#include "exahype2/CellData.h"
#include "exahype2/VolumeIndex.h"
#include "exahype2/fd/PatchUtils.h"
#include "exahype2/fd/Functors.h"
#include "peano4/utils/Loop.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"
"""

  file = open("kernels/FD4.h", "w")
  file.write(f"""#pragma once
{solver_includes}
             
{FD4_kernel_declaration}
{FD4_call_with_measurement_declaration}
{FD4_stateless_kernel_declaration}
{FD4_stateless_call_with_measurement_declaration}

#if defined(SharedOMP)
{FD4_omp_kernel_declaration}
{FD4_omp_call_with_measurement_declaration}
{FD4_omp_stateless_kernel_declaration}
{FD4_omp_stateless_call_with_measurement_declaration}
#endif

#if defined(GPUOffloadingOMP)
{FD4_omp_accelerator_kernel_declaration}
{FD4_omp_accelerator_call_with_measurement_declaration}
{ComputeLinearCombination_omp_accelerator_kernel_declaration}
{ComputeLinearCombination_omp_accelerator_call_with_measurement_declaration}
{ComputeFinalLinearCombination_omp_accelerator_kernel_declaration}
{ComputeFinalLinearCombination_omp_accelerator_call_with_measurement_declaration}
#endif

#if defined(GPUOffloadingSYCL)
{FD4_sycl_accelerator_kernel_declaration}
{FD4_sycl_accelerator_call_with_measurement_declaration}
#endif

#ifdef USE_MLIR
#include "MLIRbridge.cpph"
#endif
#include "FD4.cpph"
""")
  file.close()

  file = open("kernels/FD4.cpph", "w")
  file.write(FD4_kernel)
  file.write(FD4_call_with_measurement)
  file.write(FD4_stateless_kernel)
  file.write(FD4_stateless_call_with_measurement)

  file.write("#if defined(SharedOMP)\n")
  file.write(FD4_omp_kernel)
  file.write(FD4_omp_call_with_measurement)
  file.write(FD4_omp_stateless_kernel)
  file.write(FD4_omp_stateless_call_with_measurement)
  file.write("#endif\n")

  file.write("#if defined(GPUOffloadingOMP)\n")
  file.write(FD4_omp_accelerator_kernel)
  file.write(FD4_omp_accelerator_call_with_measurement)
  file.write(ComputeLinearCombination_omp_accelerator_kernel)
  file.write(ComputeLinearCombination_omp_accelerator_call_with_measurement)
  file.write(ComputeFinalLinearCombination_omp_accelerator_kernel)
  file.write(ComputeFinalLinearCombination_omp_accelerator_call_with_measurement)
  file.write("#endif\n")

  file.write("#if defined(GPUOffloadingSYCL)\n")
  file.write(FD4_sycl_accelerator_kernel)
  file.write(FD4_sycl_accelerator_call_with_measurement)
  file.write("#endif\n")
  file.close()

  file = open("kernels/fd4_3d.mlir", "w")
  file.write(FD4_mlir_kernel)
  file.close()

  file = open("kernels/fd4_3d_omp.mlir", "w")
  file.write(FD4_mlir_omp_kernel)
  file.close()  

  # Generate MLIR bridge files
  with open("MLIRbridge.cpph", 'w') as file:
    file.write("""
#include "MemRefDescriptor.cpph"
// MLIR-compatible structure using memref descriptors
struct MLIRCellData {
    MemRefDescriptor2D QIn;             // Field 0
    MemRefDescriptor2D QOut;            // Field 1  
    MemRefDescriptorNested cellCentre;  // Field 2
    MemRefDescriptorNested cellSize;    // Field 3
    MemRefDescriptor1D t;               // Field 4
    MemRefDescriptor1D dt;              // Field 5
    MemRefDescriptor1D id;              // Field 6
    int64_t numberOfCells;              // Field 7
    int64_t memoryLocation;             // Field 8
    int64_t targetDevice;               // Field 9
    double** QOut_legacy;               // Field 10
    double* maxEigenvalue;              // Field 11
};
void convertToMLIRStruct(exahype2::CellData<double, double>* patchData, MLIRCellData* mlir_data);
void convertToMLIRStructGPU(exahype2::CellData<double, double>* copyCellData, MLIRCellData* mlir_data);
void cleanupMLIRStruct(MLIRCellData* mlir_data);
void cleanupMLIRStructGPU(MLIRCellData* mlir_data);
void copybackMLIRresults(exahype2::CellData<double, double>* patchData, MLIRCellData* mlir_data);
""")
    file.write("""extern "C" void timeStepWithFD4_MLIR_wrapper(exahype2::CellData<double, double>* patchData, bool use_omp);\n""")
    for definition in mlir_cpu_function_definitions:
      file.write(definition.print_declaration())
      file.write(definition.print_declaration_with_timer())
      definition._arguments[0]._type = DSL.SyntaxTree.TCustom("MLIRCellData*")
      file.write(definition.print_declaration())
      file.write(definition.print_declaration_with_timer())
    file.write("#ifdef MLIR_GPU\n")
    FD4_mlir_gpu_function_definition._arguments[1]._type = DSL.SyntaxTree.TCustom("MLIRCellData*")
    file.write(FD4_mlir_gpu_function_definition.print_declaration())
    file.write(FD4_mlir_gpu_function_definition.print_declaration_with_timer())
    file.write("#endif")
  
  d = {}
  d["KERNEL_NAMESPACE"] = "exahype2::fd::fd4"
  d["SOLVER"] = "FD4"
  d["SOLVER_CLASS"] = "FD4"
  d["KERNEL_NAME"] = "timeStepWithFD4"
  d["SOLVER_INCLUDES"] = solver_includes
  d["SOLVER_CLASS_NAMESPACE"] = "benchmarks::exahype2::kernelbenchmarks"

  d["PATCH_SIZE"] = patch_size
  d["NUMBER_OF_UNKNOWNS"] = unknowns
  d["NUMBER_OF_AUXILIARY_VARIABLES"] = auxiliary_variables
  d["KOSIGMA"] = ko_sigma
  d["FLUX_IMPLEMENTATION"] = flux_implementation
  d["NCP_IMPLEMENTATION"] = ncp_implementation
  d["SOURCE_IMPLEMENTATION"] = source_term_implementation
  d["COMPUTE_MAX_EIGENVALUE"] = compute_max_eigenvalue_of_next_time_step
  d["HALO_SIZE"] = 3

  template_strings = [argument.print_cpp() for argument in FD4_mlir_function_definition._template]
  d["TEMPLATE_DEFINITION"] = "template <" + ",".join(template_strings) + ">\n"

  template_call_jinja = """
      {{SOLVER_CLASS_NAMESPACE}}::{{SOLVER_CLASS}},
      {{PATCH_SIZE}},
      {{HALO_SIZE}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      static_cast<int>({{KOSIGMA}}),
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %}
    """
  d["TEMPLATE_CALL"] = jinja2.Template(template_call_jinja).render(**d)

  template_value_definitions_jinja = """
    int NC = {{PATCH_SIZE}};
    int NumberOfUnknowns = {{NUMBER_OF_UNKNOWNS}};
    int NumberOfAuxiliaryVariables = {{NUMBER_OF_AUXILIARY_VARIABLES}};
    int H = {{HALO_SIZE}};
    int KOsigma = static_cast<int>({{KOSIGMA}});
    bool EvaluateFlux = {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %};
    bool EvaluateNonconservativeProduct = {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %};
    bool EvaluateSource = {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %};
    bool EvaluateMaximumEigenvalueAfterTimeStep = {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %};
  """
  d["TEMPLATE_VALUE_DEFINITIONS"] = jinja2.Template(template_value_definitions_jinja).render(**d)

  filepath = os.path.realpath(__file__)
  filepath_root = filepath[0:filepath.find("solvers/")]
  with open(filepath_root + "dslhype/MLIRbridge.template.cpp", 'r') as template_file:
    mlir_bridge_template = template_file.read()
  mlir_bridge_cpp = jinja2.Template(mlir_bridge_template).render(**d)
  with open("MLIRbridge.cpp", 'w') as file:
    file.write(mlir_bridge_cpp)

def create_compute_kernel_for_FD4InExaGRyPE_DSL(flux_implementation, 
                                                ncp_implementation, 
                                                source_implementation, 
                                                compute_max_eigenvalue_of_next_time_step,
                                                solver_variant: SolverVariant,
                                                kernel_variant: KernelVariant,
                                                KOSigma,
                                                SecondOrder):
  """
  
  I return only the unqualified function call, i.e. without any namespaces.
  So by setting the right namespace as prefix, you can direct it to particular
  implementations.
  
  """  

  template  = "timeStepWithFD4InExaGRyPE"
  
  if solver_variant == SolverVariant.WithVirtualFunctions:
    template += """<
      {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
      {{HALO_SIZE}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      static_cast<int>({{KOSIGMA}}),
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      false, // Runge-Kutta, so no need to copy old data over or to take dt into account
      {% if SECONDORDER %} true {% else %} false {% endif %}, //if we have SO enabled, the quantities that get advections would be different
      {% if SECONDORDER %} false {% else %} true {% endif %} //if we have SO enabled, the quantities that get advections would be different
    >(patchData,
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
  }
);
  """
  elif solver_variant == SolverVariant.Stateless:
    template += """Stateless<
      {{SOLVER_NAME}},
      {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
      {{HALO_SIZE}}, 
      {{NUMBER_OF_UNKNOWNS}}, 
      {{NUMBER_OF_AUXILIARY_VARIABLES}}, 
      static_cast<int>({{KOSIGMA}}),
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      false, // Runge-Kutta, so no need to copy old data over or to take dt into account
      {% if SECONDORDER %} true {% else %} false {% endif %}, //if we have SO enabled, the quantities that get advections would be different
      {% if SECONDORDER %} false {% else %} true {% endif %} //if we have SO enabled, the quantities that get advections would be different
    >(patchData);
  """
  elif solver_variant == SolverVariant.AcceleratorWithExplicitCopy:
    template += """Stateless<
      {{SOLVER_NAME}},
      {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
      {{HALO_SIZE}}, 
      {{NUMBER_OF_UNKNOWNS}}, 
      {{NUMBER_OF_AUXILIARY_VARIABLES}}, 
      static_cast<int>({{KOSIGMA}}),
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      false, // Runge-Kutta, so no need to copy old data over or to take dt into account
      {% if SECONDORDER %} true {% else %} false {% endif %}, //if we have SO enabled, the quantities that get advections would be different
      {% if SECONDORDER %} false {% else %} true {% endif %} //if we have SO enabled, the quantities that get advections would be different
    >(gpuPatchData.targetDevice, gpuPatchData);
  """
  else:
    assert False, "not supported combination: {} x {}".format( solver_variant, kernel_variant )

  result = jinja2.Template( template, undefined=jinja2.DebugUndefined)
  d= {}
  d[ "FLUX_IMPLEMENTATION" ]         = flux_implementation
  d[ "NCP_IMPLEMENTATION" ]          = ncp_implementation
  d[ "SOURCE_IMPLEMENTATION" ]       = source_implementation
  d[ "COMPUTE_MAX_EIGENVALUE" ]      = compute_max_eigenvalue_of_next_time_step
  d[ "KOSIGMA" ]                     = KOSigma
  d[ "SECONDORDER" ]                 = SecondOrder
  return result.render(**d)

def create_compute_kernel_for_FD4_DSL(flux_implementation, 
                                                ncp_implementation, 
                                                source_implementation, 
                                                compute_max_eigenvalue_of_next_time_step,
                                                solver_variant: SolverVariant,
                                                kernel_variant: KernelVariant,
                                                KOSigma):
  """
  
  I return only the unqualified function call, i.e. without any namespaces.
  So by setting the right namespace as prefix, you can direct it to particular
  implementations.
  
  """  

  template  = "timeStepWithFD4"
  
  if solver_variant == SolverVariant.WithVirtualFunctions:
    template += """<
      {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
      {{HALO_SIZE}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      static_cast<int>({{KOSIGMA}}),
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      false // Runge-Kutta, so no need to copy old data over or to take dt into account
    >(patchData,
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
  }
);
  """
  elif solver_variant == SolverVariant.Stateless:
    template += """Stateless<
      {{SOLVER_NAME}},
      {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
      {{HALO_SIZE}}, 
      {{NUMBER_OF_UNKNOWNS}}, 
      {{NUMBER_OF_AUXILIARY_VARIABLES}}, 
      static_cast<int>({{KOSIGMA}}),
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      false // Runge-Kutta, so no need to copy old data over or to take dt into account
    >(patchData);
  """
  elif solver_variant == SolverVariant.AcceleratorWithExplicitCopy:
    template += """Stateless<
      {{SOLVER_NAME}},
      {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
      {{HALO_SIZE}}, 
      {{NUMBER_OF_UNKNOWNS}}, 
      {{NUMBER_OF_AUXILIARY_VARIABLES}}, 
      static_cast<int>({{KOSIGMA}}),
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      false // Runge-Kutta, so no need to copy old data over or to take dt into account
    >(gpuPatchData.targetDevice, gpuPatchData);
  """
  else:
    assert False, "not supported combination: {} x {}".format( solver_variant, kernel_variant )

  result = jinja2.Template( template, undefined=jinja2.DebugUndefined)
  d= {}
  d[ "FLUX_IMPLEMENTATION" ]         = flux_implementation
  d[ "NCP_IMPLEMENTATION" ]          = ncp_implementation
  d[ "SOURCE_IMPLEMENTATION" ]       = source_implementation
  d[ "COMPUTE_MAX_EIGENVALUE" ]      = compute_max_eigenvalue_of_next_time_step
  d[ "KOSIGMA" ]                     = KOSigma
  return result.render(**d)

def create_derivative_calculation_kernel_for_FD4InExaGRyPE_DSL(solver_variant: SolverVariant,
                                                              kernel_variant: KernelVariant,
                                                              SecondOrder):

  template  = "DerivativesCalInExaGRyPE"
  
  if solver_variant == SolverVariant.WithVirtualFunctions:
    template += """<
      {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
      {{HALO_SIZE}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {% if SECONDORDER %} true {% else %} false {% endif %} //if we have SO enabled, the quantities that get advections would be different
    >(patchData);
  """
  elif solver_variant == SolverVariant.Stateless:
    template += """Stateless<
      {{SOLVER_NAME}},
      {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
      {{HALO_SIZE}}, 
      {{NUMBER_OF_UNKNOWNS}}, 
      {{NUMBER_OF_AUXILIARY_VARIABLES}}, 
      {% if SECONDORDER %} true {% else %} false {% endif %} //if we have SO enabled, the quantities that get advections would be different
    >(patchData);
  """
  elif solver_variant == SolverVariant.AcceleratorWithExplicitCopy:
    template += """Stateless<
      {{SOLVER_NAME}},
      {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
      {{HALO_SIZE}}, 
      {{NUMBER_OF_UNKNOWNS}}, 
      {{NUMBER_OF_AUXILIARY_VARIABLES}}, 
      {% if SECONDORDER %} true {% else %} false {% endif %} //if we have SO enabled, the quantities that get advections would be different
    >(targetDevice, gpuPatchData);
  """
  else:
    assert False, "not supported combination: {} x {}".format( solver_variant, kernel_variant )

  result = jinja2.Template( template, undefined=jinja2.DebugUndefined)
  d= {}
  d[ "SECONDORDER" ]                 = SecondOrder
  return result.render(**d)

   
#def create_compute_kernel_for_FD4(flux_implementation, 
#                                  ncp_implementation, 
#                                  source_implementation, 
#                                  compute_max_eigenvalue_of_next_time_step, 
#                                  solver_variant,
#                                  kernel_variant,
#                                  KOSigma):
#  """
#  
#  I return only the unqualified function call, i.e. without any namespaces.
#  So by setting the right namespace as prefix, you can direct it to particular
#  implementations.
#  
#  """  
#  KernelCalls = {
#    KernelVariant.PatchWiseAoSHeap:               "timeStep_patchwise_heap",
#    KernelVariant.PatchWiseAoSoAHeap:             "timeStep_patchwise_heap",
#    KernelVariant.PatchWiseSoAHeap:               "timeStep_patchwise_heap",
#    KernelVariant.BatchedAoSHeap:                 "timeStep_batched_heap",
#    KernelVariant.BatchedAoSoAHeap:               "timeStep_batched_heap",
#    KernelVariant.BatchedSoAHeap:                 "timeStep_batched_heap",
#    KernelVariant.TaskGraphAoSHeap:               "timeStep_taskgraph_heap",
#    KernelVariant.TaskGraphAoSoAHeap:             "timeStep_taskgraph_heap",
#    KernelVariant.TaskGraphSoAHeap:               "timeStep_taskgraph_heap",
#  }
#  
#  EnumeratorTemplateTypes = {
#    KernelVariant.PatchWiseAoSHeap:               "::exahype2::enumerator::AoSLexicographicEnumerator",
#    KernelVariant.PatchWiseAoSoAHeap:             "::exahype2::enumerator::AoSoALexicographicEnumerator",
#    KernelVariant.PatchWiseSoAHeap:               "::exahype2::enumerator::SoALexicographicEnumerator",
#    KernelVariant.BatchedAoSHeap:                 "::exahype2::enumerator::AoSLexicographicEnumerator",
#    KernelVariant.BatchedAoSoAHeap:               "::exahype2::enumerator::AoSoALexicographicEnumerator",
#    KernelVariant.BatchedSoAHeap:                 "::exahype2::enumerator::SoALexicographicEnumerator",
#    KernelVariant.TaskGraphAoSHeap:               "::exahype2::enumerator::AoSLexicographicEnumerator",
#    KernelVariant.TaskGraphAoSoAHeap:             "::exahype2::enumerator::AoSoALexicographicEnumerator",
#    KernelVariant.TaskGraphSoAHeap:               "::exahype2::enumerator::SoALexicographicEnumerator",
#  }
#
#  template  = KernelCalls[kernel_variant]
#  
#  if solver_variant == SolverVariant.WithVirtualFunctions:
#    template += """_functors(
#  patchData,
#  {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
#  {{HALO_SIZE}},
#  {{NUMBER_OF_UNKNOWNS}},
#  {{NUMBER_OF_AUXILIARY_VARIABLES}},
#  {{KOSIGMA}},
#  {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
#  {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
#  {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
#  false, // Runge-Kutta, so no need to copy old data over or to take dt into account
#  ::exahype2::fd::fd4::DifferentialSourceTermVariant::CentralDifferencesWithLopsidedAdvection,
#  [&](
#    const double * __restrict__ Q,
#    const tarch::la::Vector<Dimensions,double>&  faceCentre,
#    const tarch::la::Vector<Dimensions,double>&  volumeH,
#    double                                       t,
#    double                                       dt,
#    int                                          normal,
#    double * __restrict__ F
#  )->void {
#    {% if FLUX_IMPLEMENTATION!="<none>" %}
#    repositories::{{SOLVER_INSTANCE}}.flux( Q, faceCentre, volumeH, t, dt, normal, F );
#    {% endif %}
#  },
#  [&](
#    const double * __restrict__                  Q,
#    const double * __restrict__     deltaQ,
#    const tarch::la::Vector<Dimensions,double>&  faceCentre,
#    const tarch::la::Vector<Dimensions,double>&  volumeH,
#    double                                       t,
#    double                                       dt,
#    int                                          normal,
#    double * __restrict__ BTimesDeltaQ
#  )->void {
#    {% if NCP_IMPLEMENTATION!="<none>" %}
#    repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct( Q, deltaQ, faceCentre, volumeH, t, dt, normal, BTimesDeltaQ );
#    {% endif %}
#  },
#  [&](
#    const double * __restrict__ Q,
#    const tarch::la::Vector<Dimensions,double>&  volumeX,
#    const tarch::la::Vector<Dimensions,double>&  volumeH,
#    double                                       t,
#    double                                       dt,
#    double * __restrict__ S
#  )->void {
#    {% if SOURCE_IMPLEMENTATION!="<none>" %}
#    repositories::{{SOLVER_INSTANCE}}.sourceTerm( Q, volumeX, volumeH, t, dt, S );
#    {% endif %}
#  }
#);
#  """
#  elif solver_variant == SolverVariant.Stateless:
#    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},{{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}, {{TEMP_DATA_ENUMERATOR}}>(
#      patchData,
#      {{KOSIGMA}},
#      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
#      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
#      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
#      false,     // Runge-Kutta, so no need to copy old data over or to take dt into account
#      ::exahype2::fd::fd4::DifferentialSourceTermVariant::CentralDifferencesWithLopsidedAdvection
#    );
#  """
#  elif solver_variant == SolverVariant.Multicore:
#    template += """_multicore_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},{{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}, {{TEMP_DATA_ENUMERATOR}}>(
#      patchData,
#      {{KOSIGMA}},
#      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
#      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
#      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
#      false,     // Runge-Kutta, so no need to copy old data over or to take dt into account
#      ::exahype2::fd::fd4::DifferentialSourceTermVariant::CentralDifferencesWithLopsidedAdvection
#    );
#  """
#  elif solver_variant == SolverVariant.AcceleratorWithExplicitCopy:
#    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},{{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}, {{TEMP_DATA_ENUMERATOR}}>(
#      targetDevice,
#      patchData,
#      {{KOSIGMA}},
#      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
#      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
#      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
#      false,     // Runge-Kutta, so no need to copy old data over or to take dt into account
#      ::exahype2::fd::fd4::DifferentialSourceTermVariant::CentralDifferencesWithLopsidedAdvection
#    );
#  """
#  else:
#    assert False, "not supported combination: {} x {}".format( solver_variant, kernel_variant )
#
#  result = jinja2.Template( template, undefined=jinja2.DebugUndefined)
#  d= {}
#  d[ "FLUX_IMPLEMENTATION" ]         = flux_implementation
#  d[ "NCP_IMPLEMENTATION" ]          = ncp_implementation
#  d[ "SOURCE_IMPLEMENTATION" ]       = source_implementation
#  d[ "COMPUTE_MAX_EIGENVALUE" ]      = compute_max_eigenvalue_of_next_time_step
#  d[ "KOSIGMA" ]                     = KOSigma
#  d[ "TEMP_DATA_ENUMERATOR" ]        = EnumeratorTemplateTypes[kernel_variant]
#  return result.render(**d)
