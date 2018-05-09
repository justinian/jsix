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

Popcorn uses the `waf` build tool, which is included in the repo. The other
requirements are:

* python (to run waf)
* gcc
* nasm

After cloning, run `waf configure`. Then you can run `waf build` to build the
project, and `waf test` to run the tests. A floppy disk image will be built in
`build/popcorn.img`. If you have `qemu-system-x86_64` installed, then you can
run `waf qemu` to run it in `-nographic` mode.
