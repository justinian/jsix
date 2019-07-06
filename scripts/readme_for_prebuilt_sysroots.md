# jsix OS sysroot

This is a pre-built sysroot for building the jsix operating system kernel,
bootloader, and utilities. This package is provided as a convenience, and
contains software from the following repositories.

## NASM, the Netwide Assembler

As downloaded from [the NASM website][nasm] under the following license:

    Copyright 1996-2010 the NASM Authors - All rights reserved.
 
     Redistribution and use in source and binary forms, with or without
     modification, are permitted provided that the following
     conditions are met:
 
     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above
       copyright notice, this list of conditions and the following
       disclaimer in the documentation and/or other materials provided
       with the distribution.
       
       THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
       CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
       INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
       MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
       DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
       CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
       SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
       NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
       LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
       HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
       CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
       OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
       EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

[nasm]: https://www.nasm.us

## The LLVM toolchain

The LLVM sources as downloaded via git from [llvm.org][llvm] under the terms of
the [Apache License v2.0][apache2], modified [as described here][llvmlic].

[llvm]: https://llvm.org
[apache2]: https://www.apache.org/licenses/LICENSE-2.0
[llvmlic]: https://llvm.org/docs/DeveloperPolicy.html#new-llvm-project-license-framework
