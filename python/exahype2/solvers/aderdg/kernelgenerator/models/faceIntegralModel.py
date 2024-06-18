# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .abstractModelBaseClass import AbstractModelBaseClass

class FaceIntegralModel(AbstractModelBaseClass):

    def generateCode(self):
        #self.context["bndFaceSize"] = self.context["nVarPad"] * self.context["nDof"] * self.context["nDof3D"]  
        self.render(("aderdg", "faceIntegral_cpp.template"), "faceIntegral.cpp")
