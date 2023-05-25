#!/bin/bash

# Check if the script is being executed with root privileges
if [[ $EUID -ne 0 ]]; then
  echo "This script must be run as root"
  exit 1
fi

# Set the timezone
ln -snf /usr/share/zoneinfo/$TZ /etc/localtime
echo $TZ > /etc/timezone
if [ $? -ne 0 ]; then
  echo "Failed to set the timezone."
  exit 1
fi

# Define colors
red=$(tput setaf 1)
green=$(tput setaf 2)
yellow=$(tput setaf 3)
magenta=$(tput setaf 5)
cyan=$(tput setaf 6)
bold=$(tput bold)
reset=$(tput sgr0)

declare -a essential_packages=(
  software-properties-common
  apt-utils
  build-essential
  bash
  wget
  git
  curl
  subversion
  tar
  unzip
  patch
  gzip
  bzip2
  file
  gnupg
  coreutils
  mercurial
  nano
  pkg-config
  tree
  flex
)

declare -a development_packages=(
  automake
  make
  cmake
  cmake-curses-gui
  ccache
  clang-format
  clang-tidy
  cppcheck
  libtool
  vim
  less
  gdb
  ninja-build
  meson
  graphviz
  doxygen
  xorg-dev
  mesa-utils
  mesa-common-dev
)

declare -a libraries=(
  ca-certificates
  libxml2-dev
  libbz2-dev
  libxslt1-dev
  libelf-dev
  libtbb-dev
  libsimde-dev
  libboost-all-dev
  libgl-dev
  libgl1-mesa-dev
  libglu1-mesa-dev
  libglvnd-core-dev
  vtk9
)

declare -a programming_packages=(
  nodejs
  g++
  gfortran
  python3
  python3-dev
  python3-pip
  g++-multilib
  gcc-multilib
)

declare -a hpc_packages=(
  openmpi-bin
  openmpi-common
  libopenmpi-dev
  hdf5-tools
  libhdf5-openmpi-dev
  netcdf-bin
  libmetis-dev
  libparmetis-dev
)

declare -a latex_packages=(
  texlive
  texlive-base
  texlive-bibtex-extra
  texlive-binaries
  texlive-extra-utils
  texlive-font-utils
  texlive-fonts-extra
  texlive-fonts-recommended
  texlive-formats-extra
  texlive-generic-extra
  texlive-generic-recommended
  texlive-lang-english
  texlive-lang-european
  texlive-latex-base
  texlive-latex-extra
  texlive-latex-recommended
  texlive-science
  texlive-pictures
  texlive-pstricks
  texlive-xetex
  latexmk
)

declare -a packages=(
  "${essential_packages[@]}"
  "${development_packages[@]}"
  "${libraries[@]}"
  "${programming_packages[@]}"
  "${hpc_packages[@]}"
  "${latex_packages[@]}"
)

# Upgrade and install the packages
apt-get update -y
if [ $? -ne 0 ]; then
  echo "Failed to update the package list."
  exit 1
fi

apt-get upgrade -y
if [ $? -ne 0 ]; then
  echo "Failed to upgrade the packages."
  exit 1
fi

install_packages() {
  local package_list=("$@")
  local package
  for package in "${package_list[@]}"; do
    if dpkg-query -W -f='${Status}' "$package" 2>/dev/null | grep -q '^install ok installed'; then
      echo "${cyan}Package '$package' already installed, skipping...${reset}"
      continue
    fi
    if ! apt-cache show "$package" &>/dev/null; then
      echo "${yellow}Package '$package' not found in the repository, skipping...${reset}"
      continue
    fi
    echo "${green}Installing package '$package'...${reset}"
    apt-get install -y --no-install-recommends "$package"
    if [ $? -ne 0 ]; then
      echo "${red}Failed to install package '$package'${reset}"
      exit 1
    fi
  done
}

install_group() {
  local group_name=$1
  local group_packages=("${!2}")

  echo "${bold}${magenta}Installing packages for group '$group_name'...${reset}"
  install_packages "${group_packages[@]}"
}

# Essential packages
install_group "Essential Packages" essential_packages[@]

# Development packages
install_group "Development Packages" development_packages[@]

# Libraries
install_group "Libraries" libraries[@]

# Programming packages
install_group "Programming Packages" programming_packages[@]

# HPC packages
install_group "HPC Packages" hpc_packages[@]

# LaTeX packages
install_group "LaTeX Packages" latex_packages[@]

# Clean up the system
apt-get clean -y
apt-get autoremove -y
apt-get autoclean -y
rm -rf /var/lib/apt/lists/*

# Upgrade and install python packages
declare -a python_packages=(
  pip
  jinja2
  numpy
  jsonschema
  jupyterlab
  sympy
  scipy
  matplotlib
  tikzplotlib
)

for package in "${python_packages[@]}"; do
  if python3 -c "import $package" > /dev/null 2>&1; then
    echo "${cyan}Python package $package is already installed${reset}"
  else
    python3 -m pip install "$package"
    if [ $? -ne 0 ]; then
      echo "${red}Error: Failed to install Python package $package${reset}"
      exit 1
    fi
  fi
done
