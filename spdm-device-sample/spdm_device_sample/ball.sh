#!/bin/bash
set -eou pipefail

rm -rf build
mkdir -p build && cd build
# cmake -DARCH=aarch64 -DTOOLCHAIN=AARCH64_GCC -DTARGET=Debug -DCRYPTO=openssl ..
cmake -DARCH=aarch64 -DTOOLCHAIN=AARCH64_GCC -DTARGET=Debug -DCRYPTO=mbedtls ..
make VERBOSE=1 -j1

./bin/spdm_device_responder   # > resp.txt

