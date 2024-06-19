# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractKernelTemplate import AbstractKernelTemplate

class SolutionUpdate(AbstractKernelTemplate):
    def generateCode(self):
        self.render("SolutionUpdate.template.cpp", "SolutionUpdate.cpp")
