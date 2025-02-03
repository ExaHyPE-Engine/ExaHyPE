# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from enum import Enum


class SolverVariant(Enum):
  """
  A non-enclave solver can handle PDEs with virtual functions and stateless functions.
  In many cases the stateless functions pay off performance wise, but require the user
  to additionally implement a stateless (static) version of the PDE terms.

  A enclave solver can handle virtual and stateless functions.
  If we use stateless functions with a enclave solver, we enable task fusion
  and run them in parallel in one batch either
    - on the CPU (multicore), or
    - on the GPU (accelerator).
  Some programming models such as SYCL do not allow virtual function calls in their
  parallel for loops. Therefore we restrict the enclave solver's task fusion to only allow stateless terms.
  """

  WithVirtualFunctions = 0
  Stateless            = 1
  Multicore            = 2  # Fused
  Accelerator          = 3  # Fused
