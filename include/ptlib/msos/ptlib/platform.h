/*
 * platform.h
 *
 * Portable Tools Library
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
 * The Original Code is Portable Tools Library.
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

 /* Bizarely the powers that be deprecated the C++11 conversion functions
 without any indication of a replacement, not even in the proposed
 C++20 standard. Perhaps another case of "if it isn't perfect, we don't
 want it", that is common in standards bodies. */
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 1


#include <stdint.h>
#include <cstdlib>


#ifdef _MSC_VER

  #pragma warning(disable:4201)  // nonstandard extension: nameless struct/union
  #pragma warning(disable:4250)  // inherits '' via dominance
  #pragma warning(disable:4251)  // disable warning exported structs
  #pragma warning(disable:4324)  // structure was padded due to __declspec(align())
  #pragma warning(disable:4511)  // default copy ctor not generated warning
  #pragma warning(disable:4512)  // default assignment op not generated warning
  #pragma warning(disable:4514)  // unreferenced inline removed
  #pragma warning(disable:4699)  // precompiled headers
  #pragma warning(disable:4710)  // inline not expanded warning
  #pragma warning(disable:4711)  // auto inlining warning
  #pragma warning(disable:4786)  // identifier was truncated to '255' characters in the debug information
  #pragma warning(disable:4097)  // typedef synonym for class
  #pragma warning(disable:4800)  // forcing value to bool 'true' or 'false' (performance warning)
  #pragma warning(disable:26439) // This kind of function may not throw.Declare it 'noexcept' (f.6).

  #if P_64BIT
    #pragma warning(disable:4267)  // possible loss of data -- just too many to fix.
  #endif

#endif // _MSC_VER


///////////////////////////////////////////////////////////////////////////////
// Machine & Compiler dependent declarations

#ifndef WIN32
  #define WIN32  1
#endif

#ifndef _WIN32
  #define _WIN32  1
#endif

#if defined(_WIN64) && !defined(P_64BIT)
#define P_64BIT 1
#endif

// At least Windows 2000, configure.ac will generally uprate this
#ifndef WINVER
  #define WINVER 0x0500
#endif

#if !defined(_WIN32_WINNT)
  #define _WIN32_WINNT WINVER
#endif

#if defined(_WIN32_WINNT) && (_WIN32_WINNT == 0x0500) && P_HAS_IPV6 && !defined(NTDDI_VERSION)
  #define NTDDI_VERSION NTDDI_WIN2KSP1
#endif

#ifndef STRICT
  #define STRICT
#endif

#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

// Remove some stupid defines from Windows headers.
#undef DELETE
#undef min
#undef max


// Declaration for platform independent architectures
#define PBYTE_ORDER PLITTLE_ENDIAN


// Standard array index type (depends on compiler)
// Type used in array indexes especially that required by operator[] functions.
#if defined(_MSC_VER) && !P_64BIT
  #define PINDEX int
  #define PINDEX_SIGNED 1
#else
  typedef size_t PINDEX;
  #define PINDEX_SIGNED 0
#endif


#if _MSC_VER>=1400
  #define strcasecmp(s1,s2) _stricmp(s1,s2)
  #define strncasecmp(s1,s2,n) _strnicmp(s1,s2,n)
#elif defined(_MSC_VER)
  #define strcasecmp(s1,s2) stricmp(s1,s2)
  #define strncasecmp(s1,s2,n) strnicmp(s1,s2,n)
#endif


class PWin32Overlapped : public OVERLAPPED
{
  // Support class for overlapped I/O in Win32.
  public:
    PWin32Overlapped();
    ~PWin32Overlapped();
    void Reset();
};


class PWin32Handle
{
  protected:
    HANDLE m_handle;
  public:
    explicit PWin32Handle(HANDLE h = NULL)
      : m_handle(h)
    { }

    ~PWin32Handle() { Close(); }

    void Close();
    bool IsValid() const { return m_handle != NULL && m_handle != INVALID_HANDLE_VALUE; }
    HANDLE Detach();
    HANDLE * GetPointer();

    PWin32Handle & operator=(HANDLE h);
    operator HANDLE() const { return m_handle; }

    bool Wait(uint32_t timeout) const;
    bool Duplicate(HANDLE h, uint32_t flags = DUPLICATE_SAME_ACCESS, uint32_t access = 0);

#if __cplusplus < 201103L
  private:
    PWin32Handle(const PWin32Handle &) : m_handle() { }
    void operator=(const PWin32Handle &) { }
#else
    PWin32Handle(const PWin32Handle &) = delete;
    PWin32Handle & operator=(const PWin32Handle &) = delete;
#endif
};



enum { PWIN32ErrorFlag = 0x40000000 };

class PString;

class RegistryKey
{
  public:
    enum OpenMode {
      ReadOnly,
      ReadWrite,
      Create
    };
    RegistryKey(const PString & subkey, OpenMode mode);
    ~RegistryKey();

    bool EnumKey(PINDEX idx, PString & str);
    bool EnumValue(PINDEX idx, PString & str);
    bool DeleteKey(const PString & subkey);
    bool DeleteValue(const PString & value);
    bool QueryValue(const PString & value, PString & str);
    bool QueryValue(const PString & value, uint32_t & num, bool boolean);
    bool SetValue(const PString & value, const PString & str);
    bool SetValue(const PString & value, uint32_t num);
  private:
    HKEY m_hKey;
};

#undef Yield

#define P_HAS_UNIQUE_THREAD_ID_FMT 0
typedef UINT     PUniqueThreadIdentifier;
typedef uint32_t PProcessIdentifier;

#ifdef _DEBUG
  __inline void PBreakToDebugger() { ::DebugBreak(); }
#else
  __inline void PBreakToDebugger() { }
#endif


#include <malloc.h>
#include <mmsystem.h>


#ifdef _MSC_VER
  #include <crtdbg.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <io.h>
#include <fcntl.h>
#include <direct.h>
#include <time.h>

#include <signal.h>
typedef void (__cdecl * PRunTimeSignalHandler)(int);
#define SIGRTMAX NSIG

// used by various modules to disable the winsock2 include to avoid header file problems
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#include <winsock2.h> // Version 2 of windows socket
#include <ws2tcpip.h> // winsock2 is not complete, ws2tcpip add some defines such as IP_TOS

typedef int socklen_t;

#ifndef EINPROGRESS
  #define EINPROGRESS             (WSAEINPROGRESS|PWIN32ErrorFlag)
  #define ENOTSOCK                (WSAENOTSOCK|PWIN32ErrorFlag)
  #define EMSGSIZE                (WSAEMSGSIZE|PWIN32ErrorFlag)
  #define EOPNOTSUPP              (WSAEOPNOTSUPP|PWIN32ErrorFlag)
  #define EAFNOSUPPORT            (WSAEAFNOSUPPORT|PWIN32ErrorFlag)
  #define EADDRINUSE              (WSAEADDRINUSE|PWIN32ErrorFlag)
  #define EADDRNOTAVAIL           (WSAEADDRNOTAVAIL|PWIN32ErrorFlag)
  #define ENETDOWN                (WSAENETDOWN|PWIN32ErrorFlag)
  #define ENETUNREACH             (WSAENETUNREACH|PWIN32ErrorFlag)
  #define ENETRESET               (WSAENETRESET|PWIN32ErrorFlag)
  #define ECONNABORTED            (WSAECONNABORTED|PWIN32ErrorFlag)
  #define ECONNRESET              (WSAECONNRESET|PWIN32ErrorFlag)
  #define ENOBUFS                 (WSAENOBUFS|PWIN32ErrorFlag)
  #define EISCONN                 (WSAEISCONN|PWIN32ErrorFlag)
  #define ENOTCONN                (WSAENOTCONN|PWIN32ErrorFlag)
  #define ETIMEDOUT               (WSAETIMEDOUT|PWIN32ErrorFlag)
  #define ECONNREFUSED            (WSAECONNREFUSED|PWIN32ErrorFlag)
  #define EHOSTUNREACH            (WSAEHOSTUNREACH|PWIN32ErrorFlag)
  #define EWOULDBLOCK             (WSAEWOULDBLOCK|PWIN32ErrorFlag)
#endif


#ifndef NETDB_SUCCESS
  #define NETDB_SUCCESS 0
#endif



#endif // PTLIB_PLATFORM_H


// End Of File ///////////////////////////////////////////////////////////////
