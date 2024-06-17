from .abstractModelBaseClass import AbstractModelBaseClass

class CellErrorIntegral(AbstractModelBaseClass):

    def generateCode(self):
        self.render(("aderdg", "cellErrorIntegral_cpp.template"), "cellErrorIntegral.cpp")