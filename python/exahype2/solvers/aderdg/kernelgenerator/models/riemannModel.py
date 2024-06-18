# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .abstractModelBaseClass import AbstractModelBaseClass
import math

class RiemannModel(AbstractModelBaseClass):

    def generateCode(self):
        self.context["sqrt_half_Pi"] = math.sqrt(math.pi/2)
        
        if self.context["kernelType"] == "aderdg":
            if(self.context["isLinear"]):
                self.render(("aderdg", "riemannSolverLinear_cpp.template"),    "riemannSolver.cpp")
            else:
                self.render(("aderdg", "riemannSolverNonLinear_cpp.template"), "riemannSolver.cpp")
        elif self.context["kernelType"] == "fv":
            self.render(("fv", "riemannSolver_cpp.template"), "riemannSolver.cpp")
