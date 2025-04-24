#----------------------------------------------------------------------------------------------------------------------
# ptlib dependencies
#----------------------------------------------------------------------------------------------------------------------

if (P_EXCEPTIONS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
endif()

if (P_PLUGINMGR)
  set(P_PLUGINS 1)
endif()

# get endianness
test_big_endian(bigEndian)
if (bigEndian)
  set(PBYTE_ORDER PBIG_ENDIAN)
else()
  set(PBYTE_ORDER PLITTLE_ENDIAN)
endif()
message("-- endian: ${PBYTE_ORDER}")

# set size of types
check_type_size(int SIZEOF_INT)

check_type_size("unsigned long long" ulongLongSize)
if (ulongLongSize) 
  set(HAVE_UNSIGNED_LONG_LONG_INT 1)
endif()

check_type_size("long long" longLongSize)
if (longLongSize) 
  set(HAVE_LONG_LONG_INT 1)
endif()

check_type_size("long double" longDoubleSize)
if (longDoubleSize) 
  set(HAVE_LONG_DOUBLE 1)
endif()

# set wchar support
check_include_file_cxx("wchar.h" P_HAS_WCHAR)
if (P_HAS_WCHAR)
  set(P_HAS_WOSTREAM 1)
  check_include_file("iconv.h" P_HAS_ICONV)
endif()

# set system headers
check_include_file_cxx("unistd.h"     HAVE_UNISTD_H)
check_include_file_cxx("alloca.h"     HAVE_ALLOCA_H)
check_include_file_cxx("sys/types.h"  HAVE_SYS_TYPES_H)
check_include_file_cxx("sys/ndir.h"   HAVE_SYS_NDIR_H)
check_include_file_cxx("sys/dir.h"    HAVE_SYS_DIR_H)
check_include_file_cxx("ndir.h"       HAVE_NDIR_H)
check_include_file_cxx("sys/stat.h"   HAVE_SYS_STAT_H)

check_include_file_cxx("stdlib.h"     HAVE_STDLIB_H)
check_include_file_cxx("stddef.h"     HAVE_STDDEF_H)
if (HAVE_STDLIB_H AND HAVE_STDDEF_H)
  set(STDC_HEADERS 1)
elseif(HAVE_STDLIB_H)
  set(HAVE_STDLIB_H 1)
endif()

check_include_file_cxx("string.h"           HAVE_STRING_H)
check_include_file_cxx("strings.h"          HAVE_STRINGS_H)
check_include_file_cxx("signal.h"           HAVE_SIGNAL_H)
check_include_file_cxx("fcntl.h"            HAVE_FCNTL_H)
check_include_file_cxx("termios.h"          HAVE_TERMIOS_H)
check_include_file_cxx("sys/termios.h"      HAVE_SYS_TERMIOS_H)
check_include_file_cxx("ioctl.h"            HAVE_IOCTL_H)
check_include_file_cxx("sys/ioctl.h"        HAVE_SYS_IOCTL_H)
check_include_file_cxx("sys/time.h"         HAVE_SYS_TIME_H)
check_include_file_cxx("sys/socket.h"       HAVE_SYS_SOCKET_H)
check_include_file_cxx("netdb.h"            HAVE_NETDB_H)
check_include_file_cxx("netinet/in.h"       HAVE_NETINET_IN_H)
check_include_file_cxx("netinet/tcp.h"      HAVE_NETINET_TCP_H)
check_include_file_cxx("arpa/inet.h"        HAVE_ARPA_INET_H)
check_include_file_cxx("net/if.h"           HAVE_NET_IF_H)
check_include_file_cxx("ifaddrs.h"          HAVE_IFADDRS_H)
check_include_file_cxx("sys/sockio.h"       HAVE_SYS_SOCKIO_H)
check_include_file_cxx("sys/sysctl.h"       HAVE_SYS_SYSCTL_H)
check_include_file_cxx("linux/errqueue.h"   HAVE_LINUX_ERRQUEUE_H)
check_include_file_cxx("net/if_dl.h"        HAVE_NET_IF_DL_H)
check_include_file_cxx("net/if_types.h"     HAVE_NET_IF_TYPES_H)
check_include_file_cxx("netinet/if_ether.h" HAVE_NETINET_IF_ETHER_H)
check_include_file_cxx("semaphore.h"        HAVE_SEMAPHORE_H)
check_include_file_cxx("poll.h"             HAVE_POLL_H)

CHECK_SYMBOL_EXISTS(recvmsg             "sys/types.h;sys/socket.h;netinet/in.h" HAVE_RECVMSG)
CHECK_SYMBOL_EXISTS(MSG_ERRQUEUE        "sys/types.h;sys/socket.h;netinet/in.h" HAVE_RECVMSG_MSG_ERRQUEUE)
CHECK_SYMBOL_EXISTS(IP_RECVERR          "sys/types.h;sys/socket.h;netinet/in.h" HAVE_RECVMSG_IP_RECVERR)
CHECK_SYMBOL_EXISTS(dlopen              "dlfcn.h"                               P_DYNALINK)

# check that rt_msghdr is defined for FreeBSD
CHECK_SYMBOL_EXISTS(rt_msghdr      "net/route.h"          P_HAS_RT_MSGHDR)

# check that setpgrp() takes none or two arguments
check_c_source_compiles("#include <unistd.h>
   int main(int, char**) {
     pid_t p = setpgrp();
   }" setpgrp_no_arg)
if (setpgrp_no_arg)
  set(P_SETPGRP_NO_ARG 1)
endif()

# check that AF_NETLINK is defined
CHECK_SYMBOL_EXISTS(AF_NETLINK   "asm/types.h;sys/socket.h;linux/netlink.h;linux/rtnetlink.h;linux/genetlink.h" P_HAS_NETLINK)

# check pthread mutex
check_c_source_compiles("#include <pthread.h>
   int main(int, char**) {
     pthread_mutexattr_t attr;
     pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
     return 0;
   }" has_PTHREAD_MUTEX_RECURSIVE_NP) 
if (has_PTHREAD_MUTEX_RECURSIVE_NP)
  set(P_HAS_PTHREAD_MUTEX 1)
else()
  check_c_source_compiles("#include <pthread.h>
     int main(int, char**) {
       pthread_mutexattr_t attr;
       pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
       return 0;
     }" has_PTHREAD_MUTEX_RECURSIVE)
  if (has_PTHREAD_MUTEX_RECURSIVE)
    set(P_HAS_PTHREAD_MUTEX 2)
  endif()
endif()

# check for arguments to getgrgid_r
check_cxx_source_compiles("#include <sys/types.h>
  #include <sys/types.h>
  #include <grp.h>
   int main(int, char**) {
     gid_t gid;
     struct group * grp;
     char * buffer;
     size_t bufsize; 
     getgrgid_r(gid, grp, buffer, bufsize);
   }" getgrgid_r_4_args)
if (getgrgid_r_4_args)
  set(P_GETGRGID_R4 1)
else()
  check_cxx_source_compiles("#include <sys/types.h>
    #include <grp.h>
    int main(int, char**) {
      gid_t gid;
      struct group * grp;
      char * buffer;
      size_t bufsize; 
      struct group *result;
      getgrgid_r(gid, grp, buffer, bufsize, &result);
   }" getgrgid_r_5_args)
  if (getgrgid_r_5_args)
    set(P_GETGRGID_R5 1)
  endif()
endif()

# check for arguments to getgrnam_r
check_cxx_source_compiles("#include <sys/types.h>
  #include <grp.h>
  int main(int, char**) {
    const char * name;
    struct group * grp;
    char * buffer;
    size_t bufsize; 
    getgrnam_r(name, grp, buffer, bufsize);
  }" getgrnam_r_4_args)
if (getgrnam_r_4_args)
  set(P_GETGRNAM_R4 1)
else()
  check_cxx_source_compiles("#include <sys/types.h>
    #include <grp.h>
    int main(int, char**) {
      const char * name;
      struct group * grp;
      char * buffer;
      size_t bufsize; 
      struct group *result;
      getgrnam_r(name, grp, buffer, bufsize, &result);
   }" getgrnam_r_5_args)
  if (getgrnam_r_5_args)
    set(P_GETGRNAM_R5 1)
  endif()
endif()


# check that std::ios::set_rdbuf is available
check_cxx_source_compiles("#include <iostream>
      struct MyStream : public std::iostream {
        MyStream()
          : std::iostream(NULL)
        {
          set_rdbuf(NULL);
        }
      };
      int main(int, char**) {
        MyStream strm;
        return 0;
      }" set_rdbuf_exists)

if (set_rdbuf_exists)
  set(P_HAS_SET_RDBUF 1)
endif()

if (PTLIB_ALLOCATOR_TYPE STREQUAL "std")
elseif (PTLIB_ALLOCATOR_TYPE STREQUAL "mt")
  check_cxx_source_compiles("#include <ext/mt_allocator.h>
    int main(int, char**) {
      typedef __gnu_cxx::__mt_alloc<char> TestAllocator;
      return 0;
    }" allocatorMt)
elseif (PTLIB_ALLOCATOR_TYPE STREQUAL "bitmap")
  check_cxx_source_compiles("#include <ext/mt_allocator.h>
    int main(int, char**) {
      typedef __gnu_cxx::__mt_alloc<char> VariableAllocator;
      typedef __gnu_cxx::bitmap_allocator<char> FixedAllocator;
      return 0;
    }" allocatorBitmap)
else()
  message(FATAL_ERROR "Unknown allocator type ${PTLIB_ALLOCATOR_TYPE}")
endif()

# set the default allocator type
if (NOT DEFINED PTLIB_ALLOCATOR_TYPE)
  set(PTLIB_ALLOCATOR_TYPE "std")
endif()

if (PTLIB_ALLOCATOR_TYPE STREQUAL "std")
elseif (PTLIB_ALLOCATOR_TYPE STREQUAL "mt")
  set(P_GNU_ALLOCATOR 1)
elseif (PTLIB_ALLOCATOR_TYPE STREQUAL "bitmap")
  set(P_GNU_ALLOCATOR 2)
endif()

# check for odbc
if (P_ODBC)  # if enabled by user
  set(CMAKE_REQUIRED_LIBRARIES "odbc")
  check_c_source_compiles("#include <sql.h>
   int main(int, char**) {
     SQLFreeHandle(0, 0);
     return 0;
   }" have_SQLFreeHandle)
  if (NOT have_SQLFreeHandle)
    message(FATAL_ERROR "ODBC support is enabled but the odbc library is not found")
  else()
    check_c_source_compiles("#include <sqltypes.h>  
      BYTE b;
      int main(int, char**) {
        return 0;
      }" have_BYTE)
    if (have_BYTE)
      set(P_ODBC_DEFINES_WINDOWS_TYPES 1)
    endif()
  endif()
endif()


# platform specific configuration options

if(P_WINDOWS)
  #TODO: check for curses. Set P_CURSES to 2 if found
else()
  check_include_file_cxx("pthread.h" P_PTHREADS)
  check_include_file_cxx("dirent.h" HAVE_DIRENT_H)

  # check for ncurses
  if (P_CURSES)
    check_include_file_cxx("ncurses.h" HAVE_NCURSES_H)
    if (HAVE_NCURSES_H)
      set(P_CURSES 1)
    endif()
  endif()
endif()