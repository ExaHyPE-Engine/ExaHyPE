# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .abstractModelBaseClass import AbstractModelBaseClass

class MatrixUtilsModel(AbstractModelBaseClass):

    def generateCode(self):
        if(self.context["useVectPDEs"]):
            self.render(("common", "matrixUtils_cpph.template"), "matrixUtils.cpph")
