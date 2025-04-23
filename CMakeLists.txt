cmake_minimum_required(VERSION 3.14)
project(ptlib
    VERSION 3.0.0
    DESCRIPTION "Reduced Opal"
    LANGUAGES CXX)

#----------------------------------------------------------------------------------------------------------------------
# general settings and options
#----------------------------------------------------------------------------------------------------------------------

include(cmake/utils.cmake)
include(GNUInstallDirs)
include(CheckIncludeFileCXX)

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

#----------------------------------------------------------------------------------------------------------------------
# extract ptlib version
#----------------------------------------------------------------------------------------------------------------------

file(READ "version.h" ptver)

# extract ptlib major version
string(REGEX MATCH " MAJOR_VERSION ([0-9]*)" PTLIB_MAJOR ${ptver})
set(PTLIB_MAJOR ${CMAKE_MATCH_1})

# extract ptlib minor version
string(REGEX MATCH " MINOR_VERSION ([0-9]*)" _ ${ptver})
set(PTLIB_MINOR ${CMAKE_MATCH_1})

# extract ptlib patch version
string(REGEX MATCH " PATCH_VERSION ([0-9]*)" _ ${ptver})
set(PTLIB_PATCH ${CMAKE_MATCH_1})

# extract ptlib OEM version
string(REGEX MATCH " OEM_VERSION ([0-9]*)" _ ${ptver})
set(PTLIB_OEM ${CMAKE_MATCH_1})

message("ptlib version: ${PTLIB_MAJOR}.${PTLIB_MINOR}.${PTLIB_PATCH}")

# set ptlib version string
set(PTLIB_VERSION "${PTLIB_MAJOR}.${PTLIB_MINOR}.${PTLIB_PATCH}")

#----------------------------------------------------------------------------------------------------------------------
# extract OS version
#----------------------------------------------------------------------------------------------------------------------

message("system: ${CMAKE_SYSTEM_NAME}")

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(P_LINUX 1)
endif ()

#----------------------------------------------------------------------------------------------------------------------
# create ptlib_config.h file
#----------------------------------------------------------------------------------------------------------------------

check_include_file_cxx("stdint.h" HAVE_STDINT_H)
if (HAVE_STDINT_H)
  set(HAVE_STDINT_H 1)
else()
  check_include_file_cxx("inttypes.h" HAVE_INTTYPES_H)
  if (HAVE_INTTYPES_H)
    set(HAVE_INTTYPES_H 1)
  endif()
endif()


# create ptlib_config.h file
configure_file(include/ptlib_config.h.in include/ptlib_config.h)

#----------------------------------------------------------------------------------------------------------------------
# ptlib dependencies
#----------------------------------------------------------------------------------------------------------------------

# Search for your dependencies here

#----------------------------------------------------------------------------------------------------------------------
# ptlib sources
#----------------------------------------------------------------------------------------------------------------------

include(GenerateExportHeader)
set(export_file_name "export_shared.h")

if(NOT BUILD_SHARED_LIBS)
    set(export_file_name "export_static.h")
endif()

generate_export_header(ptlib EXPORT_FILE_NAME include/ptlib/${export_file_name})

set(sources
    src/ptlib/common/object.cxx
    )
#    include/ptlib/ptlib/object.h)
#    include/ptlib/ptlib.h
#    src/ptlib.cpp)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})

#----------------------------------------------------------------------------------------------------------------------
# ptlib target
#----------------------------------------------------------------------------------------------------------------------

include(CMakePackageConfigHelpers)

target_sources(ptlib PRIVATE ${sources})
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
