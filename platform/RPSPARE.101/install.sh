#!/usr/bin/env bash

cp autoexec.bin zip/
cd ../../../ML/qemu-eos
./mtools_copy_ml.sh ../../magiclantern_simplified/platform/RPSPARE.101/zip
