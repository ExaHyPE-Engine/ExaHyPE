# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .abstractModelBaseClass import AbstractModelBaseClass

class CellErrorIntegral(AbstractModelBaseClass):

    def generateCode(self):
        self.render(("aderdg", "cellErrorIntegral_cpp.template"), "cellErrorIntegral.cpp")