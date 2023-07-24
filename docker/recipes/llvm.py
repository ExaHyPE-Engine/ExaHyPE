import argparse
import hpccm

parser = argparse.ArgumentParser(description='LLVM Recipe')
parser.add_argument('--format', type=str, default='docker',
                    choices=['docker', 'singularity'],
                    help='Container specification format (default: docker)')
args = parser.parse_args()

Stage0 = hpccm.Stage()
Stage0 += hpccm.primitives.baseimage(image='ubuntu:22.04')

# https://github.com/NVIDIA/hpc-container-maker/blob/master/hpccm/building_blocks/llvm.py
Stage0 += hpccm.building_blocks.llvm(extra_tools=True, openmp=True, toolset=True)

print(Stage0)
