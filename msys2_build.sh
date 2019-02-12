#!/bin/bash

echo "== msys2 build script"
which bash
which gcc
pacman -S msys/automake1.16 msys/autoconf
pwd && bash ./bootstrap.sh && ./autogen.sh --prefix=$PWD/install && make && make check && make install && bash .travis-ghpages.sh || cat config.log
