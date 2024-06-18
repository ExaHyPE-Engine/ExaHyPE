# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .abstractModelBaseClass import AbstractModelBaseClass

class MaxScaledEigenvalue(AbstractModelBaseClass):

    def generateCode(self):
        self.render(("aderdg", "maxScaledEigenvalue_cpp.template"), "maxScaledEigenvalue.cpp")