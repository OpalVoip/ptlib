function(ptlib_add_plugin name family)
  cmake_parse_arguments(ARG "" "" "SOURCES;LIBS" ${ARGN})
  set(target "ptlib_plugin_${name}")
  add_library(${target} MODULE ${ARG_SOURCES})
  set(suffix ".so")
  if(APPLE)
    set(suffix ".dylib")
  elseif(WIN32 OR CYGWIN)
    set(suffix ".dll")
  endif()
  set_target_properties(${target} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "${name}_ptplugin"
    SUFFIX "${suffix}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/${family}")
  target_link_libraries(${target} PRIVATE ptlib ${ARG_LIBS})
  if(PTLIB_IS_APPLE)
    target_link_libraries(${target} PRIVATE objc)
  endif()
  install(TARGETS ${target}
    LIBRARY DESTINATION "${PTLIB_PLUGIN_INSTALL_DIR}/${family}")
endfunction()

if(NOT PTLIB_PLUGINS)
  return()
endif()

if(PTLIB_ALSA_ENABLED)
  ptlib_add_plugin(alsa device/sound
    SOURCES sound_alsa/sound_alsa.cxx
    LIBS ${PTLIB_ALSA_LIBRARY})
  target_include_directories(ptlib_plugin_alsa PRIVATE ${PTLIB_ALSA_INCLUDE})
endif()

if(PTLIB_OSS_ENABLED)
  ptlib_add_plugin(oss device/sound
    SOURCES sound_oss/sound_oss.cxx)
endif()

if(PTLIB_PULSE_ENABLED)
  ptlib_add_plugin(pulse device/sound
    SOURCES sound_pulse/sound_pulse.cxx
    LIBS ${PTLIB_PULSE_LINK})
endif()

if(PTLIB_SHM_AUDIO AND NOT PTLIB_PLATFORM_SOURCES STREQUAL "msos")
  ptlib_add_plugin(ashm device/sound
    SOURCES sound_shm/sound_shm.cxx)
  if(NOT APPLE AND NOT WIN32)
    target_link_libraries(ptlib_plugin_ashm PRIVATE rt)
  endif()
endif()

if(PTLIB_V4L2_ENABLED)
  ptlib_add_plugin(v4l2 device/videoinput
    SOURCES vidinput_v4l2/vidinput_v4l2.cxx vidinput_v4l2/vidinput_names.cxx)
  target_compile_definitions(ptlib_plugin_v4l2 PRIVATE
    "V4L2_HEADER=\"${PTLIB_V4L2_HEADER}\"")
endif()

if(PTLIB_BSDVIDEO_ENABLED)
  ptlib_add_plugin(bsdvideo device/videoinput
    SOURCES vidinput_bsd/vidinput_bsd.cxx)
endif()
