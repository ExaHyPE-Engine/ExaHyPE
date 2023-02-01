#!/bin/bash

# This is an interactive script which configures the Peano project with every toolbox available.
# If you want to use only a subset of toolboxes, copy the commands into your command line and adapt it accordingly.

read -p "Enter CC compiler: " cc
read -p "Enter CXX compiler: " cxx
#read -p "Enter Fortran compiler: " fcc

CXXFLAGS="-std=c++20 -W -Wall -Wextra"
LDFLAGS=""

PS3="Select multithreading mode: "
select shared in none omp sycl; do
  case $shared in
    "none")
      break;;
    "omp")
      break;;
    "sycl")
      break;;
  esac
done

if [ ! "$shared" = "none" ]
then
  CXXFLAGS+=" -fiopenmp"
  LDFLAGS+=" -fiopenmp"

  PS3="Select GPU offloading mode: "
  select gpu in none omp sycl; do
    case $gpu in
      "none")
        break;;
      "omp")
        break;;
      "sycl")
        break;;
    esac
  done

  echo "CXXFLAGS: $CXXFLAGS"
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
