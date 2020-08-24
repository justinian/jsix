/* PDCLib testing suite <_PDCLIB_test.h>

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

/* -------------------------------------------------------------------------- */
/* Helper macros for test drivers                                             */
/* -------------------------------------------------------------------------- */

#define fprintf j6libc_fprintf
#define stderr j6libc_stderr
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#undef stderr
#undef fprintf

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-library-redeclaration"
struct _IO_FILE;
extern struct _IO_FILE *stderr;
extern int fprintf(struct _IO_FILE *, const char *, ...);
#pragma clang diagnostic pop

/* Host system sys/types.h */
#include <sys/types.h>

/* Some strings used for <string.h> and <stdlib.h> testing. */
static const char abcde[] = "abcde";
static const char abcdx[] = "abcdx";
static const char teststring[] = "1234567890\nABCDEFGHIJKLMNOPQRSTUVWXYZ\nabcdefghijklmnopqrstuvwxyz\n";

/* Temporary file names */
static const char testfile[]="testing/testfile";
static const char testfile1[]="testing/testfile1";
static const char testfile2[]="testing/testfile2";

/* Host system fork(), waitpid(), and _exit() */
pid_t fork(void);
pid_t waitpid(pid_t, int *, int);
void _exit(int);

#define START_SUITE(name) \
	int run_suite_ ##name (void) { \
		int TEST_RESULTS = 0;

#define END_SUITE \
		return TEST_RESULTS; \
	}

#define DECLARE_SUITE(name)  extern int run_suite_ ##name (void)
#define RUN_TEST(name)  TEST_RESULTS += test__ ##name();

#define START_TEST(name) \
	int test__ ##name (void) { \
		int TEST_RESULTS = 0;

#define END_TEST \
		return TEST_RESULTS; \
	}

/* TESTCASE() - generic test */
#define TESTCASE( x ) \
	do { \
		pid_t pid = fork(); \
		if ( !pid ) _exit((x) ? 0 : 0xFF); \
        else { \
			int __rc = 0; \
			waitpid(pid, &__rc, 0); \
			if ( __rc & 0xff00 ) { \
				TEST_RESULTS += 1; \
				fprintf( stderr, "FAILED: " __FILE__ ":%s, line %d - %s\n", __func__, __LINE__, #x ); \
			} \
		} \
	} while(0)

/* TESTCASE_REQUIRE() - must-pass test; return early otherwise */
#define TESTCASE_REQUIRE( x ) \
	do { \
		pid_t pid = fork(); \
		if ( !pid ) _exit((x) ? 0 : 0xFF); \
        else { \
			int __rc = 0; \
			waitpid(pid, &__rc, 0); \
			if ( __rc & 0xff00 ) { \
				TEST_RESULTS += 1; \
				fprintf( stderr, "FAILED: " __FILE__ ":%s, line %d - %s\n", __func__, __LINE__, #x ); \
				return TEST_RESULTS; \
			} \
		} \
	} while(0)
