# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from copy import deepcopy

import peano4.output

from .Gemms import Gemms
from .MathUtils import MathUtils

class FusedSpaceTimePredictorVolumeIntegral(object):
    def __init__(self, dictionary):
        self.d = deepcopy(dictionary)

    def generate_kernels(self, namespace, output, subdirectory, template_prefix, output_path):
        if self.d["IS_LINEAR"]:
            # Size of the tmp array
            self.d["TMP_ARRAY_SIZE"] = max((self.d["NUMBER_OF_DOF"] * self.d["NUMBER_OF_UNKNOWNS_PADDED"] if self.d["USE_FLUX"] else 0), \
                                           (self.d["DIMENSIONS"] * self.d["NUMBER_OF_UNKNOWNS_PADDED"] if self.d["USE_NCP"] else 0))

            if self.d["USE_POINT_SOURCE"]:
                generated_kernels = (
                    peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                        headfile_template=template_prefix + "FusedSpaceTimePredictorVolumeIntegral.template.h",
                        cppfile_template=template_prefix + "FusedSpaceTimePredictorVolumeIntegralLinear.template.cpp",
                        classname=output_path + "/FusedSpaceTimePredictorVolumeIntegral_WithoutPS",
                        namespace=namespace,
                        subdirectory=subdirectory + ".",
                        dictionary=self.d,
                        default_overwrite=True,
                        apply_iteratively=True,
                    )
                )
                output.add(generated_kernels)
                output.makefile.add_h_file(output_path + "/FusedSpaceTimePredictorVolumeIntegral_WithoutPS.h", generated=True)
                output.makefile.add_cpp_file(output_path + "/FusedSpaceTimePredictorVolumeIntegral_WithoutPS.cpp", generated=True)

                # Assembling the basis function for the point source
                QuadratureWeights, QuadratureNodes = MathUtils.getGaussLegendre(self.d["NUMBER_OF_DOF"])
                self.d["BASIS_FUNCTIONS"] = MathUtils.assembleBasisFunction(QuadratureNodes)

        generated_kernels = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                headfile_template=template_prefix + "FusedSpaceTimePredictorVolumeIntegral.template.h",
                cppfile_template=template_prefix + "FusedSpaceTimePredictorVolumeIntegralLinear.template.cpp" if self.d["IS_LINEAR"]
                else "FusedSpaceTimePredictorVolumeIntegralNonLinear.template.cpp",
                classname=output_path + "/FusedSpaceTimePredictorVolumeIntegral",
                namespace=namespace,
                subdirectory=subdirectory + ".",
                dictionary=self.d,
                default_overwrite=True,
                apply_iteratively=True,
            )
        )
        output.add(generated_kernels)
        output.makefile.add_h_file(output_path + "/FusedSpaceTimePredictorVolumeIntegral.h", generated=True)
        output.makefile.add_cpp_file(output_path + "/FusedSpaceTimePredictorVolumeIntegral.cpp", generated=True)

    def _build_gemms(self):
        # Gemms: M, N, K, LDA, LDB, LDC, alpha, beta, Align A, Align C, name, prefetching, type
        self.d["matmulConfigs"] = {}
        nVar     = self.d["nVar"]
        nVarPad  = self.d["nVarPad"]
        nDataPad = self.d["nDataPad"]
        nData    = self.d["nData"]
        nDof     = self.d["nDof"]
        nDof2    = nDof*nDof
        nDof3    = nDof2*nDof
        nDof3D   = self.d["nDof3D"]
        nDofPad  = self.d["nDofPad"]
        nDim     = self.d["nDim"]

        if self.d["isLinear"]:
            if self.d["useFlux"]:
                self.d["matmulConfigs"]["flux_x"] =     MatmulConfig(nVarPad, nDof, nDof, nVarPad      , nDofPad, nVarPad, 1, 0, 1, 1, "flux_x", "nopf", "gemm", self.d["predictorComputePrecisions"]) # beta, 0 => overwrite C
                self.d["matmulConfigs"]["flux_y"] =     MatmulConfig(nVarPad, nDof, nDof, nVarPad*nDof , nDofPad, nVarPad, 1, 0, 1, 1, "flux_y", "nopf", "gemm", self.d["predictorComputePrecisions"]) # beta, 0 => overwrite C
                if self.d["nDim"]>=3:
                    self.d["matmulConfigs"]["flux_z"] = MatmulConfig(nVarPad, nDof, nDof, nVarPad*nDof2, nDofPad, nVarPad, 1, 0, 1, 1, "flux_z", "nopf", "gemm", self.d["predictorComputePrecisions"]) # beta, 0 => overwrite C

            if self.d["useNCP"]:
                self.d["matmulConfigs"]["gradQ_x"] =     MatmulConfig(nVar, nDof, nDof, nDataPad      , nDofPad, nVarPad      , 1, 1, 1, 1, "gradQ_x", "nopf", "gemm", self.d["predictorComputePrecisions"])
                self.d["matmulConfigs"]["gradQ_y"] =     MatmulConfig(nVar, nDof, nDof, nDataPad*nDof , nDofPad, nVarPad*nDof , 1, 1, 1, 1, "gradQ_y", "nopf", "gemm", self.d["predictorComputePrecisions"])
                if self.d["nDim"]>=3:
                    self.d["matmulConfigs"]["gradQ_z"] = MatmulConfig(nVar, nDof, nDof, nDataPad*nDof2, nDofPad, nVarPad*nDof2, 1, 1, 1, 1, "gradQ_z", "nopf", "gemm", self.d["predictorComputePrecisions"])
        else: #NonLinear
            if self.d["useFlux"]:
                self.d["matmulConfigs"]["rhs_x"] =     MatmulConfig(nVarPad, nDof, nDof, nVarPad      , nDofPad, nVarPad      , 1, 1, 1, 1, "rhs_x", "nopf", "gemm", self.d["predictorComputePrecisions"])
                self.d["matmulConfigs"]["rhs_y"] =     MatmulConfig(nVarPad, nDof, nDof, nVarPad*nDof , nDofPad, nVarPad*nDof , 1, 1, 1, 1, "rhs_y", "nopf", "gemm", self.d["predictorComputePrecisions"])
                if self.d["nDim"]>=3:
                    self.d["matmulConfigs"]["rhs_z"] = MatmulConfig(nVarPad, nDof, nDof, nVarPad*nDof2, nDofPad, nVarPad*nDof2, 1, 1, 1, 1, "rhs_z", "nopf", "gemm", self.d["predictorComputePrecisions"])

                self.d["matmulConfigs"]["lduh_x"] =     MatmulConfig(nVarPad, nDof, nDof, nVarPad, nDofPad, nVarPad      , 1, 1, 1, 1,  "lduh_x",  "nopf", "gemm", self.d["predictorComputePrecisions"])
                self.d["matmulConfigs"]["lduh_y"] =     MatmulConfig(nVarPad, nDof, nDof, nVarPad, nDofPad, nVarPad*nDof , 1, 1, 1, 1,  "lduh_y",  "nopf", "gemm", self.d["predictorComputePrecisions"])
                if self.d["nDim"]>=3:
                    self.d["matmulConfigs"]["lduh_z"] = MatmulConfig(nVarPad, nDof, nDof, nVarPad, nDofPad, nVarPad*nDof2, 1, 1, 1, 1,  "lduh_z",  "nopf", "gemm", self.d["predictorComputePrecisions"])

                if self.d["useCERKGuess"]:
                    self.d["matmulConfigs"]["gradF_x_RKLoop"] =     MatmulConfig(nVar, nDof, nDof, nVarPad      , nDofPad, nVarPad      , 1, 1, 1, 1, "gradF_x_RKLoop", "nopf", "gemm", self.d["predictorComputePrecisions"])
                    self.d["matmulConfigs"]["gradF_y_RKLoop"] =     MatmulConfig(nVar, nDof, nDof, nVarPad*nDof , nDofPad, nVarPad*nDof , 1, 1, 1, 1, "gradF_y_RKLoop", "nopf", "gemm", self.d["predictorComputePrecisions"])
                    if self.d["nDim"]>=3:
                        self.d["matmulConfigs"]["gradF_z_RKLoop"] = MatmulConfig(nVar, nDof, nDof, nVarPad*nDof2, nDofPad, nVarPad*nDof2, 1, 1, 1, 1, "gradF_z_RKLoop", "nopf", "gemm", self.d["predictorComputePrecisions"])

            if self.d["useNCP"] or self.d['useViscousFlux']:
                self.d["matmulConfigs"]["gradQ_x"] =     MatmulConfig(nVar, nDof, nDof, nDataPad , nDofPad, nVarPad      , 1, 1, 1, 1, "gradQ_x", "nopf", "gemm", self.d["predictorComputePrecisions"])
                self.d["matmulConfigs"]["gradQ_y"] =     MatmulConfig(nVar, nDof, nDof, nDataPad*nDof, nDofPad, nVarPad*nDof , 1, 1, 1, 1, "gradQ_y", "nopf", "gemm", self.d["predictorComputePrecisions"])
                if self.d["nDim"]>=3:
                    self.d["matmulConfigs"]["gradQ_z"] = MatmulConfig(nVar, nDof, nDof, nDataPad*nDof2, nDofPad, nVarPad*nDof2, 1, 1, 1, 1, "gradQ_z", "nopf", "gemm", self.d["predictorComputePrecisions"])

                if self.d["useCERKGuess"]:
                    self.d["matmulConfigs"]["gradQ_x_RKLoop"] =     MatmulConfig(nVar, nDof, nDof, nData      , nDofPad, nVarPad      , 1, 1, 0, 1, "gradQ_x_RKLoop", "nopf", "gemm", self.d["predictorComputePrecisions"])
                    self.d["matmulConfigs"]["gradQ_y_RKLoop"] =     MatmulConfig(nVar, nDof, nDof, nData*nDof , nDofPad, nVarPad*nDof , 1, 1, 0, 1, "gradQ_y_RKLoop", "nopf", "gemm", self.d["predictorComputePrecisions"])
                    if self.d["nDim"]>=3:
                        self.d["matmulConfigs"]["gradQ_z_RKLoop"] = MatmulConfig(nVar, nDof, nDof, nData*nDof2, nDofPad, nVarPad*nDof2, 1, 1, 0, 1, "gradQ_z_RKLoop", "nopf", "gemm", self.d["predictorComputePrecisions"])

            self.d["matmulConfigs"]["lqi"] = MatmulConfig(nVarPad, nDof, nDof, nVarPad*(nDof**nDim), nDofPad, nVarPad, 1, 0, 1, 1, "lqi", "nopf", "gemm", self.d["predictorComputePrecisions"]) # beta, 0 => overwrite C

        if self.d["USE_LIBXSMM"]:
            self.controller.generateGemms("asm_fstpvi.c", self.d["matmulConfigs"].values())
