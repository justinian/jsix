![jsix](assets/jsix.svg)

# The jsix operating system

**jsix** is a custom multi-core x64 operating system that I am building from
scratch. It's far from finished, or even being usable - see the *Status and
Roadmap* section, below.

The design goals of the project are:

* Modernity - I'm not interested in designing for legacy systems, or running on
  all hardware out there. My target is only 64 bit architecutres, and modern
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

_Physical page allocation: Sufficient._ The current physical page allocator
implementation suses a group of block representing up-to-1GiB areas of usable
memory as defined by the bootloader. Each block has a three-level bitmap
denoting free/used pages.

#### Multitasking

_Sufficient._ The global scheduler object keeps separate ready/blocked lists
per core. Cores periodically attempt to balance load via work stealing.

User-space tasks are able to create threads as well as other processes.

Several kernel-only tasks exist, though I'm trying to reduce that. Eventually
only the timekeeping task should be a separate kernel-only thread.

#### API

_In progress._ User-space tasks are able to make syscalls to the kernel via
fast SYSCALL/SYSRET instructions.

Major tasks still to do:

- The process initialization protocol needs to be re-built entirely.
- Processes' handles to kernel objects need the ability to check capabilities

#### Hardware Support

  * Framebuffer driver: _In progress._ Currently on machines with a video
	device accessible by UEFI, jsix starts a user-space framebuffer driver that
	only prints out kernel logs.
  * Serial driver: _To do._ Machines without a video device should have a
	user-space log output task like the framebuffer driver, but currently this
	is done inside the kernel.
  * USB driver: _To do_
  * AHCI (SATA) driver: _To do_

## Building

jsix uses the [Ninja][] build tool, and generates the build files for it with
the `configure` script. The build also relies on a custom sysroot, which can be
downloaded via the [Peru][] tool, or built locally.

[Ninja]:    https://ninja-build.org
[Peru]:     https://github.com/buildinspace/peru

Other build dependencies:

* [clang][]: the C/C++ compiler
* [nasm][]: the assembler
* [lld][]: the linker
* [mtools][]: for creating the FAT image
* [curl][]: if using `peru` below to download the sysroot

[clang]:    https://clang.llvm.org
[nasm]:     https://www.nasm.us
[lld]:      https://lld.llvm.org
[mtools]:   https://www.gnu.org/software/mtools/
[curl]:     https://curl.se

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

Running `peru sync` as in the above section will download and unpack the
toolchain into `sysroot`. 

#### Compiling the sysroot yourself

If you have CMake installed, runing the `scripts/build_sysroot.sh` script will
download and build a LLVM toolchain configured for building the sysroot, and
then build the sysroot with it.

Built sysroots are actually stored in `~/.local/lib/jsix/sysroots` and installed
in the project dir via symbolic link, so having mulitple jsix working trees or
switching sysroot versions is easy.

### Building and running jsix

Once the toolchain has been set up, running the `./configure` script (see
`./configure --help` for available options) will set up the build configuration,
and `ninja -C build` (or wherever you put the build directory) will actually run
the build. If you have `qemu-system-x86_64` installed, the `qemu.sh` script will
to run jsix in QEMU `-nographic` mode.

I personally run this either from a real debian amd64 bullseye machine or
a windows WSL debian bullseye installation. Your mileage may vary with other
setups and distros.
