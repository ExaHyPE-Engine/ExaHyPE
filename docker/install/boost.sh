#!/bin/bash

set -e

INSTALL_DIR="/usr/local"
TMP_DIR=$(mktemp -d)

BOOST_VERSION=1.82.0
BOOST_VERSION_UNDERSCORE=${BOOST_VERSION//./_}
echo $BOOST_VERSION_UNDERSCORE
wget --progress=bar:force:noscroll https://downloads.sourceforge.net/project/boost/boost/${BOOST_VERSION}/boost_${BOOST_VERSION_UNDERSCORE}.tar.gz -P ${TMP_DIR}
tar -xvzf ${TMP_DIR}/boost_${BOOST_VERSION_UNDERSCORE}.tar.gz -C /
rm -rf ${TMP_DIR}
cd /boost_${BOOST_VERSION_UNDERSCORE}

./bootstrap.sh --prefix=${INSTALL_DIR} --with-libraries=serialization,wave,date_time,iostreams,locale,math,random,context,regex,program_options,atomic,timer,log,fiber,chrono,thread,exception,system,test,graph,filesystem
./b2 --clean && \
./b2 install variant=release link=static threading=multi address-model=64 architecture=x86 -j $(nproc)

rm -rf /boost*
