# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractKernelTemplate import AbstractKernelTemplate

from .MatmulConfig import MatmulConfig

class AMRRoutines(AbstractKernelTemplate):
    def generateCode(self):
        self.render("AMRRoutines.template.cpp", "AMRRoutines.cpp")
        # generates gemms
        if(self.context["useLibxsmm"]):
            self.controller.generateGemms("asm_amrRoutines.c", self.context["matmulConfigs"].values())

    def buildGemmsConfig(self):
        # define a sequence of matmul configs
        self.context["matmulConfigs"] = {}
        # shortcut
        nVar     = self.context["nVar"]
        nVarPad  = self.context["nVarPad"]
        nData    = self.context["nData"]
        nDataPad = self.context["nDataPad"]
        nDof     = self.context["nDof"]
        nDof2    = nDof*nDof
        nDofPad  = self.context["nDofPad"]
        nDim     = self.context["nDim"]

        # Always overwrite input (no need to set to 0), except if add
        # nDim-1 face projection, inputs are padded
        self.context["matmulConfigs"]["face_Q_x"] =     MatmulConfig(nDataPad, nDof, nDof, nDataPad, nDofPad, nDataPad, 1, 0, 1, 1, "face_Q_x", "nopf", "gemm", self.context["correctorStoragePrecision"])
        self.context["matmulConfigs"]["face_F_x"] =     MatmulConfig(nVarPad , nDof, nDof, nVarPad , nDofPad, nVarPad , 1, 0, 1, 1, "face_F_x", "nopf", "gemm", self.context["correctorStoragePrecision"])
        if(nDim == 3):
            self.context["matmulConfigs"]["face_Q_y"] = MatmulConfig(nDataPad, nDof, nDof, nDataPad*nDof, nDofPad, nDataPad*nDof, 1, 0, 1, 1, "face_Q_y", "nopf", "gemm", self.context["correctorStoragePrecision"])
            self.context["matmulConfigs"]["face_F_y"] = MatmulConfig(nVarPad , nDof, nDof, nVarPad*nDof , nDofPad, nVarPad*nDof , 1, 0, 1, 1, "face_F_y", "nopf", "gemm", self.context["correctorStoragePrecision"])
        # nDim volume projection, luh (input/output) is not padded
        self.context["matmulConfigs"]["volume_x"] =      MatmulConfig(nData   , nDof, nDof, nData         , nDofPad, nDataPad     , 1, 0, 0, 1, "volume_x", "nopf", "gemm", self.context["correctorStoragePrecision"]) # input slice not aligned
        if(nDim==3):
            self.context["matmulConfigs"]["volume_y"] =  MatmulConfig(nDataPad, nDof, nDof, nDataPad*nDof , nDofPad, nDataPad*nDof, 1, 0, 1, 1, "volume_y", "nopf", "gemm", self.context["correctorStoragePrecision"])
            self.context["matmulConfigs"]["volume_z"] =  MatmulConfig(nData   , nDof, nDof, nDataPad*nDof2, nDofPad, nData*nDof2  , 1, 0, 1, 0, "volume_z", "nopf", "gemm", self.context["correctorStoragePrecision"]) # output slice not aligned
            self.context["matmulConfigs"]["volume_z_add"] = MatmulConfig(nData, nDof, nDof, nDataPad*nDof2, nDofPad, nData*nDof2  , 1, 1, 1, 0, "volume_z_add", "nopf", "gemm", self.context["correctorStoragePrecision"]) # output slice not aligned, add to result
        else:
            self.context["matmulConfigs"]["volume_y"] =  MatmulConfig(nData   , nDof, nDof, nDataPad*nDof , nDofPad, nData*nDof   , 1, 0, 1, 0, "volume_y", "nopf", "gemm", self.context["correctorStoragePrecision"]) # output slice not aligned
            self.context["matmulConfigs"]["volume_y_add"] = MatmulConfig(nData, nDof, nDof, nDataPad*nDof , nDofPad, nData*nDof   , 1, 1, 1, 0, "volume_y_add", "nopf", "gemm", self.context["correctorStoragePrecision"]) # output slice not aligned, add to result
