# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractKernelTemplate import AbstractKernelTemplate

class Gemms(AbstractKernelTemplate):
    def generateCode(self):
        self.render("Gemms.template.h",      "Gemms.h")
        self.render("Gemms.template.cpp",    "Gemms.cpp")
