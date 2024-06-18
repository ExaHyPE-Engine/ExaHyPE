# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .abstractModelBaseClass import AbstractModelBaseClass
class GemmsCPPModel(AbstractModelBaseClass):

    def generateCode(self):
        self.render((self.context["kernelType"], "gemmsCPP_h.template"),   "gemmsCPP.h")
        self.render((self.context["kernelType"], "gemmsCPP_cpp.template"), "gemmsCPP.cpp")
