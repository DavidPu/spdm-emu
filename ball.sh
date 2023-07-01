#!/bin/bash
set -eou pipefail
mydir=$(dirname $(readlink -f $0))

rm -rf build && mkdir -p build
cd build
# cmake -DARCH=x64 -DTOOLCHAIN=GCC -DTARGET=Debug -DCRYPTO=mbedtls ..
cmake -DARCH=aarch64 -DTOOLCHAIN=AARCH64_GCC -DTARGET=Debug -DCRYPTO=mbedtls ..
make copy_sample_key
(cd $mydir/libspdm/unit_test/sample_key && python3 raw_data_key_gen.py)
make
