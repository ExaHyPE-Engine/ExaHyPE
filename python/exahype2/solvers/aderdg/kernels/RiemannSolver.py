# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractKernelTemplate import AbstractKernelTemplate

import math

class RiemannSolver(AbstractKernelTemplate):
    def generateCode(self):
        self.context["sqrt_half_Pi"] = math.sqrt(math.pi/2)
        if self.context["kernelType"] == "aderdg":
            if(self.context["isLinear"]):
                self.render("RiemannSolverLinear.template.cpp",    "RiemannSolver.cpp")
            else:
                self.render("RiemannSolverNonLinear.template.cpp", "RiemannSolver.cpp")
