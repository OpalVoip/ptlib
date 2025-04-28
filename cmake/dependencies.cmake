#----------------------------------------------------------------------------------------------------------------------
# look for packages
#----------------------------------------------------------------------------------------------------------------------

pkg_check_modules(LIBSSL openssl QUIET)
if(LIBSSL_FOUND)
  set(P_OPENSSL 1)
  target_link_libraries(ptlib ${LIBSSL_LIBRARIES})
  message("-- openssl found")
else()
  message("-- openssl not found")
endif()

# platform specific configuration options

if(P_WINDOWS)
  #TODO: check for curses. Set P_CURSES to 2 if found
else()
  # check for ncurses
  if (P_CURSES)
    pkg_check_modules(NCURSES ncurses QUIET)
    if(NCURSES_FOUND)
      set(P_CURSES 1)
      target_link_libraries(ptlib ${NCURSES_LIBRARIES})
      message("-- ncurses found")
    else()
      message("-- ncurses not found")
    endif()
  endif()

  # check for odbc
  if (P_ODBC AND P_VARTYPE)
    pkg_check_modules(ODBC odbc QUIET)
    if(ODBC_FOUND)
      set(P_ODBC 1)
      target_link_libraries(ptlib ${ODBC_LIBRARIES})
      message("-- odbc found")
    else()
      message("-- odbc not found")
    endif()
  endif()
endif()

#----------------------------------------------------------------------------------------------------------------------
# ptlib dependencies
#----------------------------------------------------------------------------------------------------------------------

if (P_EXCEPTIONS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
endif()

if (P_PLUGINMGR)
  set(P_PLUGINS 1)
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

if(NOT P_WINDOWS)
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
  check_include_file_cxx("semaphore.h"        P_HAS_SEMAPHORES)
  check_include_file_cxx("poll.h"             P_HAS_POLL)
  check_include_file_cxx("pthread.h"          P_PTHREADS)
  check_include_file_cxx("dirent.h"           HAVE_DIRENT_H)
  check_include_file_cxx("aio.h"              P_HAS_AIO)
  check_include_file_cxx("portaudio.h"        HAS_PORTAUDIO)

  CHECK_SYMBOL_EXISTS(recvmsg             "sys/types.h;sys/socket.h;netinet/in.h" P_HAS_RECVMSG)
  CHECK_SYMBOL_EXISTS(MSG_ERRQUEUE        "sys/types.h;sys/socket.h;netinet/in.h" P_HAS_RECVMSG_MSG_ERRQUEUE)
  CHECK_SYMBOL_EXISTS(IP_RECVERR          "sys/types.h;sys/socket.h;netinet/in.h" P_HAS_RECVMSG_IP_RECVERR)
  CHECK_SYMBOL_EXISTS(dlopen              "dlfcn.h"                               P_DYNALINK)

  # check that rt_msghdr is defined for FreeBSD
  CHECK_SYMBOL_EXISTS(rt_msghdr      "net/route.h"          P_HAS_RT_MSGHDR)

  # check that setpgrp() takes none or two arguments
  check_c_source_compiles("#include <unistd.h>
    int main(int, char**) {
      pid_t p = setpgrp();
    }" setpgrp_no_arg)
  if (setpgrp_no_arg)
    set(P_SETPGRP_NOPARM 1)
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
    set(P_HAS_RECURSIVE_MUTEX 1)
  else()
    check_c_source_compiles("#include <pthread.h>
      int main(int, char**) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        return 0;
      }" has_PTHREAD_MUTEX_RECURSIVE)
    if (has_PTHREAD_MUTEX_RECURSIVE)
      set(P_HAS_RECURSIVE_MUTEX 2)
    endif()
  endif()

  # check for XPG6 compliant pthreads (timed spinlocks)
  check_cxx_source_compiles("#include <pthread.h>
    int main(int, char**) {
      pthread_mutex_timedlock(NULL, NULL);
    }" has_pthread_mutex_timedlock)
  if (has_pthread_mutex_timedlock)
    set(P_PTHREADS_XPG6 1)
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

  # check for number of parms to readdir
  check_cxx_source_compiles("#define _POSIX_PTHREAD_SEMANTICS
    #include <dirent.h>
    int main(int, char**) {
      DIR *s;
      struct dirent * e;
      readdir_r(s, e);
    }" readdir_r_2_args)
  if (readdir_r_2_args)
    set(P_HAS_POSIX_READDIR_R 2)
  else()
    check_cxx_source_compiles("#define _POSIX_PTHREAD_SEMANTICS
      #include <dirent.h>
      int main(int, char**) {
        DIR *s;
        struct dirent * e;
        struct dirent * p;
        readdir_r(s, e, &p);
      }" readdir_r_3_args)
    if (readdir_r_3_args)
      set(P_HAS_POSIX_READDIR_R 3)
    else()
      set(P_HAS_POSIX_READDIR_R 0)
    endif()
  endif()

  # check arguments for getpwuid_r
  check_cxx_source_compiles("#include <sys/types.h>
    #include <pwd.h>
    int main(int, char**) {
      uid_t uid;
      struct passwd * pwd;
      char * buffer;
      size_t bufsize; 
      getpwuid_r(uid, pwd, buffer, bufsize);
    }" getpwuid_r_4_args)
  if (getpwuid_r_4_args)
    set(P_GETPWUID_R4 1)
  else()
    check_cxx_source_compiles("#include <sys/types.h>
      #include <pwd.h>
      int main(int, char**) {
        uid_t uid;
        struct passwd * pwd;
        char * buffer;
        size_t bufsize; 
        struct passwd *result;
        getpwuid_r(uid, pwd, buffer, bufsize, &result);
    }" getpwuid_r_5_args)
    if (getpwuid_r_5_args)
      set(P_GETPWUID_R5 1)
    endif()
  endif()

  # check arguments for getpwnam_r
  check_cxx_source_compiles("#include <sys/types.h>
    #include <pwd.h>
    int main(int, char**) {
      const char * name;
      struct passwd * pwd;
      char * buffer;
      size_t bufsize; 
      getpwnam_r(name, pwd, buffer, bufsize);
    }" getpwnam_r_4_args)
  if (getpwnam_r_4_args)
    set(P_GETPWNAM_R4 1)
  else()
    check_cxx_source_compiles("#include <sys/types.h>
      #include <pwd.h>
      int main(int, char**) {
        const char * name;
        struct passwd * pwd;
        char * buffer;
        size_t bufsize; 
        struct passwd *result;
        getpwnam_r(name, pwd, buffer, bufsize, &result);
    }" getpwnam_r_5_args)
    if (getpwnam_r_5_args)
      set(P_GETPWNAM_R5 1)
    endif()
  endif()

  # check for gethostbyaddr_r
  check_cxx_source_compiles("#include <netdb.h>
    int main(int, char**) {
        const void *addr;
        socklen_t len;
        int type;
        struct hostent *ret;
        char *buf;
        size_t buflen;
        struct hostent **result;
        int *h_errnop;
        gethostbyaddr_r(addr, len, type, ret, buf, buflen, result, h_errnop);
    }" gethostbyaddr_r_exists)
  if (gethostbyaddr_r_exists)
    set(P_GETHOSTBYNAME_R 1)
  endif()

  # check for inet_ntop
  check_cxx_source_compiles("#include <sys/types.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
  ],
    int af;
    void * src;
    char * dst;
    socklen_t len;
    inet_ntop(af, src, dst, len);
    }" inet_ntop_exists)
  if (inet_ntop_exists)
    set(P_HAS_INET_NTOP 1)
  endif()

  # check for IPV6_ADD_MEMBERSHIP
  check_cxx_source_compiles("#include <netinet/in.h>
    int x = IPV6_ADD_MEMBERSHIP;" ipv6_add_membership_exists)
  if (ipv6_add_membership_exists)
    set(P_HAS_IPV6_ADD_MEMBERSHIP 1)
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

  # check for malloc_info
  check_cxx_source_compiles("#include <stdio.h>
    #include <malloc.h>
    int main(int, char**) {
      FILE * out;
      malloc_info(0, out);
    }" malloc_info_exists)
  if (malloc_info_exists)
    set(P_HAS_MALLOC_INFO 1)
  endif()

  # check for Unix semaphores
  check_cxx_source_compiles("#include <semaphore.h>
    int main(int, char**) {
      sem_t *s;
      sem_init(s, 0, 0);
    }" unix_semaphores_exists)
  if (unix_semaphores_exists)
    set(P_HAS_SEMAPHORES 1) 
    # if named semaphores are available
    check_cxx_source_compiles("#include <semaphore.h>
      int main(int, char**) {
        sem_t *s = sem_open(\"test\", O_CREAT);
      }" named_semaphores_exists)
    if (named_semaphores_exists)
      set(P_HAS_NAMED_SEMAPHORES 1)
    endif() 
    # check for sem_timedwait
    check_cxx_source_compiles("#include <semaphore.h>
      int main(int, char**) {
        sem_t *s;
        const struct timespec * t;
        sem_timedwait(s, t);
      }" sem_timedwait_exists)
    if (sem_timedwait_exists)
      set(P_HAS_SEMAPHORES_XPG6 1)
    endif()
  endif()

  # if ODBC is found, check if it defines some char types
  if (P_ODBC)
    check_cxx_source_compiles("#include <sqltypes.h>
      int main(int, char**) {
        BYTE b;
      }" odbc_defines_windows_types)
    if (odbc_defines_windows_types)
      set(P_ODBC_DEFINES_WINDOWS_TYPES 1)
    endif()
  endif()

  # check if portaudio is found
  if(P_AUDIO)
    check_include_file_cxx("portaudio.h" HAS_PORTAUDIO)
    if (HAS_PORTAUDIO)
      message("-- portaudio found")
      set(P_PORTAUDIO 1)

      # check if portmixer is found
      check_include_file_cxx("portmixer.h" HAS_PORTMIXER)
      if (HAS_PORTMIXER)
        set(P_PORTMIXER 1)
        message("-- portmixer found")
      endif()
    else()
      message("-- portaudio not found")
    endif()
  endif()

  # check for pcap
  if(P_PCAP)
    check_include_file_cxx("pcap.h" HAS_PCAP)
    if (HAS_PCAP)
      message("-- pcap found")
      set(P_PCAP 1)
    else()
      message(FATAL_ERROR "pcap not found")
    endif()
  endif()
endif() # NOT P_WINDOWS