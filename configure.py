# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import os
import re
import shlex
import shutil
import argparse
import subprocess


def is_pkg_config_installed():
    return shutil.which("pkg-config") is not None


parser = argparse.ArgumentParser(
    description="Configuration helper script for GNU Automake"
)
parser.add_argument("--exahype", action="store_true", help="Enable ExaHyPE 2")
parser.add_argument("--swift", action="store_true", help="Enable Swift 2")
parser.add_argument("--particles", action="store_true", help="Enable particles support")
parser.add_argument(
    "--loadbalancing", action="store_true", help="Enable load balancing support"
)
parser.add_argument(
    "--blockstructured", action="store_true", help="Enable block structured support"
)
parser.add_argument("--fem", action="store_true", help="Enable finite elements support")
parser.add_argument("--petsc", action="store_true", help="Enable PETSc support")
parser.add_argument("--netcdf", action="store_true", help="Enable netCDF support")
parser.add_argument("--libxsmm", action="store_true", help="Enable LIBXSMM support")
parser.add_argument(
    "--multithreading",
    choices=["no", "omp", "sycl", "tbb", "cpp"],
    help="Enable multithreading support",
)
parser.add_argument("--mpi", help="Enable MPI support")
parser.add_argument(
    "--targetdart", "-td", action="store_true", help="Enable targetDART support"
)
parser.add_argument(
    "--scorep", "-scorep", action="store_true", help="Enable Score-P support"
)
parser.add_argument(
    "--gpu", choices=["no", "omp", "sycl", "cpp", "hip", "cuda"], help="Enable GPU support"
)
parser.add_argument("--asm", action="store_true", help="Dump generated assembly code")
parser.add_argument("--with-cuda", type=str, help="Path to the CUDA toolkit")
parser.add_argument(
    "--arch",
    choices=[
        "gfx906",  # gfx906=MI50
        "gfx908",  # gfx908=MI100
        "gfx90a",  # gfx90a=MI200
        "gfx942",  # gfx942=MI300
        "sm_70",
        "sm_72",
        "sm_75",
        "sm_80",
        "sm_86",
        "sm_89",
        "sm_90",
        "spir64",
    ],
    help="Select GPU compute capability",
)
args = parser.parse_args()

cc = os.environ.get("CC")
cxx = os.environ.get("CXX")
if cc is None or cxx is None:
    raise Exception(
        "Invalid C and C++ compilers provided. \
Specify compatible C and C++ compilers by using the environment flags CC and CXX."
    )

cflags = "-O3 -march=native -mtune=native -fomit-frame-pointer"
cxxflags = "-std=c++20 -w -O3 -march=native -mtune=native -fomit-frame-pointer"
nvccflags = "-O3 -march=native -mtune=native -fomit-frame-pointer"
ldflags = "-march=native -mtune=native"
configure_args = []

if args.multithreading == "omp":
    cflags += " -fopenmp"
    cxxflags += " -fopenmp"
    ldflags += " -fopenmp"

    # Intel suggests to use -fiopenmp for performance and features:
    # https://www.intel.com/content/www/us/en/developer/articles/guide/porting-guide-for-icc-users-to-dpcpp-or-icx.html
    if "icx" in cc:
        cflags = re.sub("-fopenmp", "-fiopenmp", cxxflags)
        ldflags = re.sub("-fopenmp", "-fiopenmp", cxxflags)
    if "icpx" in cxx:
        cxxflags = re.sub("-fopenmp", "-fiopenmp", cxxflags)
        ldflags = re.sub("-fopenmp", "-fiopenmp", cxxflags)

if args.multithreading == "tbb":
    cflags += " -ltbb"
    cxxflags += " -ltbb"
    ldflags += " -ltbb"

if args.multithreading == "sycl":
    cflags += " -fsycl"
    cxxflags += " -fsycl"
    ldflags += " -fsycl"

# Merge in any additional flags needed for the selected GPU backend
if args.gpu is not None and args.gpu != "no" and not args.arch:
    raise Exception("You need to specify a GPU architecture when using GPU support.")

if args.gpu == "omp" and "icx" in cc and "icpx" in cxx and "spir64" in args.arch:
    cflags += " -fopenmp-targets=" + args.arch
    cxxflags += " -fopenmp-targets=" + args.arch
    ldflags += " -fopenmp-targets=" + args.arch

elif args.gpu == "omp" and "amdclang" in cc and "amdclang++" in cxx:
    cflags += " -lstdc++fs --offload-arch=" + args.arch + " -D__AMDGPU__"
    cxxflags += " -lstdc++fs --offload-arch=" + args.arch + " -D__AMDGPU__"
    cxxflags = re.sub(r"-std=c\+\+20", "-std=c++17", cxxflags)
    ldflags += " -lstdc++fs --offload-arch=" + args.arch
    if args.asm:
        cflags += " -save-temps"
        cxxflags += " -save-temps"

elif (
    args.gpu == "omp"
    and ("clang" in cc or "clangwrap" in cc or "tdclang" in cc)
    and (
        "clang++" in cxx
        or "clangwrap" in cxx
        or "clangwrap++" in cxx
        or "tdclang++" in cxx
    )
):
    cflags += (
        " -fopenmp-targets=nvptx64-nvidia-cuda -Xopenmp-target=nvptx64-nvidia-cuda -march="
        + args.arch
    )
    cxxflags += (
        " -fopenmp-targets=nvptx64-nvidia-cuda -Xopenmp-target=nvptx64-nvidia-cuda -march="
        + args.arch
    )
    cxxflags = re.sub(r"-std=c\+\+20", "-std=c++17", cxxflags)
    ldflags += (
        " -fopenmp-targets=nvptx64-nvidia-cuda -Xopenmp-target=nvptx64-nvidia-cuda -march="
        + args.arch
    )
    if args.asm:
        cflags += " -save-temps -Xcuda-ptxas -v"
        cxxflags += " -save-temps -Xcuda-ptxas -v"

elif args.gpu == "omp" and "amdclang" in cc and "amdclang++" in cxx:
    cflags += " -lstdc++fs --offload-arch=" + args.arch
    cxxflags += " -lstdc++fs --offload-arch=" + args.arch
    ldflags += " -lstdc++fs --offload-arch=" + args.arch

elif args.gpu == "omp" and ("nvc" in cc or "nvc++ in cc") and "nvc++" in cxx:
    cflags += " -mp=gpu -gpu=cc" + args.arch.split("_")[1]  # -cuda
    cxxflags += " -mp=gpu -gpu=cc" + args.arch.split("_")[1]  # -cuda
    ldflags += " -mp=gpu -gpu=cc" + args.arch.split("_")[1]  # -cuda
    if args.asm:
        cflags += " -gpu=keep,zeroinit,ptxinfo,all,lineinfo -Minfo=mp"
        cxxflags += " -gpu=keep,zeroinit,ptxinfo,all,lineinfo -Minfo=mpp"

elif args.gpu == "cpp" and ("nvc" in cc or "nvc++ in cc") and "nvc++" in cxx:
    cflags += " -cuda -mp=gpu -gpu=cc" + args.arch.split("_")[1]
    cxxflags += " -cuda -mp=gpu -stdpar=gpu -gpu=cc" + args.arch.split("_")[1]
    ldflags += " -cuda -mp=gpu -gpu=cc" + args.arch.split("_")[1]
    if args.asm:
        cflags += " -gpu=keep,zeroinit,ptxinfo,all,lineinfo -Minfo=mp"
        cxxflags += " -gpu=keep,zeroinit,ptxinfo,all,lineinfo -Minfo=mp"

elif args.gpu == "omp" and "gcc" in cc and "g++" in cxx:
    cflags += " -fopenmp -foffload=nvptx-none -march=native"
    cxxflags += " -fopenmp -foffload=nvptx-none -march=native"
    ldflags += " -fopenmp -foffload=nvptx-none -march=native"
    if args.asm:
        cflags += " -save-temps"
        cxxflags += " -save-temps"

elif args.gpu == "sycl" and "icx" in cc and "icpx" in cxx:
    if "gfx" in args.arch:
        cflags += (
            " -fsycl -fsycl-targets=amdgcn-amd-amdsha -fsycl-device-code-split -fp-model=consistent -Xsycl-target-backend=amdgcn-amd-amdsha --offload-arch="
            + args.arch
        )
        cxxflags += (
            " -fsycl -fsycl-targets=amdgcn-amd-amdsha -fsycl-device-code-split -fp-model=consistent -Xsycl-target-backend=amdgcn-amd-amdsha --offload-arch="
            + args.arch
        )
        ldflags += (
            " -fsycl -fsycl-targets=amdgcn-amd-amdsha -fsycl-device-code-split -fp-model=consistent -Xsycl-target-backend=amdgcn-amd-amdsha --offload-arch="
            + args.arch
        )
    if "spir" in args.arch:
        cflags += " -fsycl -fsycl-targets=spir64 -fsycl-device-code-split -fp-model=consistent"
        cxxflags += " -fsycl -fsycl-targets=spir64 -fsycl-device-code-split -fp-model=consistent"
        ldflags += " -fsycl -fsycl-targets=spir64 -fsycl-device-code-split -fp-model=consistent"
    else:
        cflags += (
            " -fsycl -fsycl-targets=nvptx64-nvidia-cuda -fsycl-device-code-split -fp-model=consistent -Xsycl-target-backend=nvptx64-nvidia-cuda --cuda-gpu-arch="
            + args.arch
        )
        cxxflags += (
            " -fsycl -fsycl-targets=nvptx64-nvidia-cuda -fsycl-device-code-split -fp-model=consistent -Xsycl-target-backend=nvptx64-nvidia-cuda --cuda-gpu-arch="
            + args.arch
        )
        ldflags += (
            " -fsycl -fsycl-targets=nvptx64-nvidia-cuda -fsycl-device-code-split -fp-model=consistent -Xsycl-target-backend=nvptx64-nvidia-cuda --cuda-gpu-arch="
            + args.arch
        )

elif args.gpu == "cuda":
    nvccflags += f" -arch={args.arch}"
    if args.with_cuda is not None:
        configure_args.append(f"--with-cuda={args.with_cuda}")
    else:
        raise Exception(
            "You need to specify the path to the CUDA toolkit when compiling with CUDA. Use --with-cuda=<CUDA_PATH>."
        )

else:
    if args.gpu is not None and args.gpu != "no":
        raise Exception(
            "Unsupported compiler and GPU flag/arch combination requested: CC="
            + cc
            + " CXX="
            + cxx
            + " --gpu="
            + args.gpu
            + " --arch="
            + args.arch
        )

if args.multithreading is not None:
    configure_args.append(f"--with-multithreading={args.multithreading}")

if args.gpu is not None:
    configure_args.append(f"--with-gpu={args.gpu}")

if args.mpi is not None:
    if args.scorep:
        configure_args.append('--with-mpi="scorep-{}"'.format(args.mpi))
    else:
        configure_args.append('--with-mpi="{}"'.format(args.mpi))

if args.targetdart:
    configure_args.append("--with-targetdart")

if args.exahype:
    configure_args.append("--enable-exahype")
    configure_args.append("--enable-loadbalancing")
    configure_args.append("--enable-blockstructured")

if args.loadbalancing:
    configure_args.append("--enable-loadbalancing")

if args.blockstructured:
    configure_args.append("--enable-blockstructured")

if args.swift:
    configure_args.append("--enable-swift")
    configure_args.append("--enable-particles")
    configure_args.append("--enable-loadbalancing")

if args.particles:
    configure_args.append("--enable-particles")

if args.fem:
    configure_args.append("--enable-finiteelements")

if args.petsc:
    configure_args.append("--with-petsc")
    configure_args.append("--enable-finiteelements")


def find_netcdf_paths():
    if not is_pkg_config_installed():
        return None, None

    try:
        # Use pkg-config to get the NetCDF include directory
        include_dir = (
            subprocess.check_output(["pkg-config", "--cflags-only-I", "netcdf"])
            .decode()
            .strip()[2:]
        )
        # Use pkg-config to get the NetCDF library directory and name
        lib_dir_and_name = (
            subprocess.check_output(
                ["pkg-config", "--libs-only-L", "--libs-only-l", "netcdf"]
            )
            .decode()
            .strip()[2:]
        )
        return include_dir, lib_dir_and_name
    except subprocess.CalledProcessError:
        return None, None


if args.netcdf:
    netcdf_include_dir, netcdf_lib_dir_and_name = find_netcdf_paths()
    if netcdf_include_dir is not None:
        cxxflags += f" -I{netcdf_include_dir}"
    if netcdf_lib_dir_and_name is not None:
        ldflags += f" -L{netcdf_lib_dir_and_name}"
    configure_args.append("--with-netcdf")

if args.libxsmm:
    configure_args.append("--with-libxsmm")

# Prepare the environment variables
# Read existing C(XX)FLAGS and LDFLAGS to not overwrite them
env_cflags = os.environ.get("CFLAGS")
env_cxxflags = os.environ.get("CXXFLAGS")
env_ldflags = os.environ.get("LDFLAGS")

if env_cflags is not None:
    env_cflags += " " + cflags
else:
    env_cflags = cflags

if env_cxxflags is not None:
    env_cxxflags += " " + cxxflags
else:
    env_cxxflags = cxxflags

if env_ldflags is not None:
    env_ldflags += " " + ldflags
else:
    env_ldflags = ldflags

if args.scorep:
    env_vars = {
        "CFLAGS": "{}".format(env_cflags),
        "CC": "scorep-{}".format(cc),
        "CXXFLAGS": "{}".format(env_cxxflags),
        "CXX": "scorep-{}".format(cxx),
        "LDFLAGS": "{}".format(env_ldflags),
    }
else:
    env_vars = {
        "CFLAGS": "{}".format(env_cflags),
        "CC": "{}".format(cc),
        "CXXFLAGS": "{}".format(env_cxxflags),
        "CXX": "{}".format(cxx),
        "LDFLAGS": "{}".format(env_ldflags),
    }

if args.gpu == "cuda":
    env_vars["NVCC"] = "{}/bin/nvcc {}".format(args.with_cuda, nvccflags)

# Create a string to display environment variables like in a terminal
if args.scorep:
    env_vars_str = (
        'SCOREP_WRAPPER=OFF SCOREP_WRAPPER_INSTRUMENTER_FLAGS="--user --nocompiler --thread=omp:ompt" '
        + " ".join(f'{key}="{value}"' for key, value in env_vars.items())
    )
else:
    env_vars_str = " ".join(f'{key}="{value}"' for key, value in env_vars.items())

print(f"{env_vars_str} ./configure {' '.join(configure_args)}")

# Update the current environment with the specified variables
new_env = os.environ.copy()
new_env.update(env_vars)

subprocess.run(["libtoolize"], cwd=os.getcwd())
subprocess.run(["aclocal"], cwd=os.getcwd())
subprocess.run(["autoconf"], cwd=os.getcwd())
subprocess.run(["autoheader"], cwd=os.getcwd())
subprocess.run(["cp", f"{os.getcwd()}/src/config.h.in", os.getcwd()])
subprocess.run(["automake", "--add-missing"], cwd=os.getcwd())

# Call the ./configure script with the specified arguments
args_str = " ".join(configure_args)
args_list = shlex.split(args_str)

subprocess.check_call(["./configure"] + args_list, cwd=os.getcwd(), env=new_env)
