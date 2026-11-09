# Loaded before project(). On Windows, select a vcpkg toolchain when the
# caller did not pass one, then install every manifest feature whose port
# builds for the Visual Studio triplet and whose library the build probes.

if(CMAKE_HOST_WIN32 AND NOT DEFINED PTLIB_USE_VCPKG)
  set(PTLIB_USE_VCPKG ON CACHE BOOL
    "On Windows, use vcpkg for dependencies when a checkout is found")
endif()

function(ptlib_find_vcpkg out)
  set(_found "")
  foreach(_root IN ITEMS "$ENV{VCPKG_ROOT}" "$ENV{VCPKG_INSTALLATION_ROOT}")
    if(_root AND EXISTS "${_root}/scripts/buildsystems/vcpkg.cmake")
      set(_found "${_root}/scripts/buildsystems/vcpkg.cmake")
      break()
    endif()
  endforeach()
  if(NOT _found)
    find_program(PTLIB_VCPKG_EXECUTABLE NAMES vcpkg vcpkg.exe)
    if(PTLIB_VCPKG_EXECUTABLE)
      get_filename_component(_root "${PTLIB_VCPKG_EXECUTABLE}" DIRECTORY)
      if(EXISTS "${_root}/scripts/buildsystems/vcpkg.cmake")
        set(_found "${_root}/scripts/buildsystems/vcpkg.cmake")
      endif()
    endif()
  endif()
  if(NOT _found)
    set(_vswhere "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/Installer/vswhere.exe")
    if(NOT EXISTS "${_vswhere}")
      set(_vswhere "$ENV{ProgramFiles}/Microsoft Visual Studio/Installer/vswhere.exe")
    endif()
    if(EXISTS "${_vswhere}")
      execute_process(
        COMMAND "${_vswhere}" -latest -products * -requires Microsoft.VisualStudio.Component.Vcpkg
                -property installationPath
        OUTPUT_VARIABLE _vs
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
      string(STRIP "${_vs}" _vs)
      if(_vs AND EXISTS "${_vs}/VC/vcpkg/scripts/buildsystems/vcpkg.cmake")
        set(_found "${_vs}/VC/vcpkg/scripts/buildsystems/vcpkg.cmake")
      endif()
    endif()
  endif()
  if(NOT _found)
    foreach(_root IN ITEMS
        "$ENV{USERPROFILE}/vcpkg"
        "$ENV{USERPROFILE}/source/vcpkg"
        "$ENV{LOCALAPPDATA}/vcpkg"
        "C:/vcpkg"
        "C:/src/vcpkg")
      if(_root AND EXISTS "${_root}/scripts/buildsystems/vcpkg.cmake")
        set(_found "${_root}/scripts/buildsystems/vcpkg.cmake")
        break()
      endif()
    endforeach()
  endif()
  if(_found)
    file(TO_CMAKE_PATH "${_found}" _found)
  endif()
  set(${out} "${_found}" PARENT_SCOPE)
endfunction()

if(CMAKE_HOST_WIN32 AND PTLIB_USE_VCPKG AND NOT DEFINED CACHE{CMAKE_TOOLCHAIN_FILE} AND NOT CMAKE_TOOLCHAIN_FILE)
  ptlib_find_vcpkg(_ptlib_vcpkg)
  if(_ptlib_vcpkg)
    set(CMAKE_TOOLCHAIN_FILE "${_ptlib_vcpkg}" CACHE FILEPATH "vcpkg toolchain")
    message(STATUS "PTLib: using vcpkg toolchain ${_ptlib_vcpkg}")
  else()
    message(STATUS "PTLib: vcpkg was not found; Windows dependencies are not taken from vcpkg")
  endif()
  unset(_ptlib_vcpkg)
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
