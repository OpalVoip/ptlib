cmake_minimum_required(VERSION 3.14)
project(ptlib
    VERSION 3.0.0
    DESCRIPTION "Opal VoIP Library"
    LANGUAGES C CXX)

#----------------------------------------------------------------------------------------------------------------------
# general settings and options
#----------------------------------------------------------------------------------------------------------------------

include(cmake/utils.cmake)
include(GNUInstallDirs)
include(TestBigEndian)
include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckCSourceCompiles)
include(CheckIncludeFileCXX)
include(CheckCXXSymbolExists)
include(CheckCXXSourceCompiles)
include(CheckTypeSize)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" is_top_level)

# PTLIB_SHARED_LIBS option (undefined by default) can be used to force shared/static build
option(PTLIB_BUILD_TESTS "Build ptlib tests" OFF)
option(PTLIB_BUILD_EXAMPLES "Build ptlib examples" OFF)
option(PTLIB_BUILD_DOCS "Build ptlib documentation" OFF)
option(PTLIB_INSTALL "Generate target for installing ptlib" ${is_top_level})
set_if_undefined(PTLIB_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/ptlib" CACHE STRING
    "Install path for ptlib package-related CMake files")

if(DEFINED PTLIB_SHARED_LIBS)
    set(BUILD_SHARED_LIBS ${PTLIB_SHARED_LIBS})
endif()

if(NOT DEFINED CMAKE_BUILD_TYPE AND NOT DEFINED CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set_if_undefined(CMAKE_CXX_VISIBILITY_PRESET hidden)
set_if_undefined(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

add_library(ptlib) # initialized below
add_library(ptlib::ptlib ALIAS ptlib)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#----------------------------------------------------------------------------------------------------------------------
# extract ptlib version
#----------------------------------------------------------------------------------------------------------------------

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/version.h" ptver)

# extract ptlib major version
string(REGEX MATCH " MAJOR_VERSION ([0-9]*)" PTLIB_MAJOR ${ptver})
set(PTLIB_MAJOR ${CMAKE_MATCH_1})

# extract ptlib minor version
string(REGEX MATCH " MINOR_VERSION ([0-9]*)" _ ${ptver})
set(PTLIB_MINOR ${CMAKE_MATCH_1})

# extract ptlib patch version
string(REGEX MATCH " PATCH_VERSION ([0-9]*)" _ ${ptver})
set(PTLIB_PATCH ${CMAKE_MATCH_1})

# extract ptlib build type
string(REGEX MATCH " BUILD_TYPE ([a-zA-Z]*)" _ ${ptver})
set(PTLIB_BUILD_TYPE ${CMAKE_MATCH_1})

# extract ptlib OEM version
string(REGEX MATCH " OEM_VERSION ([0-9]*)" _ ${ptver})
set(PTLIB_OEM ${CMAKE_MATCH_1})

message("ptlib version: ${PTLIB_MAJOR}.${PTLIB_MINOR}.${PTLIB_PATCH}-${PTLIB_BUILD_TYPE}")

# set ptlib version string
set(PTLIB_VERSION "${PTLIB_MAJOR}.${PTLIB_MINOR}.${PTLIB_PATCH}-${PTLIB_BUILD_TYPE}")

set(P_DEFAULT_PLUGIN_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/ptlib-${PTLIB_VERSION}")

#----------------------------------------------------------------------------------------------------------------------
# extract OS version
#----------------------------------------------------------------------------------------------------------------------

message("system: ${CMAKE_SYSTEM_NAME}")

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  execute_process(COMMAND uname -r OUTPUT_VARIABLE LINUX_KERNEL_VERSION)
  message("linux kernel version: ${LINUX_KERNEL_VERSION}")
  set(P_LINUX ${LINUX_KERNEL_VERSION})
elseif (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
  set(P_FREEBSD 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
  set(P_OPENBSD 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
  set(P_NETBSD 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(P_DARWIN 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL "SunOS")
  set(P_SOLARIS 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Android")
  set(P_ANDROID 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(P_WINDOWS 1)
  set(WIN32 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL "MacOSX")
  set(P_MACOSX 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL "iOS")
  set(P_IOS 1)
endif ()

#----------------------------------------------------------------------------------------------------------------------
# ptlib configuration options
#----------------------------------------------------------------------------------------------------------------------

set(P_EXCEPTIONS 1)
set(P_USE_ASSERTS 1)
set(P_PLUGINMGR 1)
set(P_DEFAULT_PLUGIN_DIR "${P_DEFAULT_PLUGIN_DIR}")
set(P_PINDEX_IS_SIZE_T 1)
set(PTLIB_ALLOCATOR_TYPE "std")

# non-platform specific features
#undef PMEMORY_CHECK

set(P_AUDIO 1)
set(P_VIDEO 1)

set(P_TIMERS 1)
set(P_REGEX 1)
set(P_TEXT_TO_SPEECH 1)
set(P_SPEECH_RECOGNITION 1)
set(P_ASN 1)
set(P_PIPECHAN 1)
set(P_DTMF 1)
set(P_WAVFILE 1)
set(P_SOCKS 1)
set(P_FTP 1)
set(P_TELNET 1)
set(P_CLI 1)
set(P_CURSES 1)
set(P_ODBC 1)
set(P_SERIAL 1)
set(P_POP3SMTP 1)
set(P_CONFIG_FILE 1)
set(P_CYPHER 1)
set(P_GUID 1)
set(P_SYSTEMLOG 1)
set(P_CHANNEL_UTILS 1)
set(P_NETWORKING 1)
set(P_NAT 1)
set(P_URL 1)
set(P_MEDIAFILE 1)
set(P_VIDFILE 1)
set(P_PORTAUDIO 1)
set(P_PORTMIXER 1)

# additional feature dependencies
if (P_GUID)
  set(P_SPOOLDIR 1)
endif()

if (_WIN32 OR P_PIPECHAN)
  set(P_REMCONN 1)
endif()

if(P_ASN)
  set(P_SNMP)
endif()

if (P_NAT)
  set(P_STUN)
  if (P_STUN)
    set(P_STUNSRVR)
    if (P_SSL)
      set(P_TURN)
    endif()
  endif()
endif()

if (P_URL)
  set(P_VCARD)
  set(P_HTTP)
  if (P_HTTP)
    if (P_CONFIG_FILE)
      set(P_HTTPFORMS)
      if (P_HTTPFORMS)
        set(P_HTTPSVC)
      endif()
    endif()
    set(P_SSDP)
  endif()
endif()


#set(P_NO_PTHREAD_KILL 0)

#----------------------------------------------------------------------------------------------------------------------
# ptlib dependencies
#----------------------------------------------------------------------------------------------------------------------

include(cmake/dependencies.cmake)
# create ptlib_config.h file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/ptlib_config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/ptlib_config.h)

#----------------------------------------------------------------------------------------------------------------------
# create revision.h file
#----------------------------------------------------------------------------------------------------------------------

find_package(Git REQUIRED)
execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
                OUTPUT_VARIABLE GIT_HASH
                OUTPUT_STRIP_TRAILING_WHITESPACE)
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/revision.h)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/revision.h" gitver)
  string(REGEX MATCH " GIT_COMMIT \"(.*)\"" _ ${gitver})
  set(OLD_GIT_HASH ${CMAKE_MATCH_1})
endif()

if(NOT OLD_GIT_HASH STREQUAL GIT_HASH)
  message("GIT_HASH: ${GIT_HASH}")
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/revision.h.in ${CMAKE_CURRENT_SOURCE_DIR}/revision.h)
endif()

#----------------------------------------------------------------------------------------------------------------------
# ptlib sources
#----------------------------------------------------------------------------------------------------------------------

include(GenerateExportHeader)
set(export_file_name "export_shared.h")

if(NOT BUILD_SHARED_LIBS)
    set(export_file_name "export_static.h")
endif()

generate_export_header(ptlib EXPORT_FILE_NAME include/ptlib/${export_file_name})

set(PTLIB_COMMON_SRC_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/src/ptlib/common")
set(PTLIB_COMPONENT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/ptclib")
if (P_WINDOWS)
  set(PTLIB_PLATFORM_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/ptlib/msos")
else()
  set(PTLIB_PLATFORM_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/ptlib/unix")
endif()

include(cmake/toplevel.cmake)
    
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})

#----------------------------------------------------------------------------------------------------------------------
# ptlib target
#----------------------------------------------------------------------------------------------------------------------

include(CMakePackageConfigHelpers)

#target_sources(ptlib PRIVATE ${sources})
target_compile_definitions(ptlib PUBLIC "$<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:PTLIB_STATIC_DEFINE>")

target_include_directories(ptlib
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/ptlib>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include/ptlib>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/ptlib/ptlib>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/ptlib/ptclib>"
        )

set_target_properties(ptlib PROPERTIES
    SOVERSION ${PROJECT_VERSION_MAJOR}
    VERSION ${PROJECT_VERSION})

if(PTLIB_INSTALL AND NOT CMAKE_SKIP_INSTALL_RULES)
    configure_package_config_file(cmake/ptlib-config.cmake.in ptlib-config.cmake
        INSTALL_DESTINATION "${PTLIB_INSTALL_CMAKEDIR}")

    write_basic_package_version_file(ptlib-config-version.cmake
        COMPATIBILITY SameMajorVersion)

    install(TARGETS ptlib EXPORT ptlib_export
        RUNTIME COMPONENT ptlib
        LIBRARY COMPONENT ptlib NAMELINK_COMPONENT ptlib-dev
        ARCHIVE COMPONENT ptlib-dev
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
    install(DIRECTORY include/
        TYPE INCLUDE
        COMPONENT ptlib-dev)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/ptlib/${export_file_name}"
        COMPONENT ptlib-dev
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ptlib")

    set(targets_file "ptlib-shared-targets.cmake")

    if(NOT BUILD_SHARED_LIBS)
        set(targets_file "ptlib-static-targets.cmake")
    endif()

    install(EXPORT ptlib_export
        COMPONENT ptlib-dev
        FILE "${targets_file}"
        DESTINATION "${PTLIB_INSTALL_CMAKEDIR}"
        NAMESPACE ptlib::)

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/ptlib-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/ptlib-config-version.cmake"
        COMPONENT ptlib-dev
        DESTINATION "${PTLIB_INSTALL_CMAKEDIR}")

    if(MSVC)
        set(pdb_file "")
        set(pdb_file_destination "")

        if(BUILD_SHARED_LIBS)
            set(pdb_file "$<TARGET_PDB_FILE:ptlib>")
            set(pdb_file_destination "${CMAKE_INSTALL_BINDIR}")
        else()
            # TARGET_PDB_FILE does not work for pdb file generated for static library build, determining it manually
            set(pdb_file "$<TARGET_FILE_DIR:ptlib>/$<TARGET_FILE_PREFIX:ptlib>$<TARGET_FILE_BASE_NAME:ptlib>.pdb")
            set(pdb_file_destination "${CMAKE_INSTALL_LIBDIR}")
        endif()

        install(FILES "${pdb_file}"
            COMPONENT ptlib-dev
            CONFIGURATIONS Debug RelWithDebInfo
            DESTINATION "${pdb_file_destination}"
            OPTIONAL)
    endif()
endif()

#----------------------------------------------------------------------------------------------------------------------
# other targets
#----------------------------------------------------------------------------------------------------------------------

if(PTLIB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(PTLIB_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(PTLIB_BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    doxygen_add_docs(docs include)
endif()
