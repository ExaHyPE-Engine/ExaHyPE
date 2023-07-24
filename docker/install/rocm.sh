# https://repo.radeon.com/
# https://github.com/RadeonOpenCompute/ROCm-docker/blob/master/quick-start.md
# https://docs.amd.com/bundle/ROCm-Installation-Guide-v5.3/page/Introduction_to_ROCm_Installation_Guide_for_Linux.html

# Install ROCm 5.2
RUN apt-get install -yqq libnuma-dev gnupg2 && \
wget --progress=bar:force:noscroll https://repo.radeon.com/amdgpu-install/22.20/ubuntu/bionic/amdgpu-install_22.20.50200-1_all.deb && \
apt-get install -y ./amdgpu-install_22.20.50200-1_all.deb && \
amdgpu-install -y --usecase=hiplibsdk,rocm && \
rm ./amdgpu-install_22.20.50200-1_all.deb && \
echo 'export PATH=$PATH:/opt/rocm/bin:/opt/rocm/rocprofiler/bin:/opt/rocm/opencl/bin' | tee -a /etc/profile.d/rocm.sh && \
echo 'export HIPCC_VERBOSE=7' | tee -a /etc/profile.d/rocm.sh && \
echo 'export HIP_PLATFORM=nvidia' | tee -a /etc/profile.d/rocm.sh