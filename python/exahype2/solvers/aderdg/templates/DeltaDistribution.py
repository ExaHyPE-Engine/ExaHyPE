# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractKernelTemplate import AbstractKernelTemplate

class DeltaDistribution(AbstractKernelTemplate):
    def generateCode(self):
        if(self.context["usePointSources"]):
            self.render("DeltaDistribution.template.cpp", "DeltaDistribution.cpp")
