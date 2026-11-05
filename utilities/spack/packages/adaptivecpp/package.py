# Copyright Spack Project Developers. See COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)

from spack.package import *


class Adaptivecpp(CMakePackage):
    """AdaptiveCpp implements SYCL and C++ standard parallelism for CPUs and GPUs."""

    homepage = "https://adaptivecpp.github.io"
    url = "https://github.com/AdaptiveCpp/AdaptiveCpp/archive/refs/tags/v25.10.0.tar.gz"
    git = "https://github.com/AdaptiveCpp/AdaptiveCpp.git"


    license("BSD-2-Clause")

    version(
        "25.10.0",
        sha256="334b16ebff373bd2841f83332c2ae9a45ec192f2cf964d5fdfe94e1140776059",
    )

    variant("cuda", default=False, description="Build the CUDA backend for NVIDIA GPUs")
    variant("rocm", default=False, description="Build the HIP backend for AMD GPUs")
    variant(
        "level_zero",
        default=False,
        description="Build the Level Zero backend for Intel GPUs",
    )

    depends_on("c", type="build")
    depends_on("cxx", type="build")
    depends_on("cmake@3.10:", type="build")
    depends_on("git", type="build", when="+level_zero")
    depends_on("python@3:", type=("build", "run"))

    depends_on("llvm@15:20 +clang", when="@25.10")

    depends_on("cuda@10:12", when="+cuda")

    depends_on("llvm targets=amdgpu", when="+rocm")
    depends_on("hip", when="+rocm")
    depends_on("hsa-rocr-dev", when="+rocm")

    # The clang of AdaptiveCpp reads only device libraries that an LLVM up to its own version wrote.
    resource(
        name="rocm-device-libs",
        url="https://github.com/ROCm/llvm-project/archive/refs/tags/rocm-6.3.3.tar.gz",
        sha256="4df9aba24e574edf23844c0d2d9dda112811db5c2b08c9428604a21b819eb23d",
        destination="rocm-device-libs",
        when="+rocm",
    )

    depends_on("oneapi-level-zero", when="+level_zero")

    def cmake_args(self):
        llvm = self.spec["llvm"]
        args = [
            self.define("LLVM_DIR", llvm.prefix.lib.cmake.llvm),
            self.define("CLANG_EXECUTABLE_PATH", llvm.prefix.bin.join("clang++")),
            self.define("ACPP_COMPILER_FEATURE_PROFILE", "full"),
            self.define_from_variant("WITH_CUDA_BACKEND", "cuda"),
            self.define_from_variant("WITH_ROCM_BACKEND", "rocm"),
            self.define("WITH_OPENCL_BACKEND", False),
            self.define_from_variant("WITH_LEVEL_ZERO_BACKEND", "level_zero"),
        ]
        if self.spec.satisfies("+cuda"):
            args.append(self.define("CUDA_TOOLKIT_ROOT_DIR", self.spec["cuda"].prefix))
        if self.spec.satisfies("+rocm"):
            args.extend(
                [
                    self.define("ROCM_PATH", self.spec["hip"].prefix),
                    self.define("ROCM_DEVICE_LIBS_PATH", self.prefix.amdgcn.bitcode),
                ]
            )
        return args

    @run_before("cmake", when="+rocm")
    def build_rocm_device_libs(self):
        llvm = self.spec["llvm"].prefix
        source = join_path(
            self.stage.source_path,
            "rocm-device-libs",
            "llvm-project-rocm-6.3.3",
            "amd",
            "device-libs",
        )
        build = join_path(self.stage.path, "rocm-device-libs-build")
        cmake = Executable(self.spec["cmake"].prefix.bin.cmake)
        cmake(
            "-S",
            source,
            "-B",
            build,
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_C_COMPILER={llvm.bin.clang}",
            f"-DCMAKE_CXX_COMPILER={llvm.bin.join('clang++')}",
            f"-DLLVM_DIR={llvm.lib.cmake.llvm}",
            f"-DClang_DIR={llvm.lib.cmake.clang}",
            f"-DCMAKE_INSTALL_PREFIX={self.prefix}",
        )
        cmake("--build", build, "--parallel", str(make_jobs))
        cmake("--install", build)

    @run_after("install", when="+rocm")
    def use_own_rocm_device_libs(self):
        filter_file(
            r"--rocm-device-lib-path=\$ACPP_ROCM_PATH/amdgcn/bitcode",
            f"--rocm-device-lib-path={self.prefix.amdgcn.bitcode}",
            join_path(self.prefix.etc.AdaptiveCpp, "acpp-rocm.json"),
        )
