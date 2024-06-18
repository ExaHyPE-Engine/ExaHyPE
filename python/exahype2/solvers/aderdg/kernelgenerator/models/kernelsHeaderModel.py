# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .abstractModelBaseClass import AbstractModelBaseClass

class KernelsHeaderModel(AbstractModelBaseClass):

    def generateCode(self):

        self.context["solverNamespace"] = self.context["solverName"].split("::")[0]
        self.context["solverNamespaceList"] = self.context["solverName"].split("::")
        self.context["solverNamespaceList"].pop(-1)
        self.context["solverClass"] = self.context["solverName"].split("::")[-1]

        self.render((self.context["kernelType"], "Kernels_h.template"), "Kernels.h")
