#!/bin/bash

# This is an interactive script which configures the Peano project with every toolbox available.
# If you want to use only a subset of toolboxes, copy the commands into your command line and adapt it accordingly.

read -p "Enter CC compiler [amdclang, hipcc]: " cc
cc=${cc:-amdclang}
read -p "Enter CXX compiler [amdclang++, hipcc]: " cxx
cxx=${cxx:-amdclang++}
#read -p "Enter Fortran compiler: " fcc

CXXFLAGS="-std=c++20 -W -Wall -Wextra"
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
  LDFLAGS+=" -fopenmp"

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
    select cap in gfx906 gfx908 gfx90a; do
      case $cap in
        "gfx906") # MI 50
          break;;
        "gfx908") # MI 100
          break;;
        "gfx90a") # MI 200
          break;;
      esac
    done

    CXXFLAGS+=" --offload-arch=$cap"
    LDFLAGS+=" --offload-arch=$cap -lstdc++fs"

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
