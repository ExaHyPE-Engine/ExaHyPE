# Copyright Spack Project Developers. See COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)

from spack.package import *
from spack_repo.builtin.packages.kokkos.package import Kokkos as BuiltinKokkos


class Kokkos(BuiltinKokkos):
    """Kokkos with the CCCL include folder of CUDA 13 for clang builds of the CUDA backend."""

    # Kokkos pull request 9437 adds include/cccl of CUDA 13 to clang builds through CCCL::CCCL.
    patch(
        "https://github.com/kokkos/kokkos/commit/e511cd4f64f6d77a3b3a24933b87073bfc397820.patch?full_index=1",
        sha256="52e756c7153d38f8e20f48d1d15d391af31760c93ee2526ddd2668f7a1639019",
        when="@5.2.2 +cuda ^cuda@13:",
    )
