# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from copy import deepcopy

import peano4.output

from .Gemms import Gemms
from .Gemms import generate_gemms


class AMRRoutines(object):
    def __init__(self, dictionary):
        self.d = deepcopy(dictionary)

    def generate_kernels(
        self, namespace, output, subdirectory, template_prefix, output_path
    ):
        self._build_gemms(namespace, output, subdirectory, template_prefix, output_path)

        generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            headfile_template=template_prefix + "AMRRoutines.template.h",
            cppfile_template=template_prefix + "AMRRoutines.template.cpp",
            classname=output_path + "/AMRRoutines",
            namespace=namespace,
            subdirectory=subdirectory + ".",
            dictionary=self.d,
            default_overwrite=True,
            apply_iteratively=True,
        )
        output.add(generated_kernels)
        output.makefile.add_h_file(output_path + "/AMRRoutines.h", generated=True)
        output.makefile.add_cpp_file(output_path + "/AMRRoutines.cpp", generated=True)

    def _build_gemms(self, namespace, output, subdirectory, template_prefix, output_path):
        # Define a sequence of GEMMS
        self.d["GEMMS"] = {}
        nDim = self.d["DIMENSIONS"]
        nDof = self.d["NUMBER_OF_DOF"]
        nDof2 = nDof * nDof
        nDofPad = self.d["NUMBER_OF_DOF_PADDED"]
        nUnknownsPad = self.d["NUMBER_OF_UNKNOWNS_PADDED"]
        nData = self.d["NUMBER_OF_DATA"]
        nDataPad = self.d["NUMBER_OF_DATA_PADDED"]

        # Always overwrite input (no need to set to 0), except if add.
        # nDim-1 face projection, inputs are padded.
        self.d["GEMMS"]["face_Q_x"] = Gemms(
            nDataPad,
            nDof,
            nDof,
            nDataPad,
            nDofPad,
            nDataPad,
            1,
            0,
            1,
            1,
            "face_Q_x",
            "nopf",
            "gemm",
            self.d["SOLUTION_STORAGE_PRECISION"]
        )
        self.d["GEMMS"]["face_F_x"] = Gemms(
            nUnknownsPad,
            nDof,
            nDof,
            nUnknownsPad,
            nDofPad,
            nUnknownsPad,
            1,
            0,
            1,
            1,
            "face_F_x",
            "nopf",
            "gemm",
            self.d["SOLUTION_STORAGE_PRECISION"]
        )
        self.d["GEMMS"]["face_F_x_add"] = Gemms(
            nUnknownsPad,
            nDof,
            nDof,
            nUnknownsPad,
            nDofPad,
            nUnknownsPad,
            1,
            1,
            1,
            1,
            "face_F_x_add",
            "nopf",
            "gemm",
            self.d["SOLUTION_STORAGE_PRECISION"]
        )
        if nDim == 3:
            self.d["GEMMS"]["face_Q_y"] = Gemms(
                nDataPad,
                nDof,
                nDof,
                nDataPad * nDof,
                nDofPad,
                nDataPad * nDof,
                1,
                0,
                1,
                1,
                "face_Q_y",
                "nopf",
                "gemm",
                self.d["SOLUTION_STORAGE_PRECISION"]
            )
            self.d["GEMMS"]["face_F_y"] = Gemms(
                nUnknownsPad,
                nDof,
                nDof,
                nUnknownsPad * nDof,
                nDofPad,
                nUnknownsPad * nDof,
                1,
                0,
                1,
                1,
                "face_F_y",
                "nopf",
                "gemm",
                self.d["SOLUTION_STORAGE_PRECISION"]
            )
            self.d["GEMMS"]["face_F_y_add"] = Gemms(
                nUnknownsPad,
                nDof,
                nDof,
                nUnknownsPad * nDof,
                nDofPad,
                nUnknownsPad * nDof,
                1,
                1,
                1,
                1,
                "face_F_y_add",
                "nopf",
                "gemm",
                self.d["SOLUTION_STORAGE_PRECISION"]
            )

        self.d["GEMMS"]["volume_x"] = Gemms(
            nData,
            nDof,
            nDof,
            nData,
            nDofPad,
            nDataPad,
            1,
            0,
            0,
            # 1,
            1,
            "volume_x",
            "nopf",
            "gemm",
            self.d["SOLUTION_STORAGE_PRECISION"]
          )

        if nDim == 3:
            self.d["GEMMS"]["volume_y"] = Gemms(nDataPad, nDof, nDof, nDataPad*nDof , nDofPad, nDataPad*nDof, 1, 0, 1, 
            # 1, 
            1, "volume_y",
            "nopf",
            "gemm",
            self.d["SOLUTION_STORAGE_PRECISION"])
            self.d["GEMMS"]["volume_z"] = Gemms(nData   , nDof, nDof, nDataPad*nDof2, nDofPad, nData*nDof2  , 1, 0, 1, 
            # 1, 
            0, "volume_z",
            "nopf",
            "gemm",
            self.d["SOLUTION_STORAGE_PRECISION"])
            self.d["GEMMS"]["volume_z_add"] = Gemms(nData, nDof, nDof, nDataPad*nDof2, nDofPad, nData*nDof2  , 1, 1, 1, 
            # 1, 
            0, "volume_z_add",
            "nopf",
            "gemm",
            self.d["SOLUTION_STORAGE_PRECISION"])
        else:
            self.d["GEMMS"]["volume_y"] = Gemms(nData   , nDof, nDof, nDataPad*nDof , nDofPad, nData*nDof   , 1, 0, 1, 
            # 1, 
            0, "volume_y",
            "nopf",
            "gemm",
            self.d["SOLUTION_STORAGE_PRECISION"])
            self.d["GEMMS"]["volume_y_add"] = Gemms(nData, nDof, nDof, nDataPad*nDof , nDofPad, nData*nDof   , 1, 1, 1, 
            # 1, 
            0, "volume_y_add",
            "nopf",
            "gemm",
            self.d["SOLUTION_STORAGE_PRECISION"])

        if self.d["USE_LIBXSMM"]:
            generate_gemms(
                output_path=output_path,
                output_file_name="AMRRoutines_libxsmm.c",
                namespace=namespace,
                dict=self.d,
                gemm_config_list=self.d["GEMMS"].values(),
            )
