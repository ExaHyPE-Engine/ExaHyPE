import os
import argparse
import subprocess

# python3 .continuous_integration/configure.py --exahype --multithreading=tbb --gpu=sycl -t=intel --arch=cc86
# python3 .continuous_integration/configure.py --exahype --multithreading=omp --gpu=omp -t=nvidia --arch=cc86
# python3 .continuous_integration/configure.py --exahype --multithreading=omp --gpu=cpp -t=nvidia --arch=cc86
# python3 .continuous_integration/configure.py --exahype --multithreading=omp --gpu=omp -t=amd --arch=gfx90a
parser = argparse.ArgumentParser(description='Call GNU Autools ./configure script.')
parser.add_argument('-t', '--toolchain', choices=['gcc', 'llvm', 'nvidia', 'amd', 'intel'], help='Select toolchain')
parser.add_argument('-cc', type=str, default=os.environ.get('CC'), help='C compiler to use')
parser.add_argument('-cxx', type=str, default=os.environ.get('CXX'), help='C++ compiler to use')
parser.add_argument('-fc', type=str, default=os.environ.get('FC'), help='Fortran compiler to use')
parser.add_argument('--swift', action='store_true', help='Enable Swift')
parser.add_argument('--particles', action='store_true', help='Enable particles support')
parser.add_argument('--exahype', action='store_true', help='Enable ExaHyPE')
parser.add_argument('--loadbalancing', action='store_true', help='Enable load balancing support')
parser.add_argument('--blockstructured', action='store_true', help='Enable block structured grid support')
parser.add_argument('--fem', action='store_true', help='Enable finite elements support')
parser.add_argument('--libxsmm', action='store_true', help='Enable libxsmm support')
parser.add_argument('--multithreading', choices=['omp', 'tbb', 'sycl', 'cpp'], help='Enable multithreading support')
parser.add_argument('--gpu', choices=['omp', 'cuda', 'sycl', 'cpp', 'hip'], help='Enable GPU support')
parser.add_argument('--mpi', help='Enable MPI support')
parser.add_argument('--arch', choices=[
  'gfx906', 'gfx908', 'gfx90a', # gfx906=MI50, gfx908=MI100, gfx90a=MI200
  'cc70', 'cc72', 'cc75', 'cc80', 'cc86', 'cc90',
  'sm_70', 'sm_72', 'sm_75', 'sm_80', 'sm_86', 'sm_90',
  'spir64'],
  help='Select GPU compute capability')
args = parser.parse_args()

cxxflags = '-std=c++20 -W -Wall -Wextra'

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
  cxxflags += ' -cuda -mp=gpu -gpu=' + args.arch

if args.gpu == 'omp' and args.toolchain == "gcc":
  cxxflags += ' -fopenmp -foffload=nvptx-none -march=native'

if args.gpu == 'sycl' and args.toolchain == "intel":
  cxxflags += ' -fsycl -Xsycl-target-backend=nvptx64-nvidia-cuda --cuda-gpu-arch=' + args.arch

# Print out the selected compiler, compiler flags, and other options
print(f'Using C compiler: {args.cc}')
print(f'Using C++ compiler: {args.cxx} {cxxflags}')
print(f'Using Fortran compiler: {args.fc}')

# Call the ./configure script with the specified arguments
configure_args = []
configure_args.append(f'CC={args.cc}')
configure_args.append('CXX="{} {}"'.format(args.cxx, cxxflags))
configure_args.append(f'FC={args.fc}')

if args.multithreading is not None:
  configure_args.append(f'--with-multithreading={args.multithreading}')

if args.gpu is not None:
  configure_args.append(f'--with-gpu={args.gpu}')

if args.mpi is not None:
  configure_args.append('--with-mpi="{} {}"'.format(args.mpi, cxxflags))

if args.exahype:
  configure_args.append('--enable-exahype')
  configure_args.append('--enable-loadbalancing')
  configure_args.append('--enable-blockstructured')
  configure_args.append('--with-libxsmm')
if args.loadbalancing:
  configure_args.append('--enable-loadbalancing')
if args.blockstructured:
  configure_args.append('--enable-blockstructured')
if args.swift:
  configure_args.append('--enable-swift')
  configure_args.append('--enable-particles')
if args.particles:
  configure_args.append('--enable-particles')
if args.fem:
  configure_args.append('--enable-finiteelements')
if args.libxsmm:
  configure_args.append('--with-libxsmm')

print('Running ./configure ' + ' '.join(configure_args))

subprocess.run(['libtoolize'], cwd=os.getcwd())
subprocess.run(['aclocal'], cwd=os.getcwd())
subprocess.run(['autoconf'], cwd=os.getcwd())
subprocess.run(['autoheader'], cwd=os.getcwd())
subprocess.run(['cp', f'{os.getcwd()}/src/config.h.in', os.getcwd()])
subprocess.run(['automake', '--add-missing'], cwd=os.getcwd())

import shlex
args_str = ' '.join(configure_args)
args_list = shlex.split(args_str)
subprocess.check_call(['./configure'] + args_list, cwd=os.getcwd())
