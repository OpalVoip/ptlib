# Host platform macros. These match the OS tags historically produced by configure.ac.

set(PTLIB_PLATFORM_SOURCES "unix")
set(PTLIB_OS_TAG "")

if(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Android")
  set(PTLIB_OS_TAG "P_ANDROID")
elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
  set(PTLIB_OS_TAG "P_IOS")
elseif(APPLE)
  set(PTLIB_OS_TAG "P_MACOSX")
elseif(CYGWIN)
  set(PTLIB_OS_TAG "P_CYGWIN")
elseif(MINGW)
  set(PTLIB_OS_TAG "P_MINGW")
  set(PTLIB_PLATFORM_SOURCES "msos")
elseif(WIN32)
  set(PTLIB_PLATFORM_SOURCES "msos")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(PTLIB_OS_TAG "P_LINUX")
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
  set(PTLIB_OS_TAG "P_FREEBSD")
elseif(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
  set(PTLIB_OS_TAG "P_OPENBSD")
elseif(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
  set(PTLIB_OS_TAG "P_NETBSD")
else()
  message(FATAL_ERROR "PTLib CMake does not support CMAKE_SYSTEM_NAME='${CMAKE_SYSTEM_NAME}'")
endif()

set(PTLIB_IS_BSD FALSE)
if(PTLIB_OS_TAG MATCHES "^P_(FREEBSD|OPENBSD|NETBSD)$")
  set(PTLIB_IS_BSD TRUE)
endif()

set(PTLIB_IS_APPLE FALSE)
if(PTLIB_OS_TAG STREQUAL "P_MACOSX" OR PTLIB_OS_TAG STREQUAL "P_IOS")
  set(PTLIB_IS_APPLE TRUE)
endif()

# MinGW and Cygwin install plugins next to the process, matching configure.ac.
if(MINGW OR CYGWIN)
  set(PTLIB_PLUGIN_INSTALL_DIR "plugins")
  set(PTLIB_DEFAULT_PLUGIN_DIR "plugins")
else()
  set(PTLIB_PLUGIN_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/ptlib-${PTLIB_VERSION}")
  set(PTLIB_DEFAULT_PLUGIN_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/ptlib-${PTLIB_VERSION}")
endif()

include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckCXXSymbolExists)
include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

function(ptlib_have_header header variable)
  string(MAKE_C_IDENTIFIER "PTLIB_HAVE_${header}" ident)
  check_include_files("${header}" ${ident})
  if(${ident})
    ptlib_define(${variable} 1)
  endif()
endfunction()

if(PTLIB_OS_TAG)
  ptlib_define(${PTLIB_OS_TAG} 1)
endif()

# tools/configure enables the "winver" feature by default (configure.ac:
# WINVER=0x601, Windows 7). platform.h uses Windows 2000 only when this
# macro is left undefined.
if(PTLIB_PLATFORM_SOURCES STREQUAL "msos")
  ptlib_define(WINVER 0x0601)
endif()

ptlib_define(PTLIB_MAJOR ${PTLIB_VERSION_MAJOR})
ptlib_define(PTLIB_MINOR ${PTLIB_VERSION_MINOR})
ptlib_define(PTLIB_PATCH ${PTLIB_VERSION_PATCH})
ptlib_define(PTLIB_OEM ${PTLIB_VERSION_OEM})
ptlib_define(PTLIB_VERSION "\"${PTLIB_VERSION}\"")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  ptlib_define(P_64BIT 1)
endif()

# configure.ac defaults this on. Pointer-sized indexes avoid truncating casts.
ptlib_define(P_PINDEX_IS_SIZE_T 1)

if(CMAKE_CXX_BYTE_ORDER STREQUAL "BIG_ENDIAN")
  ptlib_define(PBYTE_ORDER "PBIG_ENDIAN")
else()
  ptlib_define(PBYTE_ORDER "PLITTLE_ENDIAN")
endif()

# Modern hosts provide the C99 headers these probes exist to find.
foreach(header IN ITEMS
    limits.h unistd.h alloca.h sys/types.h dirent.h sys/stat.h
    stdlib.h string.h strings.h signal.h fcntl.h termios.h
    sys/ioctl.h sys/time.h sys/socket.h netdb.h netinet/in.h
    netinet/tcp.h arpa/inet.h net/if.h stdint.h inttypes.h
    ifaddrs.h poll.h wchar.h dlfcn.h semaphore.h)
  string(TOUPPER "${header}" upper)
  string(REGEX REPLACE "[./]" "_" macro "HAVE_${upper}")
  ptlib_have_header("${header}" "${macro}")
endforeach()

ptlib_define(STDC_HEADERS 1)
ptlib_define(HAVE_LONG_LONG_INT 1)
ptlib_define(HAVE_UNSIGNED_LONG_LONG_INT 1)
ptlib_define(HAVE_LONG_DOUBLE 1)
# sizeof(int) is 4 on every LP64 and LLP64 host PTLib still targets.
ptlib_define(SIZEOF_INT 4)

if(NOT WIN32 OR CYGWIN)
  ptlib_define(P_PTHREADS 1)
endif()

cmake_push_check_state(RESET)
set(CMAKE_REQUIRED_LIBRARIES Threads::Threads)
check_cxx_symbol_exists(sem_init semaphore.h PTLIB_HAS_SEM_INIT)
if(PTLIB_HAS_SEM_INIT)
  ptlib_define(P_HAS_SEMAPHORES 1)
endif()
check_cxx_symbol_exists(sem_open semaphore.h PTLIB_HAS_SEM_OPEN)
if(PTLIB_HAS_SEM_OPEN)
  ptlib_define(P_HAS_NAMED_SEMAPHORES 1)
endif()
check_c_source_compiles("
#include <pthread.h>
int main(void) {
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  return 0;
}" PTLIB_HAS_RECURSIVE_MUTEX)
if(PTLIB_HAS_RECURSIVE_MUTEX)
  # 2 selects PTHREAD_MUTEX_RECURSIVE. 1 is the older _NP spelling.
  ptlib_define(P_HAS_RECURSIVE_MUTEX 2)
endif()
check_c_source_compiles("
#include <unistd.h>
int main(void) { return setpgrp() < 0; }
" PTLIB_SETPGRP_NOPARM)
if(PTLIB_SETPGRP_NOPARM)
  ptlib_define(P_SETPGRP_NOPARM 1)
endif()
check_c_source_compiles("
#define _GNU_SOURCE
#include <sys/types.h>
#include <pwd.h>
int main(void) {
  struct passwd pwd, *result;
  char buf[256];
  return getpwuid_r(0, &pwd, buf, sizeof(buf), &result);
}
" PTLIB_GETPWUID_R5)
if(PTLIB_GETPWUID_R5)
  ptlib_define(P_GETPWUID_R5 1)
endif()
check_c_source_compiles("
#define _GNU_SOURCE
#include <sys/types.h>
#include <pwd.h>
int main(void) {
  struct passwd pwd, *result;
  char buf[256];
  return getpwnam_r(\"root\", &pwd, buf, sizeof(buf), &result);
}
" PTLIB_GETPWNAM_R5)
if(PTLIB_GETPWNAM_R5)
  ptlib_define(P_GETPWNAM_R5 1)
endif()
check_c_source_compiles("
#define _GNU_SOURCE
#include <sys/types.h>
#include <grp.h>
int main(void) {
  struct group grp, *result;
  char buf[256];
  return getgrgid_r(0, &grp, buf, sizeof(buf), &result);
}
" PTLIB_GETGRGID_R5)
if(PTLIB_GETGRGID_R5)
  ptlib_define(P_GETGRGID_R5 1)
endif()
check_c_source_compiles("
#define _GNU_SOURCE
#include <sys/types.h>
#include <grp.h>
int main(void) {
  struct group grp, *result;
  char buf[256];
  return getgrnam_r(\"wheel\", &grp, buf, sizeof(buf), &result);
}
" PTLIB_GETGRNAM_R5)
if(PTLIB_GETGRNAM_R5)
  ptlib_define(P_GETGRNAM_R5 1)
endif()
cmake_pop_check_state()

check_cxx_symbol_exists(poll poll.h PTLIB_HAS_POLL)
if(PTLIB_HAS_POLL)
  ptlib_define(P_HAS_POLL 1)
endif()
check_symbol_exists(recvmsg sys/socket.h PTLIB_HAS_RECVMSG)
if(PTLIB_HAS_RECVMSG)
  ptlib_define(P_HAS_RECVMSG 1)
endif()
check_include_files("netinet/in.h;netinet/ip.h" PTLIB_HAVE_NETINET_IP_H)
check_symbol_exists(inet_ntop arpa/inet.h PTLIB_HAS_INET_NTOP)
if(PTLIB_HAS_INET_NTOP)
  ptlib_define(P_HAS_INET_NTOP 1)
endif()
check_cxx_source_compiles("
#include <netinet/in.h>
int main(void) { struct sockaddr_in6 a; return (int)sizeof(a); }
" PTLIB_HAS_SOCKADDR_IN6)
if(PTLIB_HAS_SOCKADDR_IN6)
  set(PTLIB_HOST_HAS_IPV6 TRUE)
else()
  set(PTLIB_HOST_HAS_IPV6 FALSE)
endif()

check_include_files(execinfo.h PTLIB_HAVE_EXECINFO_H)
if(PTLIB_HAVE_EXECINFO_H)
  check_symbol_exists(backtrace execinfo.h PTLIB_HAS_BACKTRACE_SYM)
endif()
check_include_files(cxxabi.h PTLIB_HAVE_CXXABI_H)

check_cxx_source_compiles("#include <atomic>\nint main(){std::atomic<int> a{0}; return a.load();}" PTLIB_HAS_STD_ATOMIC)
if(PTLIB_HAS_STD_ATOMIC)
  ptlib_define(P_STD_ATOMIC 1)
endif()

check_include_files(wchar.h PTLIB_HAVE_WCHAR_H)
if(PTLIB_HAVE_WCHAR_H)
  ptlib_define(P_HAS_WCHAR 1)
  ptlib_define(P_HAS_WOSTREAM 1)
endif()

if(PTLIB_OS_TAG STREQUAL "P_LINUX")
  check_include_files("sys/socket.h;linux/netlink.h" PTLIB_HAVE_NETLINK)
  if(PTLIB_HAVE_NETLINK)
    ptlib_define(P_HAS_NETLINK 1)
  endif()
  check_include_files(linux/errqueue.h PTLIB_HAVE_LINUX_ERRQUEUE_H)
  if(PTLIB_HAVE_LINUX_ERRQUEUE_H)
    ptlib_define(HAVE_LINUX_ERRQUEUE_H 1)
    ptlib_define(P_HAS_RECVMSG_MSG_ERRQUEUE 1)
    ptlib_define(P_HAS_RECVMSG_IP_RECVERR 1)
  endif()
endif()

check_symbol_exists(dlopen dlfcn.h PTLIB_HAS_DLOPEN)
if(PTLIB_HAS_DLOPEN OR WIN32)
  ptlib_define(P_DYNALINK 1)
endif()

# Windows uses MultiByteToWideChar / WideCharToMultiByte in contain.cxx.
# Do not pull in libiconv there (vcpkg or otherwise).
if(NOT WIN32)
  find_package(Iconv QUIET)
  if(Iconv_FOUND AND TARGET Iconv::Iconv)
    ptlib_define(P_HAS_ICONV 1)
    set(PTLIB_ICONV_LINK Iconv::Iconv)
    set(PTLIB_ICONV_PACKAGE Iconv)
  else()
    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_LIBRARIES iconv)
    check_cxx_source_compiles("
#include <iconv.h>
int main(){ iconv_t cd = iconv_open(\"UTF-8\", \"WCHAR_T\"); return cd == (iconv_t)-1; }
" PTLIB_HAS_ICONV_LIB)
    if(NOT PTLIB_HAS_ICONV_LIB)
      set(CMAKE_REQUIRED_LIBRARIES)
      check_cxx_source_compiles("
#include <iconv.h>
int main(){ iconv_t cd = iconv_open(\"UTF-8\", \"WCHAR_T\"); return cd == (iconv_t)-1; }
" PTLIB_HAS_ICONV_LIBC)
    endif()
    cmake_pop_check_state()
    if(PTLIB_HAS_ICONV_LIB OR PTLIB_HAS_ICONV_LIBC)
      ptlib_define(P_HAS_ICONV 1)
      if(PTLIB_HAS_ICONV_LIB)
        set(PTLIB_ICONV_LINK iconv)
      endif()
    endif()
  endif()
endif()

check_symbol_exists(swab unistd.h PTLIB_HAS_SWAB)
if(PTLIB_HAS_SWAB)
  ptlib_define(USE_SYSTEM_SWAB 1)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|i[3-6]86|AMD64)$")
  check_cxx_source_compiles("
#include <x86intrin.h>
int main(){ return (int)__rdtsc(); }
" PTLIB_HAS_RDTSC)
  if(PTLIB_HAS_RDTSC)
    ptlib_define(P_HAS_RDTSC 1)
  endif()
endif()
