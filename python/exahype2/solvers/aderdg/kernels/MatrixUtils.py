# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractKernelTemplate import AbstractKernelTemplate

class MatrixUtils(AbstractKernelTemplate):
    def generateCode(self):
        if(self.context["useVectPDEs"]):
            self.render("MatrixUtils.template.cpph", "MatrixUtils.cpph")
