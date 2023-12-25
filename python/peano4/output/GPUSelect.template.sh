#!/bin/bash

unset node_local_mpi_rank

# Different MPI implementations use different environment variables to set
# The node local ranks, ExaHyPE2 assumes 1 MPI rank per 1 GPU within a node
# And the node local ranks are used to determine the GPU Id. 
# This variable is set to the detected mpicxx used in the configuration,
# Depending on your needs update the varaible.
# OpenMPI uses OMPI_COMM_WORLD_LOCAL_RANK, Intel oneAPI MPI and MPICH uses 
# The variable MPI_LOCALRANKID, the SLURM_LOCALID can also be used to derive it.
{% if mpi_implementation=="OPENMPI" %}
export node_local_mpi_rank="$OMPI_COMM_WORLD_LOCAL_RANK"
{% elif mpi_implementation=="MPICH" %}
export node_local_mpi_rank="$MPI_LOCALRANKID"
{% elif mpi_implementation=="INTEL" %}
export node_local_mpi_rank="$MPI_LOCALRANKID"
{% else %}
export node_local_mpi_rank="$SLURM_LOCALID"
{% endif %}

# MPICH and Intel oneAPI MPI might additionally need environment variables 
# To enable GPU-aware communication. Change according to your needs.
# For example I_MPI_GPU_SUPPORT, or MPICH_GPU_SUPPORT_ENABLED
{% if mpi_implementation=="INTEL" %}
export I_MPI_GPU_SUPPORT=1
{% elif mpi_implementation=="MPICH" %}
export MPICH_GPU_SUPPORT_ENABLED=1
{% endif %}

# Depending on your needs, other variables might be necessary depending on
# The backend and the GPU vendor different from the autodetected ones.
# For example: NVIDIA_VISIBLE_DEVICES, CUDA_VISIBLE_DEVICES, 
# ROCR_VISIBLE_DEVICES, HIP_VISIBLE_DEVICES, OPENCL_VISIBLE_DEVICES etc.
{% if gpu_vendor=="NVIDIA" %}
export CUDA_VISIBLE_DEVICES="$node_local_mpi_rank"
{% elif gpu_vendor=="AMD" %}
export ROCR_VISIBLE_DEVICES="$node_local_mpi_rank"
{% elif gpu_vendor=="INTEL" %}
export OPENCL_VISIBLE_DEVICES="$node_local_mpi_rank"
{% endif %}

# First input argument is the executable to be run:
# Usage ./gpu_select.sh <your_executable>
$1
