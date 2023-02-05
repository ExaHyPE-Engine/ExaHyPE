#!/bin/bash

# This is an interactive script which configures the Peano project with every toolbox available.
# If you want to use only a subset of toolboxes, copy the commands into your command line and adapt it accordingly.

read -p "Enter CC compiler [clang]: " cc
cc=${cc:-clang}
read -p "Enter CXX compiler [clang++]: " cxx
cxx=${cxx:-clang++}
#read -p "Enter Fortran compiler: " fcc

CXXFLAGS="-std=c++17 -W -Wall -Wextra"
LDFLAGS=""

PS3="Select multithreading mode: "
select shared in none omp; do
  case $shared in
    "none")
      break;;
    "omp")
      break;;
  esac
done

if [ ! "$shared" = "none" ]
then
  CXXFLAGS+=" -fopenmp"

  PS3="Select GPU offloading mode: "
  select gpu in none omp; do
    case $gpu in
      "none")
        break;;
      "omp")
        break;;
    esac
  done

  if [ ! "$gpu" = "none" ]
  then
    PS3="Select GPU compute capability: "
    select cap in sm_50 sm_52 sm_53 sm_60 sm_61 sm_62 sm_70 sm_72 sm_75 sm_80 sm_86 sm_90; do
      case $cap in
        "sm_50")
          break;;
        "sm_52")
          break;;
        "sm_53")
          break;;
        "sm_60")
          break;;
        "sm_61")
          break;;
        "sm_62")
          break;;
        "sm_70")
          break;;
        "sm_72")
          break;;
        "sm_75")
          break;;
        "sm_80")
          break;;
        "sm_86")
          break;;
        "sm_90")
          break;;
      esac
    done

    CXXFLAGS+=" -fopenmp-targets=nvptx64-nvidia-cuda -Xopenmp-target -march=$cap -Wno-unknown-cuda-version"

    echo "$cc $CXXFLAGS"
    echo "$cxx $CXXFLAGS"
    echo "LDFLAGS: $LDFLAGS"

    libtoolize && aclocal && autoconf && autoheader && cp src/config.h.in . && automake --add-missing && \
    ./configure \
      --enable-exahype \
      --enable-loadbalancing \
      --enable-blockstructured \
      --enable-fem \
      --with-fenv \
      --with-gpu=$gpu \
      --with-multithreading=$shared \
      CC=$cc CXX=$cxx \
      CXXFLAGS="$CXXFLAGS" \
      LDFLAGS="$LDFLAGS"
  else
    echo "$cc $CXXFLAGS"
    echo "$cxx $CXXFLAGS"
    echo "LDFLAGS: $LDFLAGS"

    libtoolize && aclocal && autoconf && autoheader && cp src/config.h.in . && automake --add-missing && \
    ./configure \
      --enable-exahype \
      --enable-loadbalancing \
      --enable-blockstructured \
      --enable-particles \
      --enable-fem \
      --enable-swift \
      --with-fenv \
      --with-multithreading=$shared \
      CC=$cc CXX=$cxx \
      CXXFLAGS="$CXXFLAGS" \
      LDFLAGS="$LDFLAGS"
  fi
fi
