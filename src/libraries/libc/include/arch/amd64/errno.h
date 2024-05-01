#pragma once
/// \file arch/amd64/errno.h
/// errno implementation for amd64

#ifdef __cplusplus
extern "C" {
#endif

int * __errno_location();
#define errno (*__errno_location())

#ifdef __cplusplus
} // extern C
#endif
