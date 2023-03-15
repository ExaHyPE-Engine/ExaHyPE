#!/bin/bash

apt-get update -y && apt-get upgrade -y && \
ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone && \
apt-get install -y --no-install-recommends \
    software-properties-common \
    apt-utils \
    build-essential \
    bash \
    wget \
    git \
    curl \
    subversion \
    tar \
    unzip \
    patch \
    zip \
    gzip \
    bzip2 \
    file \
    gnupg2 \
    coreutils \
    file \
    mercurial \
    zstd \
    nano \
    zip \
    pkg-config \
    tree \
    automake \
    make \
    ccache \
    clang-format \
    clang-tidy \
    cppcheck \
    libtool \
    vim \
    less \
    ca-certificates \
    libxml2-dev \
    gdb \
    gdbserver \
    graphviz \
    iwyu \
    unifdef \
    valgrind \
    doxygen \
    qemu \
    qemu-kvm \
    libnuma1 \
    libtbb-dev \
    perl \
    snapd \
    libbz2-dev \
    libxslt1-dev \
    libelf-dev \
    nodejs \
    g++ \
    gfortran \
    g++-multilib \
    gcc-multilib \
    libsimde-dev \
    meson \
    ninja-build \
    python2-dev \
    python3 \
    python3-dev \
    python3-pip \
    python3-setuptools \
    python3-paraview \
    texlive \
    texlive-full \
    libglu1-mesa-dev \
    libgl-dev \
    libgl1-mesa-dev \
    xorg-dev \
    mesa-common-dev \
    mesa-utils \
    libglvnd-core-dev \
    vtk9 \
    openmpi-bin \
    openmpi-common \
    libopenmpi-dev \
    hdf5-tools \
    libhdf5-openmpi-dev \
    netcdf-bin \
    libmetis-dev \
    libparmetis-dev \
    cxxtest \
    catch2 \
    libyaml-cpp-dev \
    gedit \
    libboost-all-dev && \
    apt-get clean && apt-get autoremove && apt-get autoclean && rm -rf /var/lib/apt/lists/*

python3 -m pip install --upgrade pip jinja2 numpy jsonschema jupyterlab sympy scipy matplotlib hpccm tikzplotlib
