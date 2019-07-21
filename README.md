# jsix: A toy OS kernel

**jsix** is the kernel for the hobby OS that I am currently building. It's
far from finished, or even being usable. Instead, it's a sandbox for me to play
with kernel-level code and explore architectures.

The design goals of the project are:

* Modernity - I'm not interested in designing for legacy systems, or running on
  all hardware out there. My target is only 64 bit architecutres, and modern
  commodity hardware. Currently that means x64 systems with Nehalem or newer
  CPUs and UEFI firmware. Eventually I'd like to work on an AArch64 port,
  partly to force myself to factor out the architecture-dependent pieces of the
  code base.

* Modularity - I'd like to pull as much of the system out into separate
  processes as possible, in the microkernel fashion. A sub-goal of this is to
  explore where the bottlenecks of such a microkernel are now, and whether
  eschewing legacy hardware will let me design a system that's less bogged down
  by the traditional microkernel problems. Given that there are no processes
  yet, the kernel is monolithic by default.

* Exploration - I'm really mostly doing this to have fun learning and exploring
  modern OS development. Modular design may be tossed out (hopefully
  temporarily) in some places to allow me to play around with the related
  hardware.

A note on the name: This kernel was originally named Popcorn, but I have since
discovered that the Popcorn Linux project is also developing a kernel with that
name, started around the same time as this project. So I've renamed this kernel
jsix (Always styled _jsix_ or `j6`, never capitalized) as an homage to L4, xv6,
and my wonderful wife.

## Building

jsix uses the [Ninja][] build tool, and generates the build files for it with a
custom tool called [Bonnibel][]. Bonnibel can be installed with [Cargo][], or
downloaded as a prebuilt binary from its Github repository.

[Ninja]:    https://ninja-build.org
[Bonnibel]: https://github.com/justinian/bonnibel
[Cargo]:    https://crates.io/crates/bonnibel

Requrirements:

* bonnibel
* ninja
* clang
* mtools
* curl for downloading the toolchain

### Setting up the cross toolchain

If you have `clang` and `curl` installed, runing the `scripts/build_sysroot_clang.sh`
script will download and build a nasm/binutils/LLVM toolchain configured for building
jsix host binaries.

### Building and running jsix

Once the toolchain has been set up, running Bonnibel's `pb init` command will
set up the build configuration, and `pb build` will actually run the build.  If
you have `qemu-system-x86_64` installed, the `qemu.sh` script will to run jsix
in QEMU `-nographic` mode.

I personally run this either from a real debian amd64 testing/buster machine or
a windows WSL debian testing/buster installation. The following should be
enough to set up such a system to build the kernel:

    sudo apt install qemu-system-x86 nasm clang-6.0 mtools curl
    sudo update-alternatives /usr/bin/clang clang /usr/bin/clang-6.0 1000
    sudo update-alternatives /usr/bin/clang++ clang++ /usr/bin/clang++-6.0 1000
	curl -L -o pb https://github.com/justinian/bonnibel/releases/download/2.0.0/pb_linux_amd64 && chmod a+x pb

