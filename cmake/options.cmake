#
#  settable options for PTLib.
#
#  Can be set on the command line with:
#
#     ... -D<option>=<value>
#
#  Available options are:
#
#     allocator: std, mt, bitmap
#

# check for minimum sizing
option(minsize "Minimise the size of the lib by disabling least used options" OFF)
if(minsize)
  DEFAULT_RESOLVER=no
  DEFAULT_OPENLDAP=no
  DEFAULT_OPENSSL=no
  DEFAULT_EXPAT=no
  DEFAULT_SDL=no
  DEFAULT_GSTREAMER=no
  DEFAULT_SASL=no
  DEFAULT_MLIB=no
  DEFAULT_PCAP=no
  DEFAULT_LUA=no
  DEFAULT_V8=no
  DEFAULT_ODBC=no
  DEFAULT_TTS=no
  DEFAULT_ASN=1
  DEFAULT_NAT=no
  DEFAULT_STUN=no
  DEFAULT_STUNSRVR=no
  DEFAULT_PIPECHAN=no
  DEFAULT_DTMF=no
  DEFAULT_WAVFILE=no
  DEFAULT_SOCKS=no
  DEFAULT_FTP=no
  DEFAULT_SNMP=no
  DEFAULT_TELNET=no
  DEFAULT_CLI=no
  DEFAULT_REMCONN=no
  DEFAULT_SERIAL=no
  DEFAULT_POP3SMTP=no
  DEFAULT_VIDEO=no
  DEFAULT_URL=no
  DEFAULT_HTTP=no
  DEFAULT_HTTPFORMS=no
  DEFAULT_HTTPSVC=no
  DEFAULT_SSDP=no
  DEFAULT_VCARD=no
  DEFAULT_SOAP=no
  DEFAULT_XMLRPC=no
  DEFAULT_JABBER=no
  DEFAULT_CONFIG_FILE=no
  DEFAULT_QOS=no
  DEFAULT_VIDFILE=no
  DEFAULT_FFVDEV=no
  DEFAULT_VXML=no
  DEFAULT_AWS_SDK=yes
fi

DEFAULT_APPSHARE=no
DEFAULT_SHM_VIDEO=no
DEFAULT_SHM_AUDIO=no
DEFAULT_PORTAUDIO=no
DEFAULT_VFW_CAPTURE=no
DEFAULT_SAMPLES=no
DEFAULT_PLUGINS=yes
DEFAULT_EXCEPTIONS=yes
MEMORY_CHECKER=None

#----------------------------------------------------------------------------------------------------------------------
# compiler and language options
#----------------------------------------------------------------------------------------------------------------------

# force PINDEX to be size_t
option(pindex_is_size_t "enable PINDEX set to size_t" ON)
if(pindex_is_size_t)
  set(P_PINDEX_IS_SIZE_T 1)
endif()

# disable C++ exceptions by default
option(exceptions "enable C++ exceptions" OFF)
if (exceptions)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(ptlib PRIVATE "/EHsc")
  else()
    target_compile_options(ptlib PRIVATE "-fexceptions")
  endif()
else()
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(ptlib PRIVATE "/EHa")
  else()
    target_compile_options(ptlib PRIVATE "-fno-exceptions")
  endif()
endif()

# set default memory checker to none
set(MEMORY_CHECKER "None")

# address sanitizer (GCC/clang only)
option(sanitize-address "enable GCC/clang Address Sanitizer" OFF)
if (sanitize-address)
  check_cxx_source_compiles("
    #include <sanitizer/asan_interface.h>
    int main() {
      return 0;
    }" has_asan)
  if (has_asan)
    target_compile_options(ptlib PRIVATE "-fsanitize=address -fno-omit-frame-pointer")  
    set(HAS_ADDRESS_SANITIZER 1)
    set(MEMORY_CHECKER "GCC AddressSanitizer")
  else()
    set(HAS_ADDRESS_SANITIZER 0)
  endif()
endif()

# memory leak checking (disabled if address sanitizer is enabled)
if(NOT HAS_ADDRESS_SANITIZER)
  option(memcheck "enable leak testing code" OFF)
  if(memcheck)
    set(PMEMORY_CHECK 1)
    set(MEMORY_CHECKER "Internal")
  endif()
endif()

# C++ memory allocator (set with -DPTLIB_ALLOCATOR_TYPE=std,mt,bitmap)
if(NOT DEFINED PTLIB_ALLOCATOR_TYPE)
  set(PTLIB_ALLOCATOR_TYPE "std")
endif()
if (PTLIB_ALLOCATOR_TYPE STREQUAL "std")
elseif (PTLIB_ALLOCATOR_TYPE STREQUAL "mt")
  check_cxx_source_compiles("#include <ext/mt_allocator.h>
    int main(int, char**) {
      typedef __gnu_cxx::__mt_alloc<char> TestAllocator;
      return 0;
    }" allocatorMt)
  if (allocatorMt)
    set(P_GNU_ALLOCATOR 1)
  else()
    message(FATAL_ERROR "mt allocator not found")
  endif()
elseif (PTLIB_ALLOCATOR_TYPE STREQUAL "bitmap")
  check_cxx_source_compiles("#include <ext/mt_allocator.h>
    int main(int, char**) {
      typedef __gnu_cxx::__mt_alloc<char> VariableAllocator;
      typedef __gnu_cxx::bitmap_allocator<char> FixedAllocator;
      return 0;
    }" allocatorBitmap)
  if (allocatorBitmap)
    set(P_GNU_ALLOCATOR 2)
  else()
    message(FATAL_ERROR "bitmap allocator not found")
  endif()
else()
  message(FATAL_ERROR "Unknown allocator type ${PTLIB_ALLOCATOR_TYPE}")
endif()

# force pindex to be size_t
option(pindex_is_size_t "enable PINDEX set to size_t" ON)
if(pindex_is_size_t)
  set(P_PINDEX_IS_SIZE_T 1)
endif()

# disable use of pthread_kill for checking on thread terminated
option(no_pthread_kill "disable use of pthread_kill for checking on thread terminated" OFF)
if(no_pthread_kill)
  set(P_NO_PTHREAD_KILL 1)
endif()


#----------------------------------------------------------------------------------------------------------------------
# PTLib functionality options applicable to all platforms
#----------------------------------------------------------------------------------------------------------------------

# enable VarType support by default
option(P_VARTYPE_opt "VarType support" ON)
if(P_VARTYPE_opt)
  set(P_VARTYPE 1)
endif()


#----------------------------------------------------------------------------------------------------------------------
# enable ODBC support if VARTYPE is available
#----------------------------------------------------------------------------------------------------------------------

cmake_dependent_option(P_ODBC_opt "ODBC support" TRUE "P_VARTYPE_opt" OFF)
if(NOT P_ODBC_opt)
  message("-- ODBC support disabled by option")
  unset(P_ODBC)
else()
  if(P_WINDOWS)  
    # check for windows ODBC
    check_cxx_source_compiles("#include <sql.h> 
      int main(int, char**) {
        SQLFreeHandle(0, 0);
        return 0;
      }" has_sql_h)
    if (NOT has_sql_h)
      message("-- ODBC support not found")
    else()  
      set(P_ODBC 1)
      message("-- ODBC support found")
      check_cxx_source_compiles("#include <sqltypes.h>
        int main(int, char**) {
          BYTE b;
        }" odbc_defines_windows_types)
      if (odbc_defines_windows_types)
        set(P_ODBC_DEFINES_WINDOWS_TYPES 1)
      endif()
    endif()
  else () 
    # check for unix ODBC
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
# enable IPv6 support if available
#----------------------------------------------------------------------------------------------------------------------
option(ipv6 "enable IPv6 support" ON)
if (NOT ipv6  )
  message("-- IPv6 support disabled by option")
else()
  check_cxx_source_compiles("#include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netdb.h>
    int main(int, char**) {
      struct sockaddr_in6 s;
      s.sin6_scope_id = 0;
      getnameinfo(0, 0, 0, 0, 0, 0, 0);
    }" has_ipv6)
  if (has_ipv6)
    set(P_HAS_IPV6 1)
  endif()
fi

#----------------------------------------------------------------------------------------------------------------------
# enable stack back trace support
#----------------------------------------------------------------------------------------------------------------------

option(backtrace "enable stack back trace support" ON)
if (NOT backtrace)
  message("-- Stack back trace support disabled by option")
else()
  check_cxx_source_compiles("#include <execinfo.h>
    int main(int, char**) {
      void* addrlist[100];
      int addrlen = backtrace(addrlist, sizeof(addrlist)/sizeof(void*));
      char** symbollist = backtrace_symbols(addrlist, addrlen);
    }" has_backtrace)
  if (has_backtrace)
    set(P_HAS_BACKTRACE 1)
    message("-- Stack back trace support enabled")

      check_cxx_source_compiles("#include <cxxabi.h>
        int main(int, char**) {
          char name[100];
          size_t size;
          int status;
          abi::__cxa_demangle("something", name, &size, &status);
        }" has_cxxabi)
      if (has_cxxabi)
        set(P_HAS_DEMANGLE 1)
        message("-- C++ demangle support enabled")
      endif()
    endif()
  endif()
endif()


#----------------------------------------------------------------------------------------------------------------------
# check for tracing
#----------------------------------------------------------------------------------------------------------------------

option(tracing "enable tracing" ON)
if (NOT tracing)
  message("-- PTRACE and all trace logging removed")
else()
  option(trace-instance "enable object instance in trace logging" ON)
  if (NOT trace-instance)
    message("-- PTRACE without object instance trace logging included")
    target_compile_options(ptlib PRIVATE "-DPTRACING=1")  
  else()
    message("-- PTRACE and all trace logging included")
    target_compile_options(ptlib PRIVATE "-DPTRACING=2")
  endif()
endif()

