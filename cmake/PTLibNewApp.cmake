# Write a CMakeLists.txt for an application that uses PTLib and not OPAL.
#
# Run this from the application directory, the one that will contain
# CMakeLists.txt:
#   cmake -P <ptlib>/cmake/PTLibNewApp.cmake
#
# The project name is that directory's name. With no source list, the
# sources are the .c, .cxx, .cpp, and header files in that directory.
# PTLIB_NEW_APP_SOURCES names subdirectories, such as src or src;include,
# and those trees are searched recursively. PTLib is this source tree.
# A program that also uses OPAL should run opal/cmake/OPALNewApp.cmake instead.
#
# -D options go before -P:
#   -DPTLIB_NEW_APP_NAME=<project>
#   -DPTLIB_NEW_APP_SOURCES=<dir>[;<dir>...]
#   -DPTLIB_NEW_APP_FORCE=ON   replace an existing CMakeLists.txt

if(NOT CMAKE_SCRIPT_MODE_FILE)
  message(FATAL_ERROR
    "Run this with: cmake -P <ptlib>/cmake/PTLibNewApp.cmake")
endif()

# In script mode CMAKE_SOURCE_DIR is the working directory.
set(_dest "${CMAKE_SOURCE_DIR}")
get_filename_component(_ptlib_root "${CMAKE_CURRENT_LIST_DIR}" DIRECTORY)
get_filename_component(_ptlib_root "${_ptlib_root}" ABSOLUTE)
file(TO_CMAKE_PATH "${_dest}" _dest)
file(TO_CMAKE_PATH "${_ptlib_root}" _ptlib_root)

if(EXISTS "${_dest}/cmake/OPALNewApp.cmake" OR EXISTS "${_dest}/cmake/PTLibNewApp.cmake")
  message(FATAL_ERROR
    "Run this from the new application's directory, not from the PTLib or OPAL source tree.")
endif()

if(EXISTS "${_dest}/CMakeLists.txt" AND NOT PTLIB_NEW_APP_FORCE)
  message(FATAL_ERROR
    "${_dest}/CMakeLists.txt already exists.\nRe-run with -DPTLIB_NEW_APP_FORCE=ON to replace it.")
endif()

if(PTLIB_NEW_APP_NAME)
  set(APP_NAME "${PTLIB_NEW_APP_NAME}")
else()
  get_filename_component(APP_NAME "${_dest}" NAME)
endif()
if(NOT APP_NAME MATCHES "^[A-Za-z_][A-Za-z0-9_.+-]*$")
  message(FATAL_ERROR
    "Project name \"${APP_NAME}\" is not a CMake target name.\nPass -DPTLIB_NEW_APP_NAME=<project> before -P.")
endif()

set(_source_dirs "")
if(PTLIB_NEW_APP_SOURCES)
  foreach(_dir IN LISTS PTLIB_NEW_APP_SOURCES)
    if(NOT IS_ABSOLUTE "${_dir}")
      set(_dir "${_dest}/${_dir}")
    endif()
    get_filename_component(_dir "${_dir}" ABSOLUTE)
    file(TO_CMAKE_PATH "${_dir}" _dir)
    if(NOT IS_DIRECTORY "${_dir}")
      message(FATAL_ERROR "Source directory was not found:\n  ${_dir}")
    endif()
    list(APPEND _source_dirs "${_dir}")
  endforeach()
else()
  set(_source_dirs "${_dest}")
endif()

set(_sources "")
foreach(_dir IN LISTS _source_dirs)
  set(_globs
    "${_dir}/*.c" "${_dir}/*.cxx" "${_dir}/*.cpp"
    "${_dir}/*.h" "${_dir}/*.hh" "${_dir}/*.hpp" "${_dir}/*.hxx")
  if(_dir STREQUAL "${_dest}")
    file(GLOB _found ${_globs})
  else()
    file(GLOB_RECURSE _found ${_globs})
  endif()
  list(APPEND _sources ${_found})
endforeach()
if(NOT _sources)
  message(FATAL_ERROR
    "No C or C++ sources were found in:\n  ${_source_dirs}")
endif()

set(_relative "")
foreach(_src IN LISTS _sources)
  file(TO_CMAKE_PATH "${_src}" _src)
  if(_src MATCHES "/(build|CMakeFiles)/")
    continue()
  endif()
  file(RELATIVE_PATH _rel "${_dest}" "${_src}")
  list(APPEND _relative "${_rel}")
endforeach()
if(NOT _relative)
  message(FATAL_ERROR
    "No C or C++ sources were found in:\n  ${_source_dirs}")
endif()
list(SORT _relative)
set(APP_SOURCES "")
foreach(_file IN LISTS _relative)
  if(APP_SOURCES)
    string(APPEND APP_SOURCES "\n")
  endif()
  string(APPEND APP_SOURCES "  ${_file}")
endforeach()
set(_sources "${_relative}")

set(PTLIB_HINT "${_ptlib_root}")

configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/PTLibNewApp.cmake.in"
  "${_dest}/CMakeLists.txt"
  @ONLY
  NEWLINE_STYLE UNIX)

message(STATUS "Wrote ${_dest}/CMakeLists.txt")
message(STATUS "Project: ${APP_NAME}")
message(STATUS "Sources: ${_sources}")
message(STATUS "PTLib: ${PTLIB_HINT}")
