#!/bin/bash

# source bootstrap.sh

set -xe

if [ ! -d vbcc ]; then
  git clone https://github.com/erique/vbcc_vasm_vlink.git
  cd vbcc_vasm_vlink && ./test.sh && cp -r build/vbcc .. && cd -
  rm -rf vbcc_vasm_vlink
fi

if [ ! -d ndk ]; then
  curl -#LO http://aminet.net/dev/misc/NDK3.2.lha
  7z x NDK3.2.lha -ondk
  rm NDK3.2.lha
fi

set +xe

export VBCC=$PWD/vbcc
export PATH=$PATH:$VBCC/bin
export NDK=$PWD/ndk
