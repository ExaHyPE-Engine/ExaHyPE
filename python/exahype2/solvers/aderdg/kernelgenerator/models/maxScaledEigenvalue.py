from .abstractModelBaseClass import AbstractModelBaseClass

class MaxScaledEigenvalue(AbstractModelBaseClass):

    def generateCode(self):
        self.render(("aderdg", "maxScaledEigenvalue_cpp.template"), "maxScaledEigenvalue.cpp")