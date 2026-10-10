# Optional dependencies can come from vcpkg. Point CMake at a vcpkg checkout:
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
# Default manifest features are openssl, expat, sdl2, and jpeg. Other features
# are listed in vcpkg.json.
#
# The default target is Release. Other targets:
#   cmake --build build --target debug   # Debug tree in build/debug
#   cmake --build build --target both     # Release and Debug
cmake_minimum_required(VERSION 3.20)

# A machine-wide "vcpkg integrate install" notices vcpkg.json and, with
# manifest mode off, prints a warning on every MSBuild invocation. CMake
# selects the libraries, so the generated projects do not use that integration.
list(APPEND CMAKE_VS_GLOBALS "VcpkgEnabled=false")

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/version.h" _ptlib_version_h)
string(REGEX MATCH "#define MAJOR_VERSION ([0-9]+)" _ "${_ptlib_version_h}")
set(PTLIB_VERSION_MAJOR "${CMAKE_MATCH_1}")
string(REGEX MATCH "#define MINOR_VERSION ([0-9]+)" _ "${_ptlib_version_h}")
set(PTLIB_VERSION_MINOR "${CMAKE_MATCH_1}")
string(REGEX MATCH "#define PATCH_VERSION ([0-9]+)" _ "${_ptlib_version_h}")
set(PTLIB_VERSION_PATCH "${CMAKE_MATCH_1}")
string(REGEX MATCH "#define OEM_VERSION ([0-9]+)" _ "${_ptlib_version_h}")
set(PTLIB_VERSION_OEM "${CMAKE_MATCH_1}")
string(REGEX MATCH "#define BUILD_TYPE ([A-Za-z]+)" _ "${_ptlib_version_h}")
set(_ptlib_build_type "${CMAKE_MATCH_1}")

set(PTLIB_VERSION "${PTLIB_VERSION_MAJOR}.${PTLIB_VERSION_MINOR}.${PTLIB_VERSION_PATCH}")
if(_ptlib_build_type STREQUAL "AlphaCode")
  string(APPEND PTLIB_VERSION "-alpha")
elseif(_ptlib_build_type STREQUAL "BetaCode")
  string(APPEND PTLIB_VERSION "-beta")
endif()

# configure --enable-minsize. Set before project() so a vcpkg manifest does
# not install the default features that this switch turns off. jpeg stays,
# matching configure.ac, which does not disable it.
if(PTLIB_MINSIZE)
  set(VCPKG_MANIFEST_NO_DEFAULT_FEATURES ON)
  if(NOT VCPKG_MANIFEST_FEATURES)
    set(VCPKG_MANIFEST_FEATURES jpeg)
  endif()
endif()
if(PTLIB_OPALMIN)
  set(_ptlib_vcpkg_features "${VCPKG_MANIFEST_FEATURES}")
  if(NOT _ptlib_vcpkg_features AND DEFINED ENV{VCPKG_MANIFEST_FEATURES})
    set(_ptlib_vcpkg_features "$ENV{VCPKG_MANIFEST_FEATURES}")
  endif()
  string(REPLACE "," ";" _ptlib_vcpkg_features "${_ptlib_vcpkg_features}")
  list(REMOVE_ITEM _ptlib_vcpkg_features ldap sasl)
  set(VCPKG_MANIFEST_FEATURES "${_ptlib_vcpkg_features}")
  unset(_ptlib_vcpkg_features)
endif()

project(PTLib
  VERSION "${PTLIB_VERSION_MAJOR}.${PTLIB_VERSION_MINOR}.${PTLIB_VERSION_PATCH}"
  DESCRIPTION "Portable Tools Library"
  LANGUAGES C CXX)

# Single-config generators (Makefiles, Ninja) have no build type until one is
# chosen. Release stays the default target; debug and both are added below.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
    "Build type for the default target. The debug and both targets build Debug separately."
    FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    Release Debug RelWithDebInfo MinSizeRel)
endif()
if(CMAKE_GENERATOR STREQUAL "Ninja Multi-Config" AND NOT CMAKE_DEFAULT_BUILD_TYPE)
  set(CMAKE_DEFAULT_BUILD_TYPE Release CACHE STRING
    "Configuration built by default for the Ninja Multi-Config generator." FORCE)
endif()

if(APPLE)
  enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT DEFINED BUILD_SHARED_LIBS)
  set(BUILD_SHARED_LIBS ON)
endif()

# make/my_macros.m4 links MinGW and Cygwin DLLs with -shared -Wl,--kill-at.
# -shared exports every global symbol. --kill-at strips the stdcall @N suffix
# so the import library matches the undecorated names in the headers.
# CMake already passes -shared. MSVC exports nothing unless a symbol is
# marked dllexport or listed in a module definition, and then it writes no
# import library. WINDOWS_EXPORT_ALL_SYMBOLS is that same export-all link.
if(MINGW OR CYGWIN)
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--kill-at")
  string(APPEND CMAKE_MODULE_LINKER_FLAGS " -Wl,--kill-at")
elseif(MSVC)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

set(PTLIB_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(cmake/PTLibConfigHeader.cmake)

find_package(Threads REQUIRED)

include(cmake/PTLibPlatform.cmake)
include(cmake/PTLibOptions.cmake)

set(PTLIB_FESTIVAL_ENABLED FALSE)
if(PTLIB_TEXT_TO_SPEECH AND NOT PTLIB_PLATFORM_SOURCES STREQUAL "msos")
  check_include_files("festival/festival.h" PTLIB_HAVE_FESTIVAL_H)
  if(PTLIB_HAVE_FESTIVAL_H)
    set(PTLIB_FESTIVAL_ENABLED TRUE)
    ptlib_define(P_FESTIVAL 1)
  endif()
endif()

find_package(BISON QUIET)
if(BISON_FOUND AND BISON_VERSION VERSION_GREATER_EQUAL "3.0")
  bison_target(PTLibGetdate
    "${PTLIB_SOURCE_DIR}/src/ptlib/common/getdate.y"
    "${CMAKE_CURRENT_BINARY_DIR}/getdate.c")
  set(PTLIB_GETDATE_SOURCE "${BISON_PTLibGetdate_OUTPUT_SOURCE}")
else()
  set(PTLIB_GETDATE_SOURCE "${PTLIB_SOURCE_DIR}/src/ptlib/common/getdate.tab.c")
endif()

find_package(Git QUIET)
set(PTLIB_GIT_COMMIT "")
if(GIT_FOUND)
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
    WORKING_DIRECTORY "${PTLIB_SOURCE_DIR}"
    OUTPUT_VARIABLE PTLIB_GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
endif()
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/include/revision.h"
"/* Generated by CMake. */
#ifndef _PTLib_REVISION_H
#define _PTLib_REVISION_H
#define SVN_REVISION 0
#define GIT_COMMIT \"${PTLIB_GIT_COMMIT}\"
#endif
")

# The config header's MSVC include_alias paths belong to the Visual Studio
# projects. This build passes headers and libraries on the command line.
ptlib_define(P_CMAKE 1)

ptlib_write_config_header(
  "${PTLIB_SOURCE_DIR}/include/ptlib_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/ptlib_config.h")

include(cmake/PTLibSources.cmake)
ptlib_collect_sources(PTLIB_SOURCES)

add_library(ptlib ${PTLIB_SOURCES})
add_library(PTLib::ptlib ALIAS ptlib)
if(BUILD_SHARED_LIBS AND MSVC)
  target_compile_definitions(ptlib PRIVATE PTLIB_EXPORTS)
  target_compile_definitions(ptlib INTERFACE PTLIB_SHARED)
endif()
# Unix: libpt, debug libpt_d, static libpt_s / libpt_d_s.
# 64-bit MSVC matches the Visual Studio projects: ptlib64 / ptlib64d, and
# ptlib64s / ptlib64sd for the static library, so both configurations can
# share one directory. Other Windows toolchains keep pt.
set(_ptlib_output pt)
set(_ptlib_output_debug pt)
if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 8)
  if(BUILD_SHARED_LIBS)
    set(_ptlib_output ptlib64)
    set(_ptlib_output_debug ptlib64d)
  else()
    set(_ptlib_output ptlib64s)
    set(_ptlib_output_debug ptlib64sd)
  endif()
elseif(NOT WIN32)
  if(BUILD_SHARED_LIBS)
    set(_ptlib_output_debug pt_d)
  else()
    set(_ptlib_output pt_s)
    set(_ptlib_output_debug pt_d_s)
  endif()
endif()
set_target_properties(ptlib PROPERTIES
  OUTPUT_NAME "${_ptlib_output}"
  OUTPUT_NAME_DEBUG "${_ptlib_output_debug}"
  VERSION "${PROJECT_VERSION}"
  SOVERSION "${PTLIB_VERSION_MAJOR}"
  POSITION_INDEPENDENT_CODE ON)

target_include_directories(ptlib
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
    "$<BUILD_INTERFACE:${PTLIB_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

target_compile_definitions(ptlib
  PUBLIC
    "PTRACING=${PTLIB_PTRACING}"
  PRIVATE
    _REENTRANT)

if(NOT PTLIB_EXCEPTIONS)
  if(MSVC)
    target_compile_options(ptlib PRIVATE /EHs-c-)
  else()
    target_compile_options(ptlib PRIVATE -fno-exceptions)
  endif()
endif()
# MSVC leaves __cplusplus defined as 199711L unless this switch is set, so a
# generated solution would ignore the C++20 standard above. ptlib/atomic.h uses
# the macro to decide whether std::atomic<std::shared_ptr<T>> already exists.
# PUBLIC puts the switch on this target and on every project that links it.
if(MSVC)
  target_compile_options(ptlib PUBLIC $<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>)
endif()

if(NOT PTLIB_USE_SYSTEM_REGEX)
  set_source_files_properties(
    "${PTLIB_SOURCE_DIR}/src/ptlib/common/regex/regcomp.c"
    "${PTLIB_SOURCE_DIR}/src/ptlib/common/regex/regexec.c"
    "${PTLIB_SOURCE_DIR}/src/ptlib/common/regex/regerror.c"
    "${PTLIB_SOURCE_DIR}/src/ptlib/common/regex/regfree.c"
    PROPERTIES COMPILE_DEFINITIONS POSIX_MISTAKE)
  target_include_directories(ptlib PRIVATE
    "${PTLIB_SOURCE_DIR}/src/ptlib/common/regex")
endif()

target_link_libraries(ptlib PUBLIC Threads::Threads)
if(CMAKE_DL_LIBS)
  target_link_libraries(ptlib PUBLIC ${CMAKE_DL_LIBS})
endif()
if(PTLIB_ICONV_LINK)
  target_link_libraries(ptlib PUBLIC ${PTLIB_ICONV_LINK})
endif()
if(PTLIB_LINK_LIBRARIES)
  target_link_libraries(ptlib PUBLIC ${PTLIB_LINK_LIBRARIES})
endif()

if(PTLIB_IS_APPLE)
  target_link_libraries(ptlib PUBLIC
    "-framework AudioToolbox"
    "-framework CoreAudio"
    "-framework AudioUnit"
    "-framework CoreServices"
    "-framework SystemConfiguration"
    "-framework Foundation"
    "-framework AVFoundation"
    "-framework CoreMedia"
    "-framework CoreVideo"
    "-framework Cocoa"
    objc)
endif()

if(WIN32)
  # MinGW ignores #pragma comment(lib), so the Windows import libraries
  # those pragmas name are listed here.
  # mpr: WNetAddConnection. psapi: GetProcessMemoryInfo.
  # crypt32: certificate store. vfw32: AVIFile.
  # winmm: wave/mixer/mmio. dnsapi: DnsQuery.
  target_link_libraries(ptlib PUBLIC
    ws2_32 iphlpapi wer mpr psapi crypt32 vfw32 winmm dnsapi)
endif()

add_subdirectory(plugins)
if(PTLIB_BUILD_SAMPLES)
  add_subdirectory(samples)
endif()

set(_ptlib_include_os "${PTLIB_PLATFORM_SOURCES}")
install(TARGETS ptlib
  EXPORT PTLibTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
if(TARGET PTLib_ffmpeg)
  install(TARGETS PTLib_ffmpeg EXPORT PTLibTargets)
endif()
install(FILES
  "${PTLIB_SOURCE_DIR}/include/ptlib.h"
  "${PTLIB_SOURCE_DIR}/include/ptlib_wx.h"
  "${CMAKE_CURRENT_BINARY_DIR}/include/ptlib_config.h"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(DIRECTORY "${PTLIB_SOURCE_DIR}/include/ptlib/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ptlib"
  FILES_MATCHING PATTERN "*.h" PATTERN "*.inl"
  PATTERN "unix/*" EXCLUDE
  PATTERN "msos/*" EXCLUDE)
install(DIRECTORY "${PTLIB_SOURCE_DIR}/include/ptlib/${_ptlib_include_os}/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ptlib/${_ptlib_include_os}")
install(DIRECTORY "${PTLIB_SOURCE_DIR}/include/ptclib/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ptclib"
  FILES_MATCHING PATTERN "*.h" PATTERN "*.inl")

file(READ "${PTLIB_SOURCE_DIR}/ptlib.pc.in" _ptlib_pc)
string(REGEX REPLACE "\r?\nmakedir=@makedir@\r?\n" "\n" _ptlib_pc "${_ptlib_pc}")
string(REPLACE "@PTLIB_VERSION@" "${PTLIB_VERSION}" _ptlib_pc "${_ptlib_pc}")
string(REPLACE "@prefix@" "${CMAKE_INSTALL_PREFIX}" _ptlib_pc "${_ptlib_pc}")
string(REPLACE "@exec_prefix@" "${CMAKE_INSTALL_PREFIX}" _ptlib_pc "${_ptlib_pc}")
string(REPLACE "@libdir@" "${CMAKE_INSTALL_FULL_LIBDIR}" _ptlib_pc "${_ptlib_pc}")
string(REPLACE "@includedir@" "${CMAKE_INSTALL_FULL_INCLUDEDIR}" _ptlib_pc "${_ptlib_pc}")
string(REPLACE "@datarootdir@" "${CMAKE_INSTALL_FULL_DATAROOTDIR}" _ptlib_pc "${_ptlib_pc}")
string(REPLACE "@target_cpu@" "${CMAKE_SYSTEM_PROCESSOR}" _ptlib_pc "${_ptlib_pc}")
string(REPLACE "@target_os@" "${CMAKE_SYSTEM_NAME}" _ptlib_pc "${_ptlib_pc}")
string(REPLACE "@target@" "${CMAKE_SYSTEM_NAME}_${CMAKE_SYSTEM_PROCESSOR}" _ptlib_pc "${_ptlib_pc}")
string(REPLACE "@CPPFLAGS@" "" _ptlib_pc "${_ptlib_pc}")
string(REPLACE "@CFLAGS@" "" _ptlib_pc "${_ptlib_pc}")
string(REPLACE "@CXXFLAGS@" "" _ptlib_pc "${_ptlib_pc}")
string(REPLACE "@LDFLAGS@" "" _ptlib_pc "${_ptlib_pc}")
string(REPLACE "@LIBS@" "" _ptlib_pc "${_ptlib_pc}")
string(REPLACE "@CPLUSPLUS_STD@" "-std=c++20" _ptlib_pc "${_ptlib_pc}")
set(_ptlib_pc_lib "${_ptlib_output}")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(_ptlib_pc_lib "${_ptlib_output_debug}")
endif()
string(REPLACE "-lpt\${suffix}" "-l${_ptlib_pc_lib}" _ptlib_pc "${_ptlib_pc}")

macro(ptlib_pc_set name enabled)
  set(_pc_value no)
  if(${enabled})
    set(_pc_value yes)
  endif()
  string(REPLACE "@${name}@" "${_pc_value}" _ptlib_pc "${_ptlib_pc}")
endmacro()

ptlib_pc_set(PTLIB_IPV6 PTLIB_IPV6)
ptlib_pc_set(PTLIB_DNS_RESOLVER PTLIB_RESOLVER_ENABLED)
ptlib_pc_set(PTLIB_PCAP PTLIB_PCAP_ENABLED)
ptlib_pc_set(PTLIB_OPENSSL PTLIB_SSL_ENABLED)
ptlib_pc_set(PTLIB_OPENLDAP PTLIB_OPENLDAP_ENABLED)
ptlib_pc_set(PTLIB_SASL PTLIB_SASL_ENABLED)
ptlib_pc_set(PTLIB_SASL2 PTLIB_SASL_ENABLED)
ptlib_pc_set(PTLIB_EXPAT PTLIB_EXPAT_ENABLED)
ptlib_pc_set(PTLIB_REGEX PTLIB_USE_SYSTEM_REGEX)
ptlib_pc_set(PTLIB_SDL PTLIB_SDL_ENABLED)
ptlib_pc_set(PTLIB_TTS PTLIB_TEXT_TO_SPEECH)
ptlib_pc_set(PTLIB_ASN PTLIB_ASN)
ptlib_pc_set(PTLIB_STUN PTLIB_STUN)
ptlib_pc_set(PTLIB_PIPECHAN PTLIB_PIPECHAN)
ptlib_pc_set(PTLIB_DTMF PTLIB_DTMF)
ptlib_pc_set(PTLIB_WAVFILE PTLIB_WAVFILE)
ptlib_pc_set(PTLIB_SOCKS PTLIB_SOCKS)
ptlib_pc_set(PTLIB_FTP PTLIB_FTP)
ptlib_pc_set(PTLIB_SNMP PTLIB_SNMP)
ptlib_pc_set(PTLIB_TELNET PTLIB_TELNET)
ptlib_pc_set(PTLIB_CLI PTLIB_CLI)
ptlib_pc_set(PTLIB_REMCONN PTLIB_REMCONN)
ptlib_pc_set(PTLIB_SERIAL PTLIB_SERIAL)
ptlib_pc_set(PTLIB_POP3SMTP PTLIB_POP3SMTP)
ptlib_pc_set(PTLIB_AUDIO PTLIB_AUDIO)
ptlib_pc_set(PTLIB_VIDEO PTLIB_VIDEO)
ptlib_pc_set(PTLIB_SHM_VIDEO PTLIB_SHM_VIDEO)
ptlib_pc_set(PTLIB_VXML PTLIB_VXML)
ptlib_pc_set(PTLIB_JABBER PTLIB_JABBER)
ptlib_pc_set(PTLIB_XMLRPC PTLIB_XMLRPC)
ptlib_pc_set(PTLIB_SOAP PTLIB_SOAP)
ptlib_pc_set(PTLIB_URL PTLIB_URL)
ptlib_pc_set(PTLIB_HTTP PTLIB_HTTP)
ptlib_pc_set(PTLIB_HTTPFORMS PTLIB_HTTPFORMS)
ptlib_pc_set(PTLIB_HTTPSVC PTLIB_HTTPSVC)
ptlib_pc_set(PTLIB_CONFIG_FILE PTLIB_CONFIG_FILE)
ptlib_pc_set(PTLIB_VIDFILE PTLIB_VIDFILE)
ptlib_pc_set(PTLIB_ODBC PTLIB_ODBC_ENABLED)
ptlib_pc_set(PTLIB_GSTREAMER PTLIB_GSTREAMER_ENABLED)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/ptlib.pc" "${_ptlib_pc}")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ptlib.pc"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

get_property(_ptlib_dep_calls GLOBAL PROPERTY PTLIB_CMAKE_DEPENDENCY_CALLS)
if(_ptlib_dep_calls)
  list(JOIN _ptlib_dep_calls "\n" PTLIB_CMAKE_DEPENDENCY_CALLS)
else()
  set(PTLIB_CMAKE_DEPENDENCY_CALLS "")
endif()
get_property(_ptlib_snippets GLOBAL PROPERTY PTLIB_CMAKE_EXTRA_SNIPPETS)
if(_ptlib_snippets)
  list(JOIN _ptlib_snippets "\n" PTLIB_CMAKE_EXTRA_SNIPPETS)
  string(PREPEND PTLIB_CMAKE_EXTRA_SNIPPETS "find_package(PkgConfig REQUIRED)\n")
else()
  set(PTLIB_CMAKE_EXTRA_SNIPPETS "")
endif()
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PTLibConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/PTLibConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/PTLib")
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/PTLibConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY SameMajorVersion)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/PTLibConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/PTLibConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/PTLib")
install(EXPORT PTLibTargets
  NAMESPACE PTLib::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/PTLib")
# Build-tree package so a consumer can find_package(PTLib) without installing.
export(EXPORT PTLibTargets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/PTLibTargets.cmake"
  NAMESPACE PTLib::)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  @ONLY)
add_custom_target(uninstall
  COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  COMMENT "Removing the files installed from this build"
  VERBATIM)

include(cmake/PTLibBuildVariants.cmake)
