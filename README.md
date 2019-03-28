# popcorn: A toy OS kernel

**popcorn** is the kernel for the hobby OS that I am currently building. It's
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

## Building

Popcorn uses the [Ninja][] build tool, and generates the build files for it
with a custom tool called [Bonnibel][]. Bonnibel requires [Python 3][] and can
be downloaded with `pip`:

```
pip3 install bonnibel
```

[Ninja]:    https://ninja-build.org
[Bonnibel]: https://github.com/justinian/bonnibel
[Python 3]: https://python.org

Requrirements:

* python 3 (for installing and running Bonnibel)
* clang
* mtools
* ninja
* curl for downloading the toolchain

### Setting up the cross toolchain

If you have `clang` and `curl` installed, runing the `scripts/build_sysroot_clang.sh`
script will download and build a nasm/binutils/LLVM toolchain configured for building
Popcorn host binaries.

### Building and running Popcorn

Once the toolchain has been set up, running Bonnibel's `pb` command will set up the
build configuration, and `ninja -C build` will actually run the build.  If you
have `qemu-system-x86_64` installed, the `qemu.sh` script will to run Popcorn
in QEMU `-nographic` mode.

I personally run this either from a real debian amd64 testing/buster machine or
a windows WSL debian testing/buster installation. The following should be
enough to set up such a system to build the kernel:

    sudo apt install qemu-system-x86 nasm clang-6.0 mtools python3-pip curl
    sudo update-alternatives /usr/bin/clang clang /usr/bin/clang-6.0 1000
    sudo update-alternatives /usr/bin/clang++ clang++ /usr/bin/clang++-6.0 1000
    sudo pip3 install bonnibel

