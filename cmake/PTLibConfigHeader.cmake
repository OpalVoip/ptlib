# Generate include/ptlib_config.h from include/ptlib_config.h.in.
#
# The template uses Autoconf "#undef NAME" lines. tools/configure/configure.cpp
# rewrites those to "#define NAME value", so this generator does the same and
# leaves unknown #undef lines untouched. A trailer defines any enabled macro
# the nested template conditions did not reach.

function(ptlib_define name value)
  set_property(GLOBAL APPEND PROPERTY PTLIB_CONFIG_DEFINE_NAMES "${name}")
  set_property(GLOBAL PROPERTY "PTLIB_CONFIG_DEFINE_VALUE_${name}" "${value}")
endfunction()

function(ptlib_write_config_header input output)
  file(READ "${input}" content)
  get_property(names GLOBAL PROPERTY PTLIB_CONFIG_DEFINE_NAMES)
  if(names)
    list(REMOVE_DUPLICATES names)
  endif()

  foreach(name IN LISTS names)
    get_property(value GLOBAL PROPERTY "PTLIB_CONFIG_DEFINE_VALUE_${name}")
    string(REGEX REPLACE
      "(^|\r?\n)([ \t]*)#undef[ \t]+${name}[ \t]*(\r?\n)"
      "\\1\\2#define ${name} ${value}\\3"
      content "${content}")
  endforeach()

  set(trailer "\n/* Enabled by CMake when nested template conditions skip a #undef. */\n")
  foreach(name IN LISTS names)
    get_property(value GLOBAL PROPERTY "PTLIB_CONFIG_DEFINE_VALUE_${name}")
    string(APPEND trailer "#ifndef ${name}\n#define ${name} ${value}\n#endif\n")
  endforeach()

  string(REGEX REPLACE
    "(\r?\n#endif[ \t]*/\\*[ \t]*_PTLIB_CONFIG_H[ \t]*\\*/[ \t]*\r?\n)"
    "${trailer}\\1"
    content "${content}")

  file(WRITE "${output}" "${content}")
endfunction()
