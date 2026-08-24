# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from copy import deepcopy

import peano4.output


class BufferSizes(object):
    def __init__(self, dictionary):
        self.d = deepcopy(dictionary)

    def generate_kernels(self, solver, namespace, output, template_prefix, output_path):
        nUnknowns = self.d["NUMBER_OF_UNKNOWNS"]
        nUnknownsPad = self.d["NUMBER_OF_UNKNOWNS_PADDED"]
        nData = self.d["NUMBER_OF_DATA"]
        nDataPad = self.d["NUMBER_OF_DATA_PADDED"]
        nDof = self.d["NUMBER_OF_DOF"]
        nDof3D = self.d["NUMBER_OF_DOF_3D"]
        nDim = self.d["DIMENSIONS"]

        # SPT buffer sizes
        # default values
        self.d["lQiSize"] = 0
        self.d["rhsSize"] = 0
        self.d["lQiNextSize"] = 0
        self.d["lPiSize"] = 0
        self.d["lFiSize"] = 0
        self.d["lSiSize"] = 0
        self.d["lQhiSize"] = 0
        self.d["lFhiSize"] = 0
        self.d["lShiSize"] = 0
        self.d["gradQSize"] = 0
        self.d["PSiSize"] = 0

        if self.d["IS_LINEAR"]:
            self.d["lQiSize"] = nDataPad * (nDof**nDim)
            self.d["lQhiSize"] = nDataPad * (nDof**nDim)
            self.d["lFiSize"] = nDim * (nDof**nDim) * nUnknownsPad
            self.d["lFhiSize"] = nUnknownsPad * (nDof**nDim) * nDim
            if self.d["USE_SOURCE"]:
                self.d["lSiSize"] = nUnknownsPad * (nDof**nDim)
                self.d["lShiSize"] = nUnknownsPad * (nDof**nDim)
            if self.d["USE_NCP"]:
                self.d["gradQSize"] = nUnknownsPad * (nDof * nDof3D)
            if self.d["USE_POINT_SOURCE"]:
                self.d["PSiSize"] = (nDof + 1) * (nDof**nDim) * nUnknownsPad
        else:  # nonlinear
            self.d["lQiSize"] = nDataPad * (nDof ** (nDim + 1))
            # rhs holds the evolved unknowns, so it uses nUnknownsPad.
            self.d["rhsSize"] = nUnknownsPad * (nDof ** (nDim + 1))
            self.d["lQhiSize"] = nDataPad * (nDof**nDim)
            if self.d["USE_FLUX"]:
                self.d["lFiSize"] = nUnknownsPad * (nDof ** (nDim + 1)) * nDim
                self.d["lFhiSize"] = nUnknownsPad * (nDof**nDim) * nDim
            if (
                self.d["USE_SOURCE"]
                or self.d["USE_DIFFUSIVE_SOURCE"]
                or self.d["USE_NCP"]
            ):
                self.d["lSiSize"] = nUnknownsPad * (nDof ** (nDim + 1))
                self.d["lShiSize"] = nUnknownsPad * (nDof**nDim)
            if (
                self.d["USE_NCP"]
                or self.d["USE_DIFFUSIVE_FLUX"]
                or self.d["USE_DIFFUSIVE_SOURCE"]
            ):
                self.d["gradQSize"] = nUnknownsPad * (nDof**nDim) * nDim

        # Face buffer size (Riemann)
        self.d["BndFaceSize"] = nData * (nDof * nDof3D)
        self.d["BndFaceTotalSize"] = 2 * nDim * self.d["BndFaceSize"]
        self.d["BndFluxSize"] = nUnknowns * (nDof * nDof3D)
        self.d["BndFluxTotalSize"] = 2 * nDim * self.d["BndFluxSize"]

        generated_kernels = peano4.output.Jinja2TemplatedHeaderFile(
            headerfile_template=template_prefix + "BufferSizes.template.h",
            classname=output_path + "/BufferSizes",
            namespace=namespace,
            subdirectory=".",
            dictionary=self.d,
            apply_iteratively=True,
        )
        output.add(generated_kernels)
        output.makefile.add_h_file(output_path + "/BufferSizes.h", generated=True)
        solver._user_action_set_includes += f'#include "{output_path}/BufferSizes.h"\n'
