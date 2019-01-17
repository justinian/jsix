#!/usr/bin/env bash

TARGET="x86_64-elf"
NASM_VERSION="2.13.03"
BINUTILS_VERSION="2.31.1"

TOOLS="clang" # lld libunwind libcxxabi libcxx"
PROJECTS="compiler-rt libcxxabi libcxx libunwind"
#RUNTIMES="compiler-rt libcxxabi libcxx libunwind"

set -e

SYSROOT=$(realpath "$(dirname $0)/../sysroot")
WORK=$(realpath "$(dirname $0)/sysroot")
mkdir -p "${SYSROOT}"
mkdir -p "${WORK}"

export CC=clang
export CXX=clang++

if [[ ! -d "${WORK}/nasm-${NASM_VERSION}" ]]; then
	echo "Downloading NASM..."
	tarball="nasm-${NASM_VERSION}.tar.gz"
	curl -sSOL "https://www.nasm.us/pub/nasm/releasebuilds/${NASM_VERSION}/${tarball}"
	tar xzf "${tarball}" -C "${WORK}" && rm "${tarball}"
fi

if [[ ! -d "${WORK}/binutils-${BINUTILS_VERSION}" ]]; then
	echo "Downloading binutils..."
	tarball="binutils-${BINUTILS_VERSION}.tar.gz"
	curl -sSOL "https://ftp.gnu.org/gnu/binutils/${tarball}"
	tar xzf "${tarball}" -C "${WORK}" && rm "${tarball}"
fi

if [[ ! -d "${WORK}/llvm" ]]; then
	echo "Downloading LLVM..."
	git clone -q \
		--branch release_70 \
		--depth 1 \
		"https://git.llvm.org/git/llvm.git" "${WORK}/llvm"
fi

for tool in ${TOOLS}; do
	if [[ ! -d "${WORK}/llvm/tools/${tool}" ]]; then
		echo "Downloading ${tool}..."
		git clone -q \
			--branch release_70 \
			--depth 1 \
			"https://git.llvm.org/git/${tool}.git" "${WORK}/llvm/tools/${tool}"
	fi
done

if [[ ! -d "${WORK}/llvm/tools/clang/tools/extra" ]]; then
	echo "Downloading clang-tools-extra..."
	git clone -q \
		--branch release_70 \
		--depth 1 \
		"https://git.llvm.org/git/clang-tools-extra.git" "${WORK}/llvm/tools/clang/tools/extra"
fi

for proj in ${PROJECTS}; do
	if [[ ! -d "${WORK}/llvm/projects/${proj}" ]]; then
		echo "Downloading ${proj}..."
		git clone -q \
			--branch release_70 \
			--depth 1 \
			"https://git.llvm.org/git/${proj}.git" "${WORK}/llvm/projects/${proj}"
	fi
done

for proj in ${RUNTIMES}; do
	if [[ ! -d "${WORK}/llvm/runtimes/${proj}" ]]; then
		echo "Downloading ${proj}..."
		git clone -q \
			--branch release_70 \
			--depth 1 \
			"https://git.llvm.org/git/${proj}.git" "${WORK}/llvm/runtime/${proj}"
	fi
done

if [[ ! -d "${WORK}/poplibc" ]]; then
	echo "Downloading poplibc..."
	git clone \
		"https://github.com/justinian/poplibc.git" \
		"${WORK}/poplibc"
else
	echo "Updating poplibc..."
	git -C "${WORK}/poplibc" pull
fi

mkdir -p "${WORK}/build/nasm"
pushd "${WORK}/build/nasm"

echo "Configuring NASM..."

"${WORK}/nasm-${NASM_VERSION}/configure" \
	--quiet \
	--config-cache \
	--prefix="${SYSROOT}" \
	--srcdir="${WORK}/nasm-${NASM_VERSION}"

echo "Building NASM..."
(make -j && make install) > nasm_build.log
popd

mkdir -p "${WORK}/build/binutils"
pushd "${WORK}/build/binutils"

echo "Configuring binutils..."
"${WORK}/binutils-${BINUTILS_VERSION}/configure" \
	--quiet \
	--target="${TARGET}" \
	--prefix="${SYSROOT}" \
	--with-sysroot \
	--disable-nls \
	--disable-werror

echo "Building binutils..."
(make -j && make install) > "${WORK}/build/binutils_build.log"
popd

mkdir -p "${WORK}/build/llvm"
pushd "${WORK}/build/llvm"

echo "Configuring LLVM..."

cmake -G Ninja \
	-DCMAKE_BUILD_TYPE=Release \
	-DCLANG_DEFAULT_RTLIB=compiler-rt \
	-DCLANG_DEFAULT_STD_C=c11 \
	-DCLANG_DEFAULT_STD_CXX=cxx14 \
	-DCMAKE_C_COMPILER="clang" \
	-DCMAKE_CXX_COMPILER="clang++" \
	-DCMAKE_CXX_FLAGS="-Wno-unused-parameter -D_LIBCPP_HAS_NO_ALIGNED_ALLOCATION -D_LIBUNWIND_IS_BAREMETAL=1 -U_LIBUNWIND_SUPPORT_DWARF_UNWIND" \
	-DCMAKE_INSTALL_PREFIX="${SYSROOT}" \
	-DCMAKE_MAKE_PROGRAM=`which ninja` \
	-DDEFAULT_SYSROOT="${SYSROOT}" \
	-DLLVM_CONFIG_PATH="${SYSROOT}/bin/llvm-config" \
	-DLLVM_DEFAULT_TARGET_TRIPLE="x86_64-unknown-elf" \
	-DLLVM_INSTALL_BINUTILS_SYMLINKS=ON \
	-DLLVM_TARGETS_TO_BUILD="X86" \
	-DCMAKE_INSTALL_PREFIX="${SYSROOT}" \
	-DCMAKE_MAKE_PROGRAM=`which ninja` \
	-DLIBCXX_CXX_ABI=libcxxabi \
	-DLIBCXX_CXX_ABI_INCLUDE_PATHS="${WORK}/llvm/projects/libcxxabi/include" \
	-DLIBCXX_CXX_ABI_LIBRARY_PATH=lib \
	-DLIBCXX_ENABLE_EXPERIMENTAL_LIBRARY=OFF \
	-DLIBCXX_ENABLE_NEW_DELETE_DEFINITIONS=ON \
	-DLIBCXX_ENABLE_SHARED=OFF \
	-DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON \
	-DLIBCXX_ENABLE_THREADS=OFF \
	-DLIBCXX_INCLUDE_BENCHMARKS=OFF \
	-DLIBCXX_USE_COMPILER_RT=ON \
	-DLIBCXXABI_ENABLE_NEW_DELETE_DEFINITIONS=OFF \
	-DLIBCXXABI_ENABLE_THREADS=OFF \
	-DLIBCXXABI_ENABLE_SHARED=OFF \
	-DLIBCXXABI_ENABLE_STATIC_UNWINDER=ON \
	-DLIBCXXABI_LIBCXX_PATH="${WORK}/llvm/projects/libcxx" \
	-DLIBCXXABI_USE_COMPILER_RT=ON \
	-DLIBCXXABI_USE_LLVM_UNWINDER=ON \
	-DLIBUNWIND_ENABLE_SHARED=OFF \
	-DLIBUNWIND_ENABLE_THREADS=OFF \
	-DLIBUNWIND_USE_COMPILER_RT=ON \
	-DLLVM_CONFIG_PATH="${SYSROOT}/bin/llvm-config" \
	-DLLVM_DEFAULT_TARGET_TRIPLE="x86_64-unknown-elf" \
	-DLLVM_ENABLE_LIBCXX=ON \
	-DLLVM_ENABLE_PIC=OFF \
	-DLLVM_ENABLE_THREADS=OFF \
	-DLLVM_ENABLE_THREADS=OFF \
	-DLLVM_INSTALL_BINUTILS_SYMLINKS=ON \
	-DLLVM_TARGETS_TO_BUILD="X86" \
	${WORK}/llvm > cmake_configure.log

echo "Building LLVM..."
ninja && ninja install
ninja cxx cxxabi compiler-rt
ninja install-compiler-rt install-cxx install-cxxabi
popd

pushd "${WORK}/poplibc"
echo "Building poplibc..."
export CC="${SYSROOT}/bin/clang"
export LD="${SYSROOT}/bin/${TARGET}-ld"
export AR="${SYSROOT}/bin/${TARGET}-ar"
make -j install PREFIX="${SYSROOT}"
popd


#
#	mkdir -p ${WORK}/build/llvm
#	pushd ${WORK}/build/llvm
#
#	cmake -G Ninja \
#		-DCLANG_DEFAULT_RTLIB=compiler-rt \
#		-DCLANG_DEFAULT_STD_C=c11 \
#		-DCLANG_DEFAULT_STD_CXX=cxx14 \
#		-DCMAKE_ASM_COMPILER=nasm \
#		-DCMAKE_C_COMPILER="clang" \
#		-DCMAKE_CXX_COMPILER="clang++" \
#		-DDEFAULT_SYSROOT="${SYSROOT}" \
#		-DCMAKE_CXX_FLAGS="-v" \
#		-DCMAKE_INSTALL_PREFIX="${SYSROOT}" \
#		-DCMAKE_LINKER="${SYSROOT}/bin/ld.lld" \
#		-DCMAKE_MAKE_PROGRAM=`which ninja` \
#		-DCOMPILER_RT_ENABLE_LLD=ON \
#		-DLIBCXX_CXX_ABI=libcxxabi \
#		-DLIBCXX_CXX_ABI_INCLUDE_PATHS=${WORK}/libcxxabi/include \
#		-DLIBCXX_CXX_ABI_LIBRARY_PATH=lib \
#		-DLIBCXX_ENABLE_EXPERIMENTAL_LIBRARY=OFF \
#		-DLIBCXX_ENABLE_LLD=ON \
#		-DLIBCXX_ENABLE_NEW_DELETE_DEFINITIONS=ON \
#		-DLIBCXX_ENABLE_SHARED=OFF \
#		-DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON \
#		-DLIBCXX_ENABLE_STATIC_UNWINDER=ON \
#		-DLIBCXX_ENABLE_THREADS=OFF \
#		-DLIBCXX_INCLUDE_BENCHMARKS=OFF \
#		-DLIBCXX_USE_COMPILER_RT=ON \
#		-DLIBCXXABI_ENABLE_LLD=ON \
#		-DLIBCXXABI_ENABLE_NEW_DELETE_DEFINITIONS=OFF \
#		-DLIBCXXABI_ENABLE_THREADS=OFF \
#		-DLIBCXXABI_ENABLE_SHARED=OFF \
#		-DLIBCXXABI_ENABLE_STATIC_UNWINDER=ON \
#		-DLIBCXXABI_LIBCXX_PATH=${WORK}/libcxx \
#		-DLIBCXXABI_USE_COMPILER_RT=ON \
#		-DLIBCXXABI_USE_LLVM_UNWINDER=ON \
#		-DLIBUNWIND_ENABLE_LLD=ON \
#		-DLIBUNWIND_ENABLE_SHARED=OFF \
#		-DLIBUNWIND_ENABLE_THREADS=OFF \
#		-DLIBUNWIND_USE_COMPILER_RT=ON \
#		-DLLVM_CONFIG_PATH="${SYSROOT}/bin/llvm-config" \
#		-DLLVM_DEFAULT_TARGET_TRIPLE="x86_64-unknown-elf" \
#		-DLLVM_ENABLE_LIBCXX=ON \
#		-DLLVM_ENABLE_LLD=ON \
#		-DLLVM_ENABLE_PIC=OFF \
#		-DLLVM_ENABLE_PROJECTS="libcxx;libcxxabi;libunwind;compiler-rt" \
#		-DLLVM_ENABLE_THREADS=OFF \
#		-DLLVM_ENABLE_THREADS=OFF \
#		-DLLVM_INSTALL_BINUTILS_SYMLINKS=ON \
#		-DLLVM_TARGETS_TO_BUILD="X86" \
#		${WORK}/llvm
#
#	ninja && ninja install
#	ninja unwind cxx cxxabi compiler-rt
#	ninja install-compiler-rt install-unwind install-cxx install-cxxabi
#	popd
#
#
#export CC="${SYSROOT}/bin/clang"
#export CXX="${SYSROOT}/bin/clang++"
#export LD="${SYSROOT}/bin/ld.lld"
## -DCOMPILER_RT_BAREMETAL_BUILD=ON \
## -DLIBCXXABI_BAREMETAL=ON \
## -DCMAKE_C_COMPILER="${SYSROOT}/bin/clang" \
## -DCMAKE_CXX_COMPILER="${SYSROOT}/bin/clang++" \
## -DDEFAULT_SYSROOT="${SYSROOT}" \
