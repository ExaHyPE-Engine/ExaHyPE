import subprocess
from jinja2 import Template
import os

# Find out GPU vendor
def get_gpu_vendor(architecture_string):
  if architecture_string == None:
    return None
  elif architecture_string.lower().startswith("gfx"):
    return "AMD"
  elif architecture_string.lower().startswith("spir"):
    return "INTEL"
  elif architecture_string.lower().startswith("sm") or \
    architecture_string.lower().startswith("cc") or \
    architecture_string.isnumeric():
    return "NVIDIA"
  return None

def get_mpi_implementation(mpicxx):
    # Run mpicxx --version and infer which MPI implementation is being used,
    # MPICH and Intel oneAPI print information including MPICH and Intel(R) MPI Library
    # OpenMPI doesn't print any information that enables identification,
    # A workaround be running mpicxx --showme that will also involve openmpi include etc.
    bash_command = f"{mpicxx} -v"
    result = subprocess.run(bash_command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    result_str = ""
    if result.returncode == 0:
        result_str = result.stdout
    if "MPICH" in result_str: # MPICH
        return "MPICH"
    elif "Intel(R) MPI Library" in result_str: # Intel oneAPI
        return "INTEL"
    else:
        showme_bash_command = f"{mpicxx} --showme"
        showme_result = subprocess.run(showme_bash_command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        if showme_result.returncode == 0:
            if "openmpi" in showme_result.stdout:
                return "OPENMPI"
    return None


def render_gpu_select_script(mpicxx, arch, builddir):
    mpi_implementation = get_mpi_implementation(mpicxx)
    gpu_vendor = get_gpu_vendor(arch)

    if gpu_vendor == None:
        gpu_vendor = ""
    if mpi_implementation == None:
        mpi_implementation = ""

    template_string="""#!/bin/bash

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
"""

    context = {
        "gpu_vendor": gpu_vendor,
        "mpi_implementation": mpi_implementation
    }

    template = Template(template_string)
    rendered_content = template.render(context)

    with open(os.path.join(builddir, "gpu_select.sh"), "w") as file:
       file.write(rendered_content)