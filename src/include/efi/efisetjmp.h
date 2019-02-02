#ifndef GNU_EFI_SETJMP_H
#define GNU_EFI_SETJMP_H

#include <efi/eficompiler.h>
#include <efi/efisetjmp_arch.h>

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#if ! __has_builtin(setjmp)
extern UINTN setjmp(jmp_buf *env) __attribute__((returns_twice));
#endif

#if ! __has_builtin(longjmp)
extern VOID longjmp(jmp_buf *env, UINTN value) __attribute__((noreturn));
#endif

#endif /* GNU_EFI_SETJMP_H */
