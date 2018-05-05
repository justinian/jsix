/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    efi.h

Abstract:

    Public EFI header files



Revision History

--*/


// Add a predefined macro to detect usage of the library
#ifndef _GNU_EFI
#define _GNU_EFI
#endif

//
// Build flags on input
//  EFI32
//  EFI_DEBUG               - Enable debugging code
//  EFI_NT_EMULATOR         - Building for running under NT
//


#ifndef _EFI_INCLUDE_
#define _EFI_INCLUDE_

#define EFI_FIRMWARE_VENDOR         L"INTEL"
#define EFI_FIRMWARE_MAJOR_REVISION 12
#define EFI_FIRMWARE_MINOR_REVISION 33
#define EFI_FIRMWARE_REVISION ((EFI_FIRMWARE_MAJOR_REVISION <<16) | (EFI_FIRMWARE_MINOR_REVISION))

#include <efi/efibind.h>
#include <efi/eficompiler.h>
#include <efi/efidef.h>
#include <efi/efidevp.h>
#include <efi/efipciio.h>
#include <efi/efiprot.h>
#include <efi/eficon.h>
#include <efi/efiser.h>
#include <efi/efi_nii.h>
#include <efi/efipxebc.h>
#include <efi/efinet.h>
#include <efi/efiapi.h>
#include <efi/efifs.h>
#include <efi/efierr.h>
#include <efi/efiui.h>
#include <efi/efiip.h>
#include <efi/efiudp.h>
#include <efi/efitcp.h>
#include <efi/efipoint.h>
#include <efi/efisetjmp.h>

#endif
