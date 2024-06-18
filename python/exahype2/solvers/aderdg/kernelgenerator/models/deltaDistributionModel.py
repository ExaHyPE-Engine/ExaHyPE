# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .abstractModelBaseClass import AbstractModelBaseClass

class DeltaDistributionModel(AbstractModelBaseClass):

    def generateCode(self):
        if(self.context["usePointSources"]):
            self.render(("aderdg", "deltaDistribution_cpp.template"), "deltaDistribution.cpp")
