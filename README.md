![jsix](assets/jsix.svg)

# The jsix operating system

**jsix** is a custom multi-core x64 operating system that I am building from
scratch. It's far from finished, or even being usable - see the *Status and
Roadmap* section, below.

The design goals of the project are:

* Modernity - I'm not interested in designing for legacy systems, or running on
  all hardware out there. My target is only 64 bit architectures, and modern
  commodity hardware. Currently that means x64 systems with Nehalem or newer
  CPUs and UEFI firmware. (See [this list][cpu_features] for the currently
  required CPU features.) Eventually I'd like to work on an AArch64 port,
  partly to force myself to factor out the architecture-dependent pieces of the
  code base.

* Modularity - I'd like to pull as much of the system out into separate
  processes as possible, in the microkernel fashion. A sub-goal of this is to
  explore where the bottlenecks of such a microkernel are now, and whether
  eschewing legacy hardware will let me design a system that's less bogged down
  by the traditional microkernel problems.

* Exploration - I'm really mostly doing this to have fun learning and exploring
  modern OS development. Initial feature implementations may temporarily throw
  away modular design to allow for exploration of the related hardware.

A note on the name: This kernel was originally named Popcorn, but I have since
discovered that the Popcorn Linux project is also developing a kernel with that
name, started around the same time as this project. So I've renamed this kernel
jsix (Always styled _jsix_ or `j6`, never capitalized) as an homage to L4, xv6,
and my wonderful wife.

[cpu_features]: https://github.com/justinian/jsix/blob/master/src/libraries/cpu/include/cpu/features.inc

## Status and Roadmap

The following major feature areas are targets for jsix development:

#### UEFI boot loader

_Done._ The bootloader loads the kernel and initial userspace programs, and
sets up necessary kernel arguments about the memory map and EFI GOP
framebuffer. Possible future ideas:

- take over more init-time functions from the kernel
- rewrite it in Zig

#### Memory

_Virtual memory: Sufficient._ The kernel manages virtual memory with a number
of kinds of `vm_area` objects representing mapped areas, which can belong to
one or more `vm_space` objects which represent a whole virtual memory space.
(Each process has a `vm_space`, and so does the kernel itself.)

Remaining to do:

- TLB shootdowns
- Page swapping
- Large / huge page support

_Physical page allocation: Sufficient._ The current physical page allocator
implementation uses a group of blocks representing up-to-1GiB areas of usable
memory as defined by the bootloader. Each block has a three-level bitmap
denoting free/used pages.

Future work:

- Align blocks so that their first page is 1GiB-aligned, making finding free
  large/huge pages easier.

#### Multitasking

_Sufficient._ The global scheduler object keeps separate ready/blocked lists
per core. Cores periodically attempt to balance load via work stealing.

User-space tasks are able to create threads as well as other processes.

Several kernel-only tasks exist, though I'm trying to reduce that. Eventually
only the timekeeping task should be a separate kernel-only thread.

#### API

_In progress._ User-space tasks are able to make syscalls to the kernel via
fast SYSCALL/SYSRET instructions. Syscalls made via `libj6` look to both the
callee and the caller like standard SysV ABI function calls. The
implementations are wrapped in generated wrapper functions which validate the
request, check capabilities, and find the appropriate kernel objects or handles
before calling the implementation functions.

Current work in this area:

- The protocol for processes and their children for both initialization and
  sharing resources. The kernel is ignorant of this, except for providing IPC
  capabilities.

#### Hardware Support

  * Framebuffer driver: _In progress._ Currently on machines with a video
    device accessible by UEFI, jsix starts a user-space framebuffer driver that
    only prints out kernel logs.
  * Serial driver: _In progress._ The current UART driver does not expose the
    UART as an output resource yet, it merely gets the kernel logs and sends
    them over serial.
  * USB driver: _To do_
  * AHCI (SATA) driver: _To do_

## Building

jsix uses the [Ninja][] build tool, and generates the build files for it with
the `configure` script. The build also relies on a custom toolchain sysroot, which can be
downloaded or built using the scripts in [jsix-os/toolchain][].

[Ninja]:             https://ninja-build.org
[jsix-os/toolchain]: https://github.com/jsix-os/toolchain

Other build dependencies:

* [clang][]: the C/C++ compiler
* [nasm][]: the assembler
* [lld][]: the linker
* [mtools][]: for creating the FAT image

[clang]:    https://clang.llvm.org
[nasm]:     https://www.nasm.us
[lld]:      https://lld.llvm.org
[mtools]:   https://www.gnu.org/software/mtools/

The `configure` script has some Python dependencies - these can be installed via
`pip`, though doing so in a python virtual environment is recommended.
Installing via `pip` will also install `ninja`.

A Debian 11 (Bullseye) system can be configured with the necessary build
dependencies by running the following commands from the jsix repository root:

```bash
sudo apt install clang lld nasm mtools python3-pip python3-venv
python3 -m venv ./venv
source venv/bin/activate
pip install -r requirements.txt
peru sync
```

### Setting up the sysroot

Build or download the toolchain sysroot as mentioned above with
[jsix-os/toolchain][], and symlink the built toolchain directory as `sysroot`
at the root of this project.

```bash
# Example if both the toolchain and this project are cloned under ~/src
ln -s ~/src/toolchain/toolchains/llvm-13 ~/src/jsix/sysroot
```

### Building and running jsix

Once the toolchain has been set up, running the `./configure` script (see
`./configure --help` for available options) will set up the build configuration,
and `ninja -C build` (or wherever you put the build directory) will actually run
the build. If you have `qemu-system-x86_64` installed, the `qemu.sh` script will
to run jsix in QEMU `-nographic` mode.

I personally run this either from a real debian amd64 bullseye machine or
a windows WSL debian bullseye installation. Your mileage may vary with other
setups and distros.

### Running the test suite

jsix now has the `test_runner` userspace program that runs various automated
tests. It is not included in the default build, but if you use the `test.yml`
manifest it will be built, and can be run with the `test.sh` script or the
`qemu.sh` script.

```bash
./configure --manifest=assets/manifests/test.yml
if ./test.sh; then echo "All tests passed!"; else echo "Failed."; fi
```
