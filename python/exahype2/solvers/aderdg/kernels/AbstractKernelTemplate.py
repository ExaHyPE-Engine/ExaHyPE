# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import copy
import sys
import os

import jinja2

class AbstractKernelTemplate():
    """Base class of all ADER-DG kernel templates

    Override generateCode to implement your model.

    To generate gemms, pass the controller at construction and override
    buildGemmsConfig() (called at initialization) to put the list of MatMult
    configurations into the local context. Then generate the gemm using the
    controller during generateCode()
    """
    def __init__(self, baseContext, baseController=None):
        self.context = copy.copy(baseContext)
        self.controller = baseController
        self.buildGemmsConfig()

    def buildGemmsConfig(self):
        """Generates the list of MaltMult"""
        pass

    def generateCode(self):
        """To be overriden
        Generate the code by filling self.context and calling
        self.render(templatePath, outputPath)
        """
        sys.exit("Abstract method") # needs to be overridden

    def render(self, templateName, outputFilename, context=None):
        if context == None:
            context = self.context
        loader = jinja2.FileSystemLoader(os.path.realpath(os.path.dirname(__file__)))
        env = jinja2.Environment(loader=loader, trim_blocks=True, lstrip_blocks=True)
        if isinstance(templateName, str):
            template = env.get_template(templateName)
        else:
            template = env.get_template(os.path.join(*templateName))
        with open(os.path.join(context["pathToOutputDirectory"],outputFilename), "w") as output:
            output.write(template.render(context))
