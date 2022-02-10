#!/usr/bin/env bash

make clean && make CONFIG_QEMU=y -j8
# make clean && make -j8
mkdir zip
mkdir zip/ML
cp autoexec.bin zip/
sudo cp autoexec.bin /mnt/vm_share/
./install.sh

