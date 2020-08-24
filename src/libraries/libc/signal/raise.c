/* raise( int )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

extern void (*_PDCLIB_sigabrt)( int );
extern void (*_PDCLIB_sigfpe)( int );
extern void (*_PDCLIB_sigill)( int );
extern void (*_PDCLIB_sigint)( int );
extern void (*_PDCLIB_sigsegv)( int );
extern void (*_PDCLIB_sigterm)( int );

int raise( int sig )
{
    void (*sighandler)( int ) = NULL;
    const char * message;
    switch ( sig )
    {
        case SIGABRT:
            sighandler = _PDCLIB_sigabrt;
            message = "Abnormal termination (SIGABRT)";
            break;
        case SIGFPE:
            sighandler = _PDCLIB_sigfpe;
            message = "Arithmetic exception (SIGFPE)";
            break;
        case SIGILL:
            sighandler = _PDCLIB_sigill;
            message = "Illegal instruction (SIGILL)";
            break;
        case SIGINT:
            sighandler = _PDCLIB_sigint;
            message = "Interactive attention signal (SIGINT)";
            break;
        case SIGSEGV:
            sighandler = _PDCLIB_sigsegv;
            message = "Invalid memory access (SIGSEGV)";
            break;
        case SIGTERM:
            sighandler = _PDCLIB_sigterm;
            message = "Termination request (SIGTERM)";
            break;
        default:
            fprintf( stderr, "Unknown signal #%d\n", sig );
            _Exit( EXIT_FAILURE );
    }
    if ( sighandler == SIG_DFL )
    {
        fputs( message, stderr );
        _Exit( EXIT_FAILURE );
    }
    else if ( sighandler != SIG_IGN )
    {
        sighandler = signal( sig, SIG_DFL );
        sighandler( sig );
    }
    return 0;
}
