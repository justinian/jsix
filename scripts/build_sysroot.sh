#!/bin/bash

set -o errexit
set -o errtrace
set -o pipefail
set -o nounset

function errmsg () {
    echo "Error: $(caller)"
    echo "Working directory not cleaned up!"
}

trap errmsg ERR

LLVM_VERSION=11
J6_TOOLCHAINS="${J6_TOOLCHAINS:-$(realpath ~/.local/lib/jsix)}"
CHAIN_NAME="llvm-${LLVM_VERSION}"
LLVM_BRANCH="release/${LLVM_VERSION}.x"

SYSROOT="${J6_TOOLCHAINS}/sysroots/${CHAIN_NAME}"

ROOT=$(realpath ${1:-$(mktemp -d "sysroot_build.XXX")})
OUT="${ROOT}/sysroot"
echo "Working in ${ROOT}"

if [ ! -d "${ROOT}/llvm-project" ]; then
    echo "* Downloading LLVM ${LLVM_VERSION}"

    git clone -q --depth 1 \
        --branch "${LLVM_BRANCH}" \
        "https://github.com/llvm/llvm-project" \
        "${ROOT}/llvm-project"
fi

if [ -d "${OUT}" ]; then
    rm -rf "${OUT}"
fi
mkdir -p "${OUT}"

## Stage1: build an LLVM toolchain that doesn't rely on the host libraries,
## compiler, runtime, etc.

echo "* Configuring stage1"

mkdir -p "${ROOT}/stage1"
cmake -G Ninja \
    -S "${ROOT}/llvm-project/llvm" \
    -B "${ROOT}/stage1" \
	-DCLANG_DEFAULT_RTLIB=compiler-rt \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_C_COMPILER="clang" \
	-DCMAKE_C_COMPILER_LAUNCHER=ccache \
	-DCMAKE_CXX_COMPILER="clang++" \
	-DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
	-DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=lld" \
	-DLIBCXXABI_USE_COMPILER_RT=ON \
	-DLIBCXXABI_USE_LLVM_UNWINDER=ON \
	-DLIBCXX_CXX_ABI=libcxxabi  \
	-DLIBUNWIND_USE_COMPILER_RT=ON \
	-DLLVM_BUILD_TOOLS=OFF \
	-DLLVM_ENABLE_BINDINGS=OFF \
	-DLLVM_ENABLE_PROJECTS="clang;lld;libcxx;libcxxabi;libunwind;compiler-rt" \
	-DLLVM_INCLUDE_BENCHMARKS=OFF \
	-DLLVM_INCLUDE_EXAMPLES=OFF \
	-DLLVM_INCLUDE_TESTS=OFF \
	-DLLVM_INCLUDE_UTILS=OFF \
	-DLLVM_TARGETS_TO_BUILD="X86" \
	-DLLVM_USE_LINKER="lld" \
	2>&1 > "${ROOT}/configure_stage1.log"

echo "* Building stage1"

ninja -C "${ROOT}/stage1" -v > "${ROOT}/build_stage1.log"
STAGE1="${ROOT}/stage1/bin"

## Stage2: build the sysroot libraries with the stage1 toolchain

echo "* Configuring stage2"

mkdir -p "${ROOT}/stage2"
cmake -G Ninja \
    -S "${ROOT}/llvm-project/llvm" \
    -B "${ROOT}/stage2" \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_C_COMPILER="${STAGE1}/clang" \
	-DCMAKE_C_COMPILER_LAUNCHER=ccache \
	-DCMAKE_CXX_COMPILER="${STAGE1}/clang++" \
	-DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
	-DCMAKE_INSTALL_PREFIX="${OUT}" \
	-DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=${STAGE1}/ld.lld" \
	-DLIBCXXABI_ENABLE_STATIC_UNWINDER=ON \
	-DLIBCXXABI_USE_COMPILER_RT=ON \
	-DLIBCXXABI_USE_LLVM_UNWINDER=ON \
	-DLIBCXX_CXX_ABI=libcxxabi  \
	-DLIBCXX_INCLUDE_BENCHMARKS=OFF \
	-DLIBUNWIND_USE_COMPILER_RT=ON \
	-DLLVM_DEFAULT_TARGET_TRIPLE="x86_64-unknown-elf" \
	-DLLVM_ENABLE_BINDINGS=OFF \
	-DLLVM_ENABLE_LIBCXX=ON \
	-DLLVM_ENABLE_PROJECTS="libcxx;libcxxabi;libunwind" \
	-DLLVM_INCLUDE_BENCHMARKS=OFF \
	-DLLVM_INCLUDE_EXAMPLES=OFF \
	-DLLVM_INCLUDE_TESTS=OFF \
	-DLLVM_INCLUDE_TOOLS=OFF \
	-DLLVM_INCLUDE_UTILS=OFF \
	-DLLVM_TARGETS_TO_BUILD="X86" \
	-DLLVM_USE_LINKER="${STAGE1}/ld.lld" \
	2>&1 > "${ROOT}/configure_stage2.log"

echo "* Building stage2"

ninja -C "${ROOT}/stage2" -v > "${ROOT}/build_stage2.log"

echo "* Installing stage2"

ninja -C "${ROOT}/stage2" -v install/strip > "${ROOT}/install_stage2.log"

echo "* Installing new sysroot"

rm -rf "${SYSROOT}"
mv "${OUT}" "${SYSROOT}"

echo "* Cleaning up"
trap - ERR
rm -rf "${ROOT}"
echo "Done"
