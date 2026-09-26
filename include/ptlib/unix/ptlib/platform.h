/*
 * platform.h
 *
 * Unix machine dependencies
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 */

#ifndef PTLIB_PLATFORM_H
#define PTLIB_PLATFORM_H

#ifdef HAVE_STDINT_H
  #include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
  #include <inttypes.h>
#endif

#ifdef HAVE_LIMITS_H
  #include <limits.h>
#endif

#ifdef HAVE_SYS_TYPES_H
  #include <sys/types.h>
#endif

#ifdef HAVE_DIRENT_H
  #include <dirent.h>
  #define NAMLEN(dirent) strlen ((dirent)->d_name)
#else
  #define dirent direct
  #define NAMLEN(dirent) ((dirent)->d_namlen)
  #ifdef HAVE_SYS_NDIR_H
    #include <sys/ndir.h>
  #endif
  #ifdef HAVE_SYS_DIR_H
    #include <sys/dir.h>
  #endif
  #ifdef HAVE_NDIR_H
    #include <ndir.h>
  #endif
#endif

#ifdef HAVE_SYS_STAT_H
  #include <sys/stat.h>
#endif

#ifdef STDC_HEADERS
  #include <stdlib.h>
  #include <stddef.h>
#else
  #ifdef HAVE_STDLIB_H
    #include <stdlib.h>
  #endif
#endif

#ifdef HAVE_STRING_H
  #if !defined STDC_HEADERS && defined HAVE_MEMORY_H
     #include <memory.h>
  #endif
  #include <string.h>
#endif

#ifdef HAVE_STRINGS_H
  #include <strings.h>
#endif

#ifdef HAVE_UNISTD_H
  #include <unistd.h>
#endif

#ifdef HAVE_ALLOCA_H
  #include <alloca.h>
#endif

#ifdef HAVE_SIGNAL_H
  #include <signal.h>
#else
  #include <sys/signal.h>
#endif

#ifdef HAVE_FCNTL_H
  #include <fcntl.h>
#else
  #include <sys/fcntl.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_NETDB_H
  #include <netdb.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

#if P_HAS_POLL
#include <poll.h>
#endif

///////////////////////////////////////////////////////////////////////////////
#if defined(P_LINUX)

#define HAS_IFREQ

#if __GNU_LIBRARY__ < 6
  typedef int socklen_t;
#endif

///////////////////////////////////////////////////////////////////////////////
#elif defined(P_FREEBSD) || defined(P_OPENBSD) || defined(P_NETBSD)

#define HAS_IFREQ

///////////////////////////////////////////////////////////////////////////////
#elif defined(P_ANDROID)

#define P_THREAD_SAFE_LIBC
#define P_NO_CANCEL

///////////////////////////////////////////////////////////////////////////////
#elif defined (P_MACOSX) || defined(P_IOS)

#include "Availability.h"

#if defined(P_IOS)
#define P_IPHONEOS P_IOS // For backward compatibility
#endif

#define HAS_IFREQ
#define SIGRTMAX NSIG


///////////////////////////////////////////////////////////////////////////////
#elif defined(P_CYGWIN)

///////////////////////////////////////////////////////////////////////////////

// Other operating systems here

#else

#error No operating system selected.

#endif

///////////////////////////////////////////////////////////////////////////////

// includes common to all Unix variants

#ifdef P_HAS_AIO
  #include <aio.h>
#endif

typedef int SOCKET;

#ifndef PSETPGRP
  #if P_SETPGRP_NOPARM
    #define PSETPGRP()  setpgrp()
  #else
    #define PSETPGRP()  setpgrp(0, 0)
  #endif
#endif

typedef pid_t PProcessIdentifier;
typedef void (*PRunTimeSignalHandler)(int, siginfo_t *, void *);

#ifdef P_PTHREADS
  #ifndef _THREAD_SAFE
    #define _THREAD_SAFE 1
  #endif

  #include <pthread.h>
  typedef pthread_t PThreadIdentifier;
  #define PNullThreadIdentifier ((PThreadIdentifier)-1)

  #ifdef P_LINUX
    typedef pid_t PUniqueThreadIdentifier;
    #define P_UNIQUE_THREAD_ID_FMT "%u"
    #define P_THREAD_ID_FMT "0x%lx"
  #elif defined(P_MACOSX)
    typedef uint64_t PUniqueThreadIdentifier;
    #define P_UNIQUE_THREAD_ID_FMT "%llu"
    #define P_THREAD_ID_FMT "%p"
  #else
    typedef pthread_t PUniqueThreadIdentifier;
    #define P_UNIQUE_THREAD_ID_FMT "%u"
    #define P_THREAD_ID_FMT "%u"
  #endif

  #if defined(P_HAS_SEMAPHORES) || defined(P_HAS_NAMED_SEMAPHORES)
    #include <semaphore.h>
  #endif  // P_HAS_SEMPAHORES

#else

typedef pid_t PThreadIdentifier;
#define PNullThreadIdentifier ((PThreadIdentifier)-1)
#define P_THREAD_ID_FMT "%u"
typedef pid_t PUniqueThreadIdentifier;

#endif

#ifdef _DEBUG
  __inline void PBreakToDebugger() { kill(getpid(), SIGABRT); }
#else
  __inline void PBreakToDebugger() { }
#endif


///////////////////////////////////////////
//
//  define a macro for declaring classes so we can bolt
//  extra things to class declarations
//

#define PEXPORT
#define PSTATIC


///////////////////////////////////////////
//
// define some basic types and their limits
//

typedef  int16_t  PInt16; // 16 bit
typedef uint16_t PUInt16; // 16 bit
typedef  int32_t  PInt32; // 32 bit
typedef uint32_t PUInt32; // 32 bit
typedef  int64_t  PInt64; // 64 bit
typedef uint64_t PUInt64; // 64 bit

// Integer type that is same size as a pointer type.
typedef intptr_t      INT;

// Create "Windows" style definitions.

#if P_ODBC_DEFINES_WINDOWS_TYPES
  #define  ODBCINT64  int64_t
  #define UODBCINT64 uint64_t
  #include <sqltypes.h>
#else
  typedef void            VOID;
  typedef uint8_t         BYTE;
  typedef uint16_t        WORD;
  typedef uint32_t        DWORD;

  typedef char            CHAR;
  typedef signed char     SCHAR;
  typedef unsigned char   UCHAR;
  typedef short           SHORT;
  typedef signed short    SSHORT;
  typedef unsigned short  USHORT;
  typedef long            LONG;
  typedef signed long     SLONG;
  typedef unsigned long   ULONG;

  typedef float           SFLOAT;
  typedef double          SDOUBLE;
  typedef double          LDOUBLE;

  typedef void *          PTR;
  typedef void *          LPVOID;
  typedef CHAR *          LPSTR;

  typedef const CHAR *    LPCSTR;
  typedef DWORD *         LPDWORD;
  #define FAR

  typedef signed short    RETCODE;
  typedef void *          HWND;

  #ifdef P_HAS_WCHAR
    typedef wchar_t       WCHAR;
    typedef WCHAR *       LPWSTR;
    typedef const WCHAR * LPCWSTR;
  #endif

#endif // P_ODBC_DEFINES_WINDOWS_TYPES


#ifndef INT64_MAX
#define INT64_MAX	std::numeric_limits<PInt64>::max()
#endif

#ifndef UINT64_MAX
#define UINT64_MAX	std::numeric_limits<PUInt64>::max()
#endif


///////////////////////////////////////////
// Type used for array indexes and sizes

#if P_PINDEX_IS_SIZE_T
  typedef size_t PINDEX;
  #define PINDEX_SIGNED 0
#else
  typedef int PINDEX;
  #define PINDEX_SIGNED 1
#endif

#endif // PTLIB_PLATFORM_H

#if __GLIBC__
  #include <byteswap.h>
#endif

#if P_HAS_RDTSC
  #include <x86intrin.h>
#endif

enum { PGAIErrorFlag = 0x40000000 };

// End of file
