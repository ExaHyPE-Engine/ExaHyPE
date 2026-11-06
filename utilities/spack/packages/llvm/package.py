# Copyright Spack Project Developers. See COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)

from spack.package import *
from spack_repo.builtin.packages.llvm.package import Llvm as BuiltinLlvm


class Llvm(BuiltinLlvm):
    """LLVM with the OpenMP device runtime of every GPU target."""

    def cmake_args(self):
        args = super().cmake_args()
        if self.spec.satisfies("@21: +libomptarget"):
            targets = self.spec.variants["targets"].value
            gpuTriples = []
            if "all" in targets or "nvptx" in targets:
                gpuTriples.append("nvptx64-nvidia-cuda")
            if "all" in targets or "amdgpu" in targets:
                gpuTriples.append("amdgcn-amd-amdhsa")
            # The runtimes build compiles libomptarget-nvptx.bc and libomptarget-amdgpu.bc only
            # for the triples in LLVM_RUNTIME_TARGETS.
            if gpuTriples:
                args.append(
                    self.define("LLVM_RUNTIME_TARGETS", ["default"] + gpuTriples)
                )
                for triple in gpuTriples:
                    args.append(
                        self.define(f"RUNTIMES_{triple}_LLVM_ENABLE_RUNTIMES", "openmp")
                    )
        return args
