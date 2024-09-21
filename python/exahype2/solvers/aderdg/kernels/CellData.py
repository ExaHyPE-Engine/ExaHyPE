# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from copy import deepcopy

import peano4.output


class CellData(object):
    def __init__(self, dictionary):
        self.d = deepcopy(dictionary)

    def generate_kernels(
        self, namespace, output, subdirectory, template_prefix, output_path
    ):
        nUnknowns = self.d["NUMBER_OF_UNKNOWNS_PADDED"]
        nUnknownsPad = self.d["NUMBER_OF_UNKNOWNS_PADDED"]
        nAuxVar = self.d["NUMBER_OF_AUXILIARY_VARIABLES"]
        nAuxVarPad = self.d["NUMBER_OF_AUXILIARY_VARIABLES"]
        nData = self.d["NUMBER_OF_DATA"]
        nDataPad = self.d["NUMBER_OF_DATA_PADDED"]
        nDof = self.d["NUMBER_OF_DOF"]
        nDofPad = self.d["NUMBER_OF_DOF_PADDED"]
        nDof3D = self.d["NUMBER_OF_DOF_3D"]
        nDim = self.d["DIMENSIONS"]

        # SPT buffer sizes
        # default values
        self.d["lQiSize"] = -1
        self.d["lQiNextSize"] = -1
        self.d["lPiSize"] = -1
        self.d["lFiSize"] = -1
        self.d["lSiSize"] = -1
        self.d["lQhiSize"] = -1
        self.d["lFhiSize"] = -1
        self.d["lShiSize"] = -1
        self.d["gradQSize"] = -1
        self.d["PSiSize"] = -1

        if self.d["IS_LINEAR"]:
            self.d["lQiSize"] = nDataPad * (nDof**nDim)
            self.d["lQhiSize"] = nDataPad * (nDof**nDim)
            self.d["lFiSize"] = nDim * (nDof**nDim) * nUnknownsPad
            self.d["lFhiSize"] = nUnknownsPad * (nDof**nDim) * nDim
            if self.d["USE_SOURCE"]:
                self.d["lSiSize"] = nUnknownsPad * (nDof**nDim)
                self.d["lShiSize"] = nUnknownsPad * (nDof**nDim)
            if self.d["USE_NCP"]:
                self.d["gradQSize"] = nUnknownsPad * (nDof**nDim) * nDim
            if self.d["USE_POINT_SOURCE"]:
                self.d["PSiSize"] = (nDof + 1) * (nDof**nDim) * nUnknownsPad
        else:  # nonlinear
            self.d["lQiSize"] = nDataPad * (nDof ** (nDim + 1))
            self.d["lQhiSize"] = nDataPad * (nDof**nDim)
            if self.d["USE_FLUX"]:
                self.d["lFiSize"] = nUnknownsPad * (nDof ** (nDim + 1)) * nDim
                self.d["lFhiSize"] = nUnknownsPad * (nDof**nDim) * nDim
            if self.d["USE_SOURCE"] or self.d["USE_NCP"]:
                self.d["lSiSize"] = nUnknownsPad * (nDof ** (nDim + 1))
                self.d["lShiSize"] = nUnknownsPad * (nDof**nDim)
            if self.d["USE_NCP"]:
                self.d["gradQSize"] = nUnknownsPad * (nDof**nDim) * nDim

        # Face buffer size (Riemann)
        self.d["BndFaceSize"] = nDataPad * (nDof * nDof3D)
        self.d["BndFaceTotalSize"] = 2 * nDim * self.d["BndFaceSize"]
        self.d["BndFluxSize"] = nUnknownsPad * (nDof * nDof3D)
        self.d["BndFluxTotalSize"] = 2 * nDim * self.d["BndFluxSize"]

        generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            headfile_template=template_prefix + "CellData.template.h",
            cppfile_template=template_prefix + "CellData.template.cpp",
            classname=output_path + "/CellData",
            namespace=namespace,
            subdirectory=subdirectory + ".",
            dictionary=self.d,
            default_overwrite=True,
            apply_iteratively=True,
        )
        output.add(generated_kernels)
        output.makefile.add_h_file(output_path + "/CellData.h", generated=True)
        output.makefile.add_cpp_file(output_path + "/CellData.cpp", generated=True)
