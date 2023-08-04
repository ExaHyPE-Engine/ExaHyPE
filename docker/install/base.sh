#!/bin/bash

set -e

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

declare -a apt_packages=(
  software-properties-common
  apt-utils
  build-essential
  bash
  wget
  git
  curl
  ssh
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
  automake
  make
  cmake
  cmake-curses-gui
  ccache
  cppcheck
  libtool
  gcovr
  vim
  less
  meld
  gedit
  gdb
  strace
  perf
  valgrind
  unifdef
  ninja-build
  meson
  graphviz
  doxygen
  xorg-dev
  mesa-utils
  mesa-common-dev
  freeglut3-dev
  ca-certificates
  libxml2-dev
  libbz2-dev
  libxslt1-dev
  libelf-dev
  libtbb-dev
  libtbb2
  libsimde-dev
  libcurl4-gnutls-dev
  libcurl4-openssl-dev
  libgl-dev
  libgl1-mesa-dev
  libglu1-mesa-dev
  libglvnd-core-dev
  libyaml-cpp-dev
  nodejs
  gcc
  g++
  gfortran
  libgomp1
  libnuma-dev
  libnuma1
  python3
  python3-dev
  python3-venv
  python3-pip
  python3-paraview
  g++-multilib
  gcc-multilib
  zlib1g
  zlib1g-dev
  uidmap
  squashfs-tools
  squashfuse
  fuse2fs
  fuse-overlayfs
  fakeroot
  cryptsetup
  golang
  gocryptfs
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
  texlive-full
  latexmk
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
      echo "${cyan}Package '$package' is already installed, skipping...${reset}"
      continue
    fi
    if ! apt-cache show "$package" &>/dev/null; then
      echo "${yellow}Package '$package' not found in the repository, skipping...${reset}"
      continue
    fi
    echo "${green}Installing apt package '$package'...${reset}"
    apt-get install -y --no-install-recommends "$package"
    if [ $? -ne 0 ]; then
      echo "${red}Failed to install package '$package'${reset}"
      exit 1
    fi
  done
}

install_packages "${apt_packages[@]}"

# Clean up the system
apt-get clean -y
apt-get autoremove -y
apt-get autoclean -y
rm -rf /var/lib/apt/lists/*

# Upgrade and install python packages
declare -a python_packages=(
  pip
  jinja2
  jsonschema
  jupyterlab
  numpy
  sympy
  scipy
  matplotlib
  tikzplotlib
  hpccm
  black
  h5py
  mpmath
  umbridge
  clang-format
  clang-tidy
)

for package in "${python_packages[@]}"; do
  if python3 -c "import $package" > /dev/null 2>&1; then
    echo "${cyan}Python package '$package' is already installed, skipping...${reset}"
  else
    echo "${green}Installing Python package '$package'...${reset}"
    python3 -m pip install --upgrade "$package"
    if [ $? -ne 0 ]; then
      echo "${red}Failed to install Python package '$package'${reset}"
      exit 1
    fi
  fi
done
