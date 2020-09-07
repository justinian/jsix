#!/usr/bin/env bash

set -o errexit

ROOT=$(realpath $(dirname $0))
BUILD="${ROOT}/build"
TESTS="${BUILD}/native/tests" 

ninja -C "${BUILD}" "${TESTS}"
"${TESTS}" $*
