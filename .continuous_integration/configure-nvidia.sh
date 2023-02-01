#!/bin/bash

# This is an interactive script which configures the Peano project with every toolbox available.
# If you want to use only a subset of toolboxes, copy the commands into your command line and adapt it accordingly.

read -p "Enter CC compiler: " cc
read -p "Enter CXX compiler: " cxx
#read -p "Enter Fortran compiler: " fcc

CXXFLAGS="-std=c++17 -W -Wall -Wextra"
LDFLAGS=""

PS3="Select multithreading mode: "
select shared in none omp cpp; do
  case $shared in
    "none")
      break;;
    "omp")
      break;;
    "cpp")
      break;;
  esac
done

if [ ! "$shared" = "none" ]
then
  PS3="Select GPU offloading mode: "
  select gpu in none omp cpp; do
    case $gpu in
      "none")
        break;;
      "omp")
        break;;
      "cpp")
        break;;
    esac
  done

  if [ ! "$gpu" = "none" ]
  then
    PS3="Select GPU compute capability: "
    select cap in cc50 cc52 cc53 cc60 cc61 cc62 cc70 cc72 cc75 cc80 cc86 cc90; do
      case $cap in
        "cc50")
          break;;
        "cc52")
          break;;
        "cc53")
          break;;
        "cc60")
          break;;
        "cc61")
          break;;
        "cc62")
          break;;
        "cc70")
          break;;
        "cc72")
          break;;
        "cc75")
          break;;
        "cc80")
          break;;
        "cc86")
          break;;
        "cc90")
          break;;
      esac
    done

    CXXFLAGS+=" -mp=gpu -gpu=$cap"
    LDFLAGS+=" -mp=gpu -gpu=$cap"

    PS3="Do you want to enable CUDA?: "
    select cuda in yes no; do
        case $cuda in
        "yes")
          CXXFLAGS+=" -cuda"
          LDFLAGS+=" -cuda"
          break;;
        "no")
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
      --with-gpu=$gpu \
      --with-multithreading=$shared \
      CC=$cc CXX=$cxx \
      CXXFLAGS="$CXXFLAGS" \
      LDFLAGS="$LDFLAGS"
  fi
fi
