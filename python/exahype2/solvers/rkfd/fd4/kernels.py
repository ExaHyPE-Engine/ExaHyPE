# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.PDETerms  import PDETerms

import jinja2

from enum import Enum
import os
import exahype2
import exahype2.kerneldsl as DSL

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
  file.write("#endif\n")

  file.write("#if defined(GPUOffloadingSYCL)\n")
  file.write(DeriCallInExaGRyPE_sycl_accelerator_kernel)
  file.write(DeriCallInExaGRyPE_sycl_accelerator_call_with_measurement)
  file.write("#endif\n")
  file.close()

def create_FD4_kernel_definitions_3d():
  template_parameters = [DSL.SyntaxTree.Argument("NC", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("H", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("NumberOfUnknowns", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("NumberOfAuxiliaryVariables", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("KOsigma", DSL.SyntaxTree.TInteger()),
                         DSL.SyntaxTree.Argument("EvaluateFlux", DSL.SyntaxTree.TBoolean()),
                         DSL.SyntaxTree.Argument("EvaluateNonconservativeProduct", DSL.SyntaxTree.TBoolean()),
                         DSL.SyntaxTree.Argument("EvaluateSource", DSL.SyntaxTree.TBoolean()),
                         DSL.SyntaxTree.Argument("copyOldTimeStepAndScaleWithTimeStepSize", DSL.SyntaxTree.TBoolean())] #not use actually

  functor_arguments = [DSL.SyntaxTree.Argument("flux", DSL.SyntaxTree.TCustom("const Flux&")),
                       DSL.SyntaxTree.Argument("nonconservativeProduct", DSL.SyntaxTree.TCustom("const NonconservativeProduct&")),
                       DSL.SyntaxTree.Argument("sourceTerm", DSL.SyntaxTree.TCustom("const Source&"))]

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

  if not os.path.exists("kernels"):
      os.makedirs("kernels")
  
  file = open("kernels/FD4.h", "w")
  file.write(f"""#pragma once
#include "exahype2/CellData.h"
#include "exahype2/VolumeIndex.h"
#include "exahype2/fd/PatchUtils.h"
#include "exahype2/fd/Functors.h"
#include "peano4/utils/Loop.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"
             
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
#endif

#if defined(GPUOffloadingSYCL)
{FD4_sycl_accelerator_kernel_declaration}
{FD4_sycl_accelerator_call_with_measurement_declaration}
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
  file.write("#endif\n")

  file.write("#if defined(GPUOffloadingSYCL)\n")
  file.write(FD4_sycl_accelerator_kernel)
  file.write(FD4_sycl_accelerator_call_with_measurement)
  file.write("#endif\n")
  file.close()

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
