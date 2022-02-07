#pragma once
/** \file stdio.h
  * Input/output
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#include <__j6libc/file.h>
#include <__j6libc/null.h>
#include <__j6libc/restrict.h>
#include <__j6libc/size_t.h>
#include <stdarg.h>

typedef size_t fpos_t;

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

#define BUFSIZ 0

#define EOF (-127)

#define FOPEN_MAX 16
#define FILENAME_MAX 256

#define L_tmpnam FILENAME_MAX
#define TMP_MAX 100

#define SEEK_CUR 0
#define SEEK_END 1
#define SEEK_SET 2

#ifdef __cplusplus
extern "C" {
#endif

// Operations on files
//
int remove(const char *filename);
int rename(const char *old_name, const char *new_name);
FILE * tmpfile(void);
char * tmpnam(char *s);

// File access functions
//
int fclose(FILE *stream);
int fflush(FILE *stream);
FILE * fopen(const char * restrict filename, const char * restrict mode);
FILE * freopen(const char * restrict filename, const char * restrict mode, FILE * restrict stream);
void setbuf(FILE * restrict stream, char * restrict buf);
int setvbuf(FILE * restrict stream, char * restrict buf, int mode, size_t size);

// Formatted input/output functions
//
int printf(const char * restrict format, ...);
int vprintf(const char * restrict format, va_list arg);

int fprintf(FILE * restrict stream, const char * restrict format, ...);
int vfprintf(FILE * restrict stream, const char * restrict format, va_list arg);

int sprintf(char * restrict s, const char * restrict format, ...);
int vsprintf(char * restrict s, const char * restrict format, va_list arg);

int snprintf(char * restrict s, size_t n, const char * restrict format, ...);
int vsnprintf(char * restrict s, size_t n, const char * restrict format, va_list arg);

int scanf(const char * restrict format, ...);
int vscanf(const char * restrict format, va_list arg);

int fscanf(FILE * restrict stream, const char * restrict format, ...);
int vfscanf(FILE * restrict stream, const char * restrict format, va_list arg);

int sscanf(const char * restrict s, const char * restrict format, ...);
int vsscanf(const char * restrict s, const char * restrict format, va_list arg);

// Character input/output functions
//
int fgetc(FILE *stream);
char * fgets(char * restrict s, int n, FILE * restrict stream);
int fputc(char c, FILE *stream);
int fputs(const char * restrict s, FILE * restrict stream);
int getc(FILE *stream);
int getchar(void);
int putc(char c, FILE *stream);
int putchar(char c);
int puts(const char *s);
int ungetc(char c, FILE *stream);

// Direct input/output functions
//
size_t fread(void * restrict ptr, size_t size, size_t nmemb, FILE * restrict stream);
size_t fwrite(const void * restrict ptr, size_t size, size_t nmemb, FILE * restrict stream);

// File positioning functions
//
int fgetpos(FILE * restrict stream, fpos_t * restrict pos);
int fseek(FILE *stream, long int offset, int whence);
int fsetpos(FILE *stream, const fpos_t *pos);
long ftell(FILE *stream);
void rewind(FILE *stream);

// Error-handling functions
//
void clearerr(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
void perror(const char *s);

#ifdef __cplusplus
} // extern "C"
#endif
