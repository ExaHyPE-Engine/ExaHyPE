import os
import shlex
import argparse
import subprocess

parser = argparse.ArgumentParser(description='Configure helper script for GNU autotools and CMake')
parser.add_argument('--cmake', help='Use CMake')
parser.add_argument('-t', '--toolchain', choices=['gcc', 'llvm', 'nvidia', 'amd', 'intel'], help='Select toolchain')
parser.add_argument('--cc', type=str, default=os.environ.get('CC'), help='C compiler to use')
parser.add_argument('--cxx', type=str, default=os.environ.get('CXX'), help='C++ compiler to use')
parser.add_argument('--fc', type=str, default=os.environ.get('FC'), help='Fortran compiler to use')
parser.add_argument('--swift', action='store_true', help='Enable Swift')
parser.add_argument('--particles', action='store_true', help='Enable particles support')
parser.add_argument('--exahype', action='store_true', help='Enable ExaHyPE')
parser.add_argument('--loadbalancing', action='store_true', help='Enable load balancing support')
parser.add_argument('--blockstructured', action='store_true', help='Enable block structured grid support')
parser.add_argument('--fem', action='store_true', help='Enable finite elements support')
parser.add_argument('--petsc', action='store_true', help='Enable PETSc support')
parser.add_argument('--libxsmm', action='store_true', help='Enable libxsmm support')
parser.add_argument('--multithreading', choices=['omp', 'tbb', 'sycl', 'cpp'], help='Enable multithreading support')
parser.add_argument('--mpi', help='Enable MPI support')
parser.add_argument('--gpu', choices=['omp', 'cuda', 'sycl', 'cpp', 'hip'], help='Enable GPU support')
parser.add_argument('--cuda-path', type=str, help='Path to CUDA')
parser.add_argument('--arch', choices=[
  'gfx906', 'gfx908', 'gfx90a', # gfx906=MI50, gfx908=MI100, gfx90a=MI200
  'sm_70', 'sm_72', 'sm_75', 'sm_80', 'sm_86', 'sm_90',
  'spir64'],
  help='Select GPU compute capability')
args = parser.parse_args()

cflags = ''
#cxxflags = '-std=c++20 -W -Wall -Wextra'
cxxflags = '-std=c++20 -w'
fortranflags = ''

# Merge in any additional flags needed for the selected multithreading backend
if args.multithreading == 'omp':
  cxxflags += ' -fopenmp'

if args.multithreading == 'tbb':
  cxxflags += ' -ltbb'

if args.multithreading == 'sycl':
  cxxflags += ' -fsycl'

# Merge in any additional flags needed for the selected GPU backend
if args.gpu and not args.toolchain:
  print("Error: You need to specify a toolchain when using GPU support")
  exit(-1)

if args.gpu and not args.arch:
  print("Error: You need to specify a GPU architecture when using GPU support")
  exit(-1)

if args.gpu == 'omp' and args.toolchain == "clang":
  cxxflags += ' -fopenmp-targets=nvptx64-nvidia-cuda -Xopenmp-target -march=' + args.arch

if args.gpu == 'omp' and args.toolchain == "amd":
  cxxflags += ' -lstdc++fs --offload-arch=' + args.arch

if args.gpu == 'omp' or args.gpu == 'cpp' and args.toolchain == "nvidia":
  cxxflags += ' -cuda -mp=gpu -gpu=' + args.arch.replace("sm_", "cc")

if args.gpu == 'omp' and args.toolchain == "gcc":
  cxxflags += ' -fopenmp -foffload=nvptx-none -march=native'

if args.gpu == 'sycl' and args.toolchain == "intel":
  cxxflags += ' -fsycl -Xsycl-target-backend=nvptx64-nvidia-cuda --cuda-gpu-arch=' + args.arch

# Call the ./configure script with the specified arguments
configure_args = []
if args.multithreading is not None:
  if args.cmake:
    configure_args.append(f'-DWITH_MULTITHREADING={args.multithreading}')
  else:
    configure_args.append(f'--with-multithreading={args.multithreading}')

if args.gpu is not None:
  if args.cmake:
    configure_args.append(f'-DWITH_GPU={args.gpu}')
    configure_args.append(f'-DWITH_GPU_ARCH={args.arch}')
  else:
    configure_args.append(f'--with-gpu={args.gpu}')

if args.gpu == "cuda":
  if args.cuda_path:
    configure_args.append(f'--with-cuda={args.cuda_path}')

  # Would be better if architecture flag was given as a C++ flag, but it will be used by
  # the nvcc compiler (e.g. -arch={args.arch}) but this approach is problematic as the 
  # flag will be then used by the C++ compiler (nvc++, g++ etc.) and will result with compiler
  # errors.
  if args.arch:
    configure_args.append(f'--with-cuda-gpu-arch={args.arch}')

if args.mpi is not None:
  if args.cmake:
    configure_args.append('-DWITH_MPI=ON')
  else:
    configure_args.append('--with-mpi="{} {}"'.format(args.mpi, cxxflags))

if args.exahype:
  if args.cmake:
    configure_args.append('-DENABLE_EXAHYPE=ON')
    configure_args.append('-DENABLE_LOADBALANCING=ON')
    configure_args.append('-DENABLE_BLOCKSTRUCTURED=ON')
    configure_args.append('-DWITH_LIBXSMM=ON')
  else:
    configure_args.append('--enable-exahype')
    configure_args.append('--enable-loadbalancing')
    configure_args.append('--enable-blockstructured')
    configure_args.append('--with-libxsmm')

if args.loadbalancing:
  if args.cmake:
    configure_args.append('-DENABLE_LOADBALANCING=ON')
  else:
    configure_args.append('--enable-loadbalancing')

if args.blockstructured:
  if args.cmake:
    configure_args.append('-DENABLE_BLOCKSTRUCTURED=ON')
  else:
    configure_args.append('--enable-blockstructured')

if args.swift:
  if args.cmake:
    configure_args.append('-DENABLE_SWIFT=ON')
    configure_args.append('-DENABLE_PARTICLES=ON')
  else:
    configure_args.append('--enable-swift')
    configure_args.append('--enable-particles')

if args.particles:
  if args.cmake:
    configure_args.append('-DENABLE_PARTICLES=ON')
  else:
    configure_args.append('--enable-particles')

if args.fem:
  if args.cmake:
    configure_args.append('-DENABLE_FINITEELEMENTS=ON')
  else:
    configure_args.append('--enable-finiteelements')

if args.petsc:
  if args.cmake:
    configure_args.append('-DWITH_PETSC=ON')
    configure_args.append('-DENABLE_FINITEELEMENTS=ON')
  else:
    configure_args.append('--with-petsc')
    configure_args.append('--enable-finiteelements')

if args.libxsmm:
  if args.cmake:
    configure_args.append('-WITH_LIBXSMM=ON')
  else:
    configure_args.append('--with-libxsmm')

if args.cmake:
  # Prepare the environment variables. We don't need any compiler flags here as CMake handles those.
  env_vars = {
      "C": "{}".format(args.cc),
      "CXX": "{}".format(args.cxx),
      "FC": "{}".format(args.fc),
  }

  # Create a string to display environment variables like in a terminal
  env_vars_str = ' '.join(f'{key}="{value}"' for key, value in env_vars.items())
  print(f"Running: {env_vars_str} cmake .. {' '.join(configure_args)}")

  # Update the current environment with the specified variables
  new_env = os.environ.copy()
  new_env.update(env_vars)

  if not os.path.exists(args.cmake) and not os.path.isdir(args.cmake):
    subprocess.run('mkdir {}'.format(args.cmake), cwd=os.getcwd(), shell=True)

  os.chdir(args.cmake)
  args_str = ' '.join(configure_args)
  args_list = shlex.split(args_str)
  subprocess.check_call(['cmake', '..'] + args_list, cwd=os.getcwd(), env=new_env)

else:
  # Prepare the environment variables
  env_vars = {
      "C": "{} {}".format(args.cc, cflags),
      "CXX": "{} {}".format(args.cxx, cxxflags),
      "FC": "{} {}".format(args.fc, fortranflags),
  }

  # Create a string to display environment variables like in a terminal
  env_vars_str = ' '.join(f'{key}="{value}"' for key, value in env_vars.items())
  print(f"Running: {env_vars_str} ./configure {' '.join(configure_args)}")

  # Update the current environment with the specified variables
  new_env = os.environ.copy()
  new_env.update(env_vars)

  subprocess.run(['libtoolize'], cwd=os.getcwd())
  subprocess.run(['aclocal'], cwd=os.getcwd())
  subprocess.run(['autoconf'], cwd=os.getcwd())
  subprocess.run(['autoheader'], cwd=os.getcwd())
  subprocess.run(['cp', f'{os.getcwd()}/src/config.h.in', os.getcwd()])
  subprocess.run(['automake', '--add-missing'], cwd=os.getcwd())

  args_str = ' '.join(configure_args)
  args_list = shlex.split(args_str)

  subprocess.check_call(['./configure'] + args_list, cwd=os.getcwd(), env=new_env)
