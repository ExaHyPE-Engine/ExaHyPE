import argparse
import hpccm

parser = argparse.ArgumentParser(description='OpenMPI Recipe')
parser.add_argument('--format', type=str, default='docker',
                    choices=['docker', 'singularity'],
                    help='Container specification format (default: docker)')
args = parser.parse_args()

Stage0 = hpccm.Stage()
Stage0 += hpccm.primitives.baseimage(image='ubuntu:22.04')

# https://github.com/NVIDIA/hpc-container-maker/blob/master/hpccm/building_blocks/hpcx.py
Stage0 += hpccm.building_blocks.hpcx()

# https://github.com/NVIDIA/hpc-container-maker/blob/master/hpccm/building_blocks/mlnx_ofed.py
Stage0 += hpccm.building_blocks.mlnx_ofed()

# https://github.com/NVIDIA/hpc-container-maker/blob/master/hpccm/building_blocks/openmpi.py
Stage0 += hpccm.building_blocks.openmpi(cuda=True, infiniband=True)

print(Stage0)
