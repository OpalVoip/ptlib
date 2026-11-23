# Loaded before project(). On Windows, select a vcpkg toolchain when the
# caller did not pass one, then install every manifest feature whose port
# builds for the Visual Studio triplet and whose library the build probes.

if(CMAKE_HOST_WIN32 AND NOT DEFINED PTLIB_USE_VCPKG)
  set(PTLIB_USE_VCPKG ON CACHE BOOL
    "On Windows, use vcpkg for dependencies when a checkout is found")
endif()

# An unset Visual Studio platform is filled in after the toolchain loads.
# vcpkg then guesses x86-windows, and ports such as krb5 refuse that triplet.
# -A Win32 and an explicit triplet are left alone.
if(CMAKE_HOST_WIN32
    AND CMAKE_GENERATOR MATCHES "^Visual Studio"
    AND NOT VCPKG_TARGET_TRIPLET
    AND NOT DEFINED ENV{VCPKG_TARGET_TRIPLET}
    AND NOT CMAKE_GENERATOR_PLATFORM MATCHES "^[Ww][Ii][Nn]32$")
  if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(ARM64|aarch64)$")
    set(_ptlib_triplet arm64-windows)
  else()
    set(_ptlib_triplet x64-windows)
  endif()
  set(VCPKG_TARGET_TRIPLET "${_ptlib_triplet}" CACHE STRING "vcpkg triplet")
  message(STATUS "PTLib: vcpkg triplet ${VCPKG_TARGET_TRIPLET}")
  unset(_ptlib_triplet)
endif()

# The manifest baseline's ports call vcpkg_host_path_list(REMOVE_DUPLICATES).
# Visual Studio 2022's bundled scripts do not implement that operation, so
# expat fails with "Operation REMOVE_DUPLICATES not recognized."
function(ptlib_vcpkg_root_current root out)
  set(${out} FALSE PARENT_SCOPE)
  set(_list "${root}/scripts/cmake/vcpkg_host_path_list.cmake")
  if(NOT EXISTS "${_list}")
    return()
  endif()
  file(READ "${_list}" _text)
  if(_text MATCHES "REMOVE_DUPLICATES")
    set(${out} TRUE PARENT_SCOPE)
  endif()
endfunction()

function(ptlib_find_vcpkg out)
  set(_roots "")
  if(NOT "$ENV{VCPKG_ROOT}" STREQUAL "")
    list(APPEND _roots "$ENV{VCPKG_ROOT}")
  endif()
  if(NOT "$ENV{VCPKG_INSTALLATION_ROOT}" STREQUAL "")
    list(APPEND _roots "$ENV{VCPKG_INSTALLATION_ROOT}")
  endif()
  find_program(PTLIB_VCPKG_EXECUTABLE NAMES vcpkg vcpkg.exe)
  if(PTLIB_VCPKG_EXECUTABLE)
    get_filename_component(_root "${PTLIB_VCPKG_EXECUTABLE}" DIRECTORY)
    list(APPEND _roots "${_root}")
  endif()
  set(_vswhere "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/Installer/vswhere.exe")
  if(NOT EXISTS "${_vswhere}")
    set(_vswhere "$ENV{ProgramFiles}/Microsoft Visual Studio/Installer/vswhere.exe")
  endif()
  if(EXISTS "${_vswhere}")
    # Newest Visual Studio first. Require the C++ tools, not the Vcpkg
    # component: the 2022 component is older than the manifest baseline.
    execute_process(
      COMMAND "${_vswhere}" -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64
              -sort -property installationPath
      OUTPUT_VARIABLE _vs_list
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET)
    string(REGEX REPLACE "\r?\n" ";" _vs_list "${_vs_list}")
    foreach(_vs IN LISTS _vs_list)
      string(STRIP "${_vs}" _vs)
      if(_vs)
        list(APPEND _roots "${_vs}/VC/vcpkg")
      endif()
    endforeach()
  endif()
  list(APPEND _roots
    "$ENV{USERPROFILE}/vcpkg"
    "$ENV{USERPROFILE}/source/vcpkg"
    "$ENV{LOCALAPPDATA}/vcpkg"
    "C:/vcpkg"
    "C:/src/vcpkg")

  set(_found "")
  set(_stale "")
  foreach(_root IN LISTS _roots)
    if(NOT _root OR NOT EXISTS "${_root}/scripts/buildsystems/vcpkg.cmake")
      continue()
    endif()
    file(TO_CMAKE_PATH "${_root}" _root)
    ptlib_vcpkg_root_current("${_root}" _current)
    if(_current)
      set(_found "${_root}/scripts/buildsystems/vcpkg.cmake")
      break()
    elseif(NOT _stale)
      set(_stale "${_root}")
    endif()
  endforeach()
  if(_found AND _stale)
    message(STATUS "PTLib: skipping vcpkg at ${_stale}; its scripts are older than the manifest baseline")
  elseif(NOT _found AND _stale)
    message(FATAL_ERROR
      "vcpkg at ${_stale} is older than the manifest baseline in vcpkg.json.\n"
      "Its scripts do not implement REMOVE_DUPLICATES, so building expat stops with "
      "\"Operation REMOVE_DUPLICATES not recognized.\"\n"
      "Clone https://github.com/microsoft/vcpkg, bootstrap it, and pass "
      "-DCMAKE_TOOLCHAIN_FILE=<clone>/scripts/buildsystems/vcpkg.cmake. "
      "The Visual Studio 2022 generator can stay; its bundled vcpkg cannot.")
  endif()
  set(${out} "${_found}" PARENT_SCOPE)
endfunction()

# Prefer a current checkout. A cached or VCPKG_ROOT path that points at the
# Visual Studio 2022 bundled scripts still fails on expat, so replace it.
if(CMAKE_HOST_WIN32 AND PTLIB_USE_VCPKG)
  set(_ptlib_toolchain_ok FALSE)
  if(CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg\\.cmake$")
    get_filename_component(_ptlib_vcpkg_root "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)
    get_filename_component(_ptlib_vcpkg_root "${_ptlib_vcpkg_root}" DIRECTORY)
    get_filename_component(_ptlib_vcpkg_root "${_ptlib_vcpkg_root}" DIRECTORY)
    ptlib_vcpkg_root_current("${_ptlib_vcpkg_root}" _ptlib_toolchain_ok)
    if(NOT _ptlib_toolchain_ok)
      message(STATUS
        "PTLib: vcpkg at ${_ptlib_vcpkg_root} is older than the manifest baseline; looking for a newer checkout")
    endif()
  endif()
  if(NOT _ptlib_toolchain_ok)
    ptlib_find_vcpkg(_ptlib_vcpkg)
    if(_ptlib_vcpkg)
      set(CMAKE_TOOLCHAIN_FILE "${_ptlib_vcpkg}" CACHE FILEPATH "vcpkg toolchain" FORCE)
      message(STATUS "PTLib: using vcpkg toolchain ${_ptlib_vcpkg}")
    elseif(CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg\\.cmake$")
      message(FATAL_ERROR
        "vcpkg at ${_ptlib_vcpkg_root} is older than the manifest baseline in vcpkg.json.\n"
        "Its scripts do not implement REMOVE_DUPLICATES, so building expat stops with "
        "\"Operation REMOVE_DUPLICATES not recognized.\"\n"
        "Clone https://github.com/microsoft/vcpkg into C:\\src\\vcpkg (or set VCPKG_ROOT), "
        "run bootstrap-vcpkg.bat, delete the build directory, and configure again. "
        "Or run vs_2022.bat, which does those steps.")
    else()
      message(STATUS "PTLib: vcpkg was not found; Windows dependencies are not taken from vcpkg")
    endif()
    unset(_ptlib_vcpkg)
  endif()
  unset(_ptlib_toolchain_ok)
  unset(_ptlib_vcpkg_root)
endif()

# Called after the minsize switch and before project(). An explicit
# VCPKG_MANIFEST_FEATURES list, including an empty one, is left alone.
function(ptlib_vcpkg_windows_features)
  if(NOT CMAKE_HOST_WIN32 OR NOT PTLIB_USE_VCPKG OR VCPKG_MANIFEST_NO_DEFAULT_FEATURES)
    return()
  endif()
  if(NOT CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg\\.cmake$")
    return()
  endif()
  if(DEFINED CACHE{VCPKG_MANIFEST_FEATURES} OR DEFINED ENV{VCPKG_MANIFEST_FEATURES})
    return()
  endif()
  if(VCPKG_MANIFEST_FEATURES)
    return()
  endif()
  # OpenLDAP and ncurses do not support the Visual Studio triplet.
  # unixodbc is skipped because Windows ODBC comes from the SDK.
  # The AWS SDK and PortAudio stay opt-in; V8 has no usable port.
  # ALSA and PulseAudio are Linux-only features.
  set(VCPKG_MANIFEST_FEATURES sasl pcap lua gstreamer PARENT_SCOPE)
  message(STATUS "PTLib: vcpkg features sasl, pcap, lua, gstreamer")
endfunction()
