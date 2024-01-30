# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import os
import re
import shlex
import argparse
import subprocess

parser = argparse.ArgumentParser(
    description="Configuration helper script for GNU Autotools and CMake"
)
parser.add_argument(
    "-cc", "--cc", type=str, default=os.environ.get("CC"), help="C compiler to use"
)
parser.add_argument(
    "-cxx", "--cxx", type=str, default=os.environ.get("CXX"), help="C++ compiler to use"
)
parser.add_argument(
    "--cmake", help="Use CMake; specify target build folder via --cmake=..."
)
parser.add_argument("--exahype", action="store_true", help="Enable ExaHyPE2")
parser.add_argument("--swift", action="store_true", help="Enable Swift2")
parser.add_argument("--particles", action="store_true", help="Enable particles support")
parser.add_argument(
    "--loadbalancing", action="store_true", help="Enable load balancing support"
)
parser.add_argument(
    "--blockstructured", action="store_true", help="Enable block structured support"
)
parser.add_argument("--fem", action="store_true", help="Enable finite elements support")
parser.add_argument("--petsc", action="store_true", help="Enable PETSc support")
parser.add_argument("--libxsmm", action="store_true", help="Enable LIBXSMM support")
parser.add_argument(
    "--multithreading",
    choices=["omp", "tbb", "sycl", "cpp"],
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
    "--gpu", choices=["omp", "cuda", "sycl", "cpp", "hip"], help="Enable GPU support"
)
parser.add_argument("--asm", action="store_true", help="Dump generated assembly code")
parser.add_argument("--with-cuda", type=str, help="Path to the CUDA toolkit")
parser.add_argument(
    "--arch",
    choices=[
        "gfx906",
        "gfx908",
        "gfx90a",  # gfx906=MI50, gfx908=MI100, gfx90a=MI200
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

cflags = "-O3 -march=native"
cxxflags = "-std=c++20 -w -O3 -march=native"
# cxxflags = '-std=c++20 -W -Wall -Wextra -O3 -march=native -mtune=native'
nvccflags = "-O3"  # -march=native -mtune=native
ldflags = "-march=native"
configure_args = []

# Merge in any additional flags needed for the selected multithreading backend
if args.multithreading == "omp":
    cflags += " -fopenmp"
    cxxflags += " -fopenmp"
    ldflags += " -fopenmp"

    # Intel suggests to use -fiopenmp for performance and features:
    # https://www.intel.com/content/www/us/en/developer/articles/guide/porting-guide-for-icc-users-to-dpcpp-or-icx.html
    if "icx" in args.cc:
        cflags = re.sub("-fopenmp", "-fiopenmp", cxxflags)
        ldflags = re.sub("-fopenmp", "-fiopenmp", cxxflags)
    if "icpx" in args.cxx:
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
if args.gpu and not args.arch:
    raise Exception("You need to specify a GPU architecture when using GPU support.")

if args.cc is None or args.cxx is None:
    raise Exception(
        "Invalid C and C++ compilers provided. \
Specify compatible C and C++ compilers by using --cc and --cxx or the environment flags CC and CXX."
    )

if args.gpu == "omp" and "icx" in args.cc and "icpx" in args.cxx and "spir64" in args.arch:
    cflags += (
        " -fopenmp-targets="
        + args.arch
    )
    cxxflags += (
        " -fopenmp-targets="
        + args.arch
    )
    ldflags += (
        " -fopenmp-targets="
        + args.arch
    )

elif args.gpu == "omp" and "amdclang" in args.cc and "amdclang++" in args.cxx:
    cflags += " -lstdc++fs --offload-arch=" + args.arch
    cxxflags += " -lstdc++fs --offload-arch=" + args.arch
    cxxflags = re.sub(r"-std=c\+\+20", "-std=c++17", cxxflags)
    ldflags += " -lstdc++fs --offload-arch=" + args.arch

elif (
    args.gpu == "omp"
    and ("clang" in args.cc or "clangwrap" in args.cc or "tdclang" in args.cc)
    and (
        "clang++" in args.cxx
        or "clangwrap" in args.cxx
        or "clangwrap++" in args.cxx
        or "tdclang++" in args.cxx
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

elif (
    args.gpu == "omp"
    and ("nvc" in args.cc or "nvc++ in args.cc")
    and "nvc++" in args.cxx
):
    cflags += " -cuda -mp=gpu -gpu=cc" + args.arch
    cxxflags += " -cuda -mp=gpu -gpu=cc" + args.arch
    ldflags += " -cuda -mp=gpu -gpu=cc" + args.arch
    if args.asm:
        cflags += " -gpu=keep,zeroinit,ptxinfo,all,lineinfo -Minfo=mp"
        cxxflags += " -gpu=keep,zeroinit,ptxinfo,all,lineinfo -Minfo=mpp"

elif (
    args.gpu == "cpp"
    and ("nvc" in args.cc or "nvc++ in args.cc")
    and "nvc++" in args.cxx
):
    cflags += " -cuda -mp=gpu -gpu=cc" + args.arch
    cxxflags += " -cuda -mp=gpu -gpu=cc" + args.arch + " -stdpar=gpu"
    ldflags += " -cuda -mp=gpu -gpu=cc" + args.arch
    if args.asm:
        cflags += " -gpu=keep,zeroinit,ptxinfo,all,lineinfo -Minfo=mp"
        cxxflags += " -gpu=keep,zeroinit,ptxinfo,all,lineinfo -Minfo=mp"

elif args.gpu == "omp" and "gcc" in args.cc and "g++" in args.cxx:
    cflags += " -fopenmp -foffload=nvptx-none -march=native"
    cxxflags += " -fopenmp -foffload=nvptx-none -march=native"
    ldflags += " -fopenmp -foffload=nvptx-none -march=native"
    if args.asm:
        cflags += " -save-temps"
        cxxflags += " -save-temps"

elif args.gpu == "sycl" and "icx" in args.cc and "icpx" in args.cxx:
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
        if not args.cmake:
            # CUDA path is only needed with Automake builds
            # CMake automatically finds it
            configure_args.append(f"--with-cuda={args.with_cuda}")
    else:
        raise Exception(
            "You need to specify the path to the CUDA toolkit when compiling with CUDA. Use --with-cuda=<CUDA_PATH>."
        )

else:
    if args.gpu is not None:
        raise Exception(
            "Unsupported compiler and GPU flag/arch combination requested: CC="
            + args.cc
            + " CXX="
            + args.cxx
            + " --gpu="
            + args.gpu
            + " --arch="
            + args.arch
        )

if args.multithreading is not None:
    if args.cmake:
        configure_args.append(f"-DWITH_MULTITHREADING={args.multithreading}")
    else:
        configure_args.append(f"--with-multithreading={args.multithreading}")

if args.gpu is not None:
    if args.cmake:
        configure_args.append(f"-DWITH_GPU={args.gpu}")
        configure_args.append(f"-DWITH_GPU_ARCH={args.arch}")
    else:
        configure_args.append(f"--with-gpu={args.gpu}")

if args.mpi is not None:
    if args.cmake:
        configure_args.append("-DWITH_MPI=ON")
    else:
        if args.scorep:
            configure_args.append('--with-mpi="scorep-{}"'.format(args.mpi))
        else:
            configure_args.append('--with-mpi="{}"'.format(args.mpi))

if args.targetdart:
    if args.cmake:
        configure_args.append("-DWITH_TARGETDART=ON")
    else:
        configure_args.append("--with-targetdart")

if args.exahype:
    if args.cmake:
        configure_args.append("-DENABLE_EXAHYPE=ON")
        configure_args.append("-DENABLE_LOADBALANCING=ON")
        configure_args.append("-DENABLE_BLOCKSTRUCTURED=ON")
    else:
        configure_args.append("--enable-exahype")
        configure_args.append("--enable-loadbalancing")
        configure_args.append("--enable-blockstructured")

if args.loadbalancing:
    if args.cmake:
        configure_args.append("-DENABLE_LOADBALANCING=ON")
    else:
        configure_args.append("--enable-loadbalancing")

if args.blockstructured:
    if args.cmake:
        configure_args.append("-DENABLE_BLOCKSTRUCTURED=ON")
    else:
        configure_args.append("--enable-blockstructured")

if args.swift:
    if args.cmake:
        configure_args.append("-DENABLE_SWIFT=ON")
        configure_args.append("-DENABLE_PARTICLES=ON")
    else:
        configure_args.append("--enable-swift")
        configure_args.append("--enable-particles")

if args.particles:
    if args.cmake:
        configure_args.append("-DENABLE_PARTICLES=ON")
    else:
        configure_args.append("--enable-particles")

if args.fem:
    if args.cmake:
        configure_args.append("-DENABLE_FINITEELEMENTS=ON")
    else:
        configure_args.append("--enable-finiteelements")

if args.petsc:
    if args.cmake:
        configure_args.append("-DWITH_PETSC=ON")
        configure_args.append("-DENABLE_FINITEELEMENTS=ON")
    else:
        configure_args.append("--with-petsc")
        configure_args.append("--enable-finiteelements")

if args.libxsmm:
    if args.cmake:
        configure_args.append("-WITH_LIBXSMM=ON")
    else:
        configure_args.append("--with-libxsmm")

if args.cmake:
    # Prepare the environment variables. We don't need any compiler flags here as CMake handles those.
    if args.scorep:
        env_vars = {
            "CC": "scorep-{}".format(args.cc),
            "CXX": "scorep-{}".format(args.cxx),
        }
    else:
        env_vars = {
            "CC": "{}".format(args.cc),
            "CXX": "{}".format(args.cxx),
        }

    # Create a string to display environment variables like in a terminal
    if args.scorep:
        env_vars_str = "SCOREP_WRAPPER=OFF SCOREP_WRAPPER_INSTRUMENTER_FLAGS=\"--user --nocompiler --thread=omp:ompt\" " + " ".join(f'{key}="{value}"' for key, value in env_vars.items())
    else:
        env_vars_str = " ".join(f'{key}="{value}"' for key, value in env_vars.items())

    print(f"{env_vars_str} cmake .. {' '.join(configure_args)}")

    # Update the current environment with the specified variables
    new_env = os.environ.copy()
    new_env.update(env_vars)

    if not os.path.exists(args.cmake) and not os.path.isdir(args.cmake):
        subprocess.run("mkdir {}".format(args.cmake), cwd=os.getcwd(), shell=True)

    os.chdir(args.cmake)
    args_str = " ".join(configure_args)
    args_list = shlex.split(args_str)
    subprocess.check_call(["cmake", ".."] + args_list, cwd=os.getcwd(), env=new_env)

else:
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
            "CC": "scorep-{}".format(args.cc),
            "CXXFLAGS": "{}".format(env_cxxflags),
            "CXX": "scorep-{}".format(args.cxx),
            "LDFLAGS": "{}".format(env_ldflags),
        }
    else:
        env_vars = {
            "CFLAGS": "{}".format(env_cflags),
            "CC": "{}".format(args.cc),
            "CXXFLAGS": "{}".format(env_cxxflags),
            "CXX": "{}".format(args.cxx),
            "LDFLAGS": "{}".format(env_ldflags),
        }

    if args.gpu == "cuda":
        env_vars["NVCC"] = "{}/bin/nvcc {}".format(args.with_cuda, nvccflags)

    # Create a string to display environment variables like in a terminal
    if args.scorep:
        env_vars_str = "SCOREP_WRAPPER=OFF SCOREP_WRAPPER_INSTRUMENTER_FLAGS=\"--user --nocompiler --thread=omp:ompt\" " + " ".join(f'{key}="{value}"' for key, value in env_vars.items())
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
